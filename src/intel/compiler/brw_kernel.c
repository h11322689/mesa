   /*
   * Copyright © 2020 Corporation
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
      #include "brw_kernel.h"
   #include "brw_nir.h"
      #include "nir_clc_helpers.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/spirv/nir_spirv.h"
   #include "dev/intel_debug.h"
   #include "util/u_atomic.h"
      static const nir_shader *
   load_clc_shader(struct brw_compiler *compiler, struct disk_cache *disk_cache,
               {
      if (compiler->clc_shader)
            nir_shader *nir =  nir_load_libclc_shader(64, disk_cache,
               if (nir == NULL)
            const nir_shader *old_nir =
         if (old_nir == NULL) {
      /* We won the race */
      } else {
      /* Someone else built the shader first */
   ralloc_free(nir);
         }
      static nir_builder
   builder_init_new_impl(nir_function *func)
   {
      nir_function_impl *impl = nir_function_impl_create(func);
      }
      static void
   implement_atomic_builtin(nir_function *func, nir_atomic_op atomic_op,
               {
      nir_builder b = builder_init_new_impl(func);
                     nir_deref_instr *ret = NULL;
   ret = nir_build_deref_cast(&b, nir_load_param(&b, p++),
            nir_intrinsic_op op = nir_intrinsic_deref_atomic;
   nir_intrinsic_instr *atomic = nir_intrinsic_instr_create(b.shader, op);
            for (unsigned i = 0; i < nir_intrinsic_infos[op].num_srcs; i++) {
      nir_def *src = nir_load_param(&b, p++);
   if (i == 0) {
      /* The first source is our deref */
   assert(nir_intrinsic_infos[op].src_components[i] == -1);
      }
                        nir_builder_instr_insert(&b, &atomic->instr);
      }
      static void
   implement_sub_group_ballot_builtin(nir_function *func)
   {
      nir_builder b = builder_init_new_impl(func);
   nir_deref_instr *ret =
      nir_build_deref_cast(&b, nir_load_param(&b, 0),
               nir_intrinsic_instr *ballot =
         ballot->src[0] = nir_src_for_ssa(cond);
   ballot->num_components = 1;
   nir_def_init(&ballot->instr, &ballot->def, 1, 32);
               }
      static bool
   implement_intel_builtins(nir_shader *nir)
   {
               nir_foreach_function(func, nir) {
      if (strcmp(func->name, "_Z10atomic_minPU3AS1Vff") == 0) {
      /* float atom_min(__global float volatile *p, float val) */
   implement_atomic_builtin(func, nir_atomic_op_fmin,
            } else if (strcmp(func->name, "_Z10atomic_maxPU3AS1Vff") == 0) {
      /* float atom_max(__global float volatile *p, float val) */
   implement_atomic_builtin(func, nir_atomic_op_fmax,
            } else if (strcmp(func->name, "_Z10atomic_minPU3AS3Vff") == 0) {
      /* float atomic_min(__shared float volatile *, float) */
   implement_atomic_builtin(func, nir_atomic_op_fmin,
            } else if (strcmp(func->name, "_Z10atomic_maxPU3AS3Vff") == 0) {
      /* float atomic_max(__shared float volatile *, float) */
   implement_atomic_builtin(func, nir_atomic_op_fmax,
            } else if (strcmp(func->name, "intel_sub_group_ballot") == 0) {
      implement_sub_group_ballot_builtin(func);
                              }
      static bool
   lower_kernel_intrinsics(nir_shader *nir)
   {
                        unsigned kernel_sysvals_start = 0;
   unsigned kernel_arg_start = sizeof(struct brw_kernel_sysvals);
                     nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
                     nir_intrinsic_instr *load =
         load->num_components = intrin->num_components;
   load->src[0] = nir_src_for_ssa(nir_u2u32(&b, intrin->src[0].ssa));
   nir_intrinsic_set_base(load, kernel_arg_start);
   nir_intrinsic_set_range(load, nir->num_uniforms);
   nir_def_init(&load->instr, &load->def,
                        nir_def_rewrite_uses(&intrin->def, &load->def);
   progress = true;
               case nir_intrinsic_load_constant_base_ptr: {
      b.cursor = nir_instr_remove(&intrin->instr);
   nir_def *const_data_base_addr = nir_pack_64_2x32_split(&b,
      nir_load_reloc_const_intel(&b, BRW_SHADER_RELOC_CONST_DATA_ADDR_LOW),
      nir_def_rewrite_uses(&intrin->def, const_data_base_addr);
   progress = true;
                                 nir_intrinsic_instr *load =
         load->num_components = 3;
   load->src[0] = nir_src_for_ssa(nir_imm_int(&b, 0));
   nir_intrinsic_set_base(load, kernel_sysvals_start +
         nir_intrinsic_set_range(load, 3 * 4);
   nir_def_init(&load->instr, &load->def, 3, 32);
   nir_builder_instr_insert(&b, &load->instr);
   nir_def_rewrite_uses(&intrin->def, &load->def);
   progress = true;
               default:
                        if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      bool
   brw_kernel_from_spirv(struct brw_compiler *compiler,
                        struct disk_cache *disk_cache,
   struct brw_kernel *kernel,
      {
      const struct intel_device_info *devinfo = compiler->devinfo;
   const nir_shader_compiler_options *nir_options =
            struct spirv_to_nir_options spirv_options = {
      .environment = NIR_SPIRV_OPENCL,
   .caps = {
      .address = true,
   .float16 = devinfo->ver >= 8,
   .float64 = devinfo->ver >= 8,
   .groups = true,
   .image_write_without_format = true,
   .int8 = devinfo->ver >= 8,
   .int16 = devinfo->ver >= 8,
   .int64 = devinfo->ver >= 8,
   .int64_atomics = devinfo->ver >= 9,
   .kernel = true,
   .linkage = true, /* We receive linked kernel from clc */
   .float_controls = devinfo->ver >= 8,
   .generic_pointers = true,
   .storage_8bit = devinfo->ver >= 8,
   .storage_16bit = devinfo->ver >= 8,
   .subgroup_arithmetic = true,
   .subgroup_basic = true,
   .subgroup_ballot = true,
   .subgroup_dispatch = true,
   .subgroup_quad = true,
                  .intel_subgroup_shuffle = true,
      },
   .shared_addr_format = nir_address_format_62bit_generic,
   .global_addr_format = nir_address_format_62bit_generic,
   .temp_addr_format = nir_address_format_62bit_generic,
               spirv_options.clc_shader = load_clc_shader(compiler, disk_cache,
         if (spirv_options.clc_shader == NULL) {
      fprintf(stderr, "ERROR: libclc shader missing."
                     assert(spirv_size % 4 == 0);
   nir_shader *nir =
      spirv_to_nir(spirv, spirv_size / 4, NULL, 0, MESA_SHADER_KERNEL,
      nir_validate_shader(nir, "after spirv_to_nir");
   nir_validate_ssa_dominance(nir, "after spirv_to_nir");
   ralloc_steal(mem_ctx, nir);
            if (INTEL_DEBUG(DEBUG_CS)) {
      /* Re-index SSA defs so we print more sensible numbers. */
   nir_foreach_function_impl(impl, nir) {
                  fprintf(stderr, "NIR (from SPIR-V) for kernel\n");
               NIR_PASS_V(nir, implement_intel_builtins);
            /* We have to lower away local constant initializers right before we
   * inline functions.  That way they get properly initialized at the top
   * of the function and not at the top of its caller.
   */
   NIR_PASS_V(nir, nir_lower_variable_initializers, nir_var_function_temp);
   NIR_PASS_V(nir, nir_lower_returns);
   NIR_PASS_V(nir, nir_inline_functions);
   NIR_PASS_V(nir, nir_copy_prop);
            /* Pick off the single entrypoint that we want */
            /* Now that we've deleted all but the main function, we can go ahead and
   * lower the rest of the constant initializers.  We do this here so that
   * nir_remove_dead_variables and split_per_member_structs below see the
   * corresponding stores.
   */
            /* LLVM loves take advantage of the fact that vec3s in OpenCL are 16B
   * aligned and so it can just read/write them as vec4s.  This results in a
   * LOT of vec4->vec3 casts on loads and stores.  One solution to this
   * problem is to get rid of all vec3 variables.
   */
   NIR_PASS_V(nir, nir_lower_vec3_to_vec4,
            nir_var_shader_temp | nir_var_function_temp |
         /* We assign explicit types early so that the optimizer can take advantage
   * of that information and hopefully get rid of some of our memcpys.
   */
   NIR_PASS_V(nir, nir_lower_vars_to_explicit_types,
            nir_var_uniform |
   nir_var_shader_temp | nir_var_function_temp |
         struct brw_nir_compiler_opts opts = {};
            int max_arg_idx = -1;
   nir_foreach_uniform_variable(var, nir) {
      assert(var->data.location < 256);
               kernel->args_size = nir->num_uniforms;
            /* No bindings */
   struct brw_kernel_arg_desc *args =
                  nir_foreach_uniform_variable(var, nir) {
      struct brw_kernel_arg_desc arg_desc = {
      .offset = var->data.driver_location,
      };
            assert(var->data.location >= 0);
                        /* Lower again, this time after dead-variables to get more compact variable
   * layouts.
   */
   nir->global_mem_size = 0;
   nir->scratch_size = 0;
   nir->info.shared_size = 0;
   NIR_PASS_V(nir, nir_lower_vars_to_explicit_types,
            nir_var_shader_temp | nir_var_function_temp |
      if (nir->constant_data_size > 0) {
      assert(nir->constant_data == NULL);
   nir->constant_data = rzalloc_size(nir, nir->constant_data_size);
   nir_gather_explicit_io_initializers(nir, nir->constant_data,
                     if (INTEL_DEBUG(DEBUG_CS)) {
      /* Re-index SSA defs so we print more sensible numbers. */
   nir_foreach_function_impl(impl, nir) {
                  fprintf(stderr, "NIR (before I/O lowering) for kernel\n");
                        NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_constant,
            NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_uniform,
            NIR_PASS_V(nir, nir_lower_explicit_io,
            nir_var_shader_temp | nir_var_function_temp |
                  NIR_PASS_V(nir, brw_nir_lower_cs_intrinsics);
                     memset(&kernel->prog_data, 0, sizeof(kernel->prog_data));
            struct brw_compile_cs_params params = {
      .base = {
      .nir = nir,
   .stats = kernel->stats,
   .log_data = log_data,
      },
   .key = &key,
                        if (error_str)
               }
