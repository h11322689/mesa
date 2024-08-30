   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   *
   * based in part on anv driver which is:
   * Copyright © 2015 Intel Corporation
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
   #include "radv_debug.h"
   #include "radv_llvm_helper.h"
   #include "radv_private.h"
   #include "radv_shader.h"
   #include "radv_shader_args.h"
      #include "ac_binary.h"
   #include "ac_llvm_build.h"
   #include "ac_nir.h"
   #include "ac_nir_to_llvm.h"
   #include "ac_shader_abi.h"
   #include "ac_shader_util.h"
   #include "sid.h"
      struct radv_shader_context {
      struct ac_llvm_context ac;
   const struct nir_shader *shader;
   struct ac_shader_abi abi;
   const struct radv_nir_compiler_options *options;
   const struct radv_shader_info *shader_info;
                     unsigned max_workgroup_size;
   LLVMContextRef context;
                                 };
      static inline struct radv_shader_context *
   radv_shader_context_from_abi(struct ac_shader_abi *abi)
   {
         }
      static struct ac_llvm_pointer
   create_llvm_function(struct ac_llvm_context *ctx, LLVMModuleRef module, LLVMBuilderRef builder,
               {
               if (options->info->address32_hi) {
      ac_llvm_add_target_dep_function_attr(main_function.value, "amdgpu-32bit-address-high-bits",
               ac_llvm_set_workgroup_size(main_function.value, max_workgroup_size);
               }
      static void
   load_descriptor_sets(struct radv_shader_context *ctx)
   {
      const struct radv_userdata_locations *user_sgprs_locs = &ctx->shader_info->user_sgprs_locs;
            if (user_sgprs_locs->shader_data[AC_UD_INDIRECT_DESCRIPTOR_SETS].sgpr_idx != -1) {
      struct ac_llvm_pointer desc_sets = ac_get_ptr_arg(&ctx->ac, &ctx->args->ac, ctx->args->descriptor_sets[0]);
   while (mask) {
               ctx->descriptor_sets[i] = ac_build_load_to_sgpr(&ctx->ac, desc_sets, LLVMConstInt(ctx->ac.i32, i, false));
         } else {
      while (mask) {
                        }
      static enum ac_llvm_calling_convention
   get_llvm_calling_convention(LLVMValueRef func, gl_shader_stage stage)
   {
      switch (stage) {
   case MESA_SHADER_VERTEX:
   case MESA_SHADER_TESS_EVAL:
      return AC_LLVM_AMDGPU_VS;
      case MESA_SHADER_GEOMETRY:
      return AC_LLVM_AMDGPU_GS;
      case MESA_SHADER_TESS_CTRL:
      return AC_LLVM_AMDGPU_HS;
      case MESA_SHADER_FRAGMENT:
      return AC_LLVM_AMDGPU_PS;
      case MESA_SHADER_COMPUTE:
      return AC_LLVM_AMDGPU_CS;
      default:
            }
      /* Returns whether the stage is a stage that can be directly before the GS */
   static bool
   is_pre_gs_stage(gl_shader_stage stage)
   {
         }
      static void
   create_function(struct radv_shader_context *ctx, gl_shader_stage stage, bool has_previous_stage)
   {
      if (ctx->ac.gfx_level >= GFX10) {
      if (is_pre_gs_stage(stage) && ctx->shader_info->is_ngg) {
      /* On GFX10+, VS and TES are merged into GS for NGG. */
   stage = MESA_SHADER_GEOMETRY;
                  ctx->main_function = create_llvm_function(&ctx->ac, ctx->ac.module, ctx->ac.builder, &ctx->args->ac,
                           if (stage == MESA_SHADER_TESS_CTRL || (stage == MESA_SHADER_VERTEX && ctx->shader_info->vs.as_ls) ||
      ctx->shader_info->is_ngg ||
   /* GFX9 has the ESGS ring buffer in LDS. */
   (stage == MESA_SHADER_GEOMETRY && has_previous_stage)) {
         }
      static LLVMValueRef
   radv_load_base_vertex(struct ac_shader_abi *abi, bool non_indexed_is_zero)
   {
      struct radv_shader_context *ctx = radv_shader_context_from_abi(abi);
      }
      static LLVMValueRef
   radv_load_rsrc(struct radv_shader_context *ctx, LLVMValueRef ptr, LLVMTypeRef type)
   {
      if (ptr && LLVMTypeOf(ptr) == ctx->ac.i32) {
               LLVMTypeRef ptr_type = LLVMPointerType(type, AC_ADDR_SPACE_CONST_32BIT);
   ptr = LLVMBuildIntToPtr(ctx->ac.builder, ptr, ptr_type, "");
            result = LLVMBuildLoad2(ctx->ac.builder, type, ptr, "");
                           }
      static LLVMValueRef
   radv_load_ubo(struct ac_shader_abi *abi, LLVMValueRef buffer_ptr)
   {
      struct radv_shader_context *ctx = radv_shader_context_from_abi(abi);
      }
      static LLVMValueRef
   radv_load_ssbo(struct ac_shader_abi *abi, LLVMValueRef buffer_ptr, bool write, bool non_uniform)
   {
      struct radv_shader_context *ctx = radv_shader_context_from_abi(abi);
      }
      static LLVMValueRef
   radv_get_sampler_desc(struct ac_shader_abi *abi, LLVMValueRef index, enum ac_descriptor_type desc_type)
   {
               /* 3 plane formats always have same size and format for plane 1 & 2, so
   * use the tail from plane 1 so that we can store only the first 16 bytes
   * of the last plane. */
   if (desc_type == AC_DESC_PLANE_2 && index && LLVMTypeOf(index) == ctx->ac.i32) {
      LLVMValueRef plane1_addr = LLVMBuildSub(ctx->ac.builder, index, LLVMConstInt(ctx->ac.i32, 32, false), "");
   LLVMValueRef descriptor1 = radv_load_rsrc(ctx, plane1_addr, ctx->ac.v8i32);
            LLVMValueRef components[8];
   for (unsigned i = 0; i < 4; ++i)
            for (unsigned i = 4; i < 8; ++i)
                     bool v4 = desc_type == AC_DESC_BUFFER || desc_type == AC_DESC_SAMPLER;
      }
      static void
   scan_shader_output_decl(struct radv_shader_context *ctx, struct nir_variable *variable, struct nir_shader *shader,
         {
      int idx = variable->data.driver_location;
   unsigned attrib_count = glsl_count_attribute_slots(variable->type, false);
            if (variable->data.compact) {
      unsigned component_count = variable->data.location_frac + glsl_get_length(variable->type);
                           }
      static LLVMValueRef
   radv_load_output(struct radv_shader_context *ctx, unsigned index, unsigned chan)
   {
      int idx = ac_llvm_reg_index_soa(index, chan);
   LLVMValueRef output = ctx->abi.outputs[idx];
   LLVMTypeRef type = ctx->abi.is_16bit[idx] ? ctx->ac.f16 : ctx->ac.f32;
      }
      static void
   ac_llvm_finalize_module(struct radv_shader_context *ctx, LLVMPassManagerRef passmgr)
   {
      LLVMRunPassManager(passmgr, ctx->ac.module);
               }
      static void
   prepare_gs_input_vgprs(struct radv_shader_context *ctx, bool merged)
   {
      if (merged) {
         } else {
            }
      /* Ensure that the esgs ring is declared.
   *
   * We declare it with 64KB alignment as a hint that the
   * pointer value will always be 0.
   */
   static void
   declare_esgs_ring(struct radv_shader_context *ctx)
   {
               LLVMValueRef esgs_ring =
         LLVMSetLinkage(esgs_ring, LLVMExternalLinkage);
      }
      static LLVMValueRef
   radv_intrinsic_load(struct ac_shader_abi *abi, nir_intrinsic_instr *intrin)
   {
      switch (intrin->intrinsic) {
   case nir_intrinsic_load_base_vertex:
   case nir_intrinsic_load_first_vertex:
         default:
            }
      static LLVMModuleRef
   ac_translate_nir_to_llvm(struct ac_llvm_compiler *ac_llvm, const struct radv_nir_compiler_options *options,
               {
      struct radv_shader_context ctx = {0};
   ctx.args = args;
   ctx.options = options;
                     if (shaders[0]->info.float_controls_execution_mode & FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP32) {
                  bool exports_mrtz = false;
   bool exports_color_null = false;
   if (shaders[0]->info.stage == MESA_SHADER_FRAGMENT) {
      exports_mrtz = info->ps.writes_z || info->ps.writes_stencil || info->ps.writes_sample_mask;
               ac_llvm_context_init(&ctx.ac, ac_llvm, options->info, float_mode, info->wave_size, info->ballot_bit_size,
            uint32_t length = 1;
   for (uint32_t i = 0; i < shader_count; i++)
      if (shaders[i]->info.name)
         char *name = malloc(length);
   if (name) {
      uint32_t offset = 0;
   for (uint32_t i = 0; i < shader_count; i++) {
                     strcpy(name + offset, shaders[i]->info.name);
   offset += strlen(shaders[i]->info.name);
   if (i != shader_count - 1)
                                                      ctx.abi.intrinsic_load = radv_intrinsic_load;
   ctx.abi.load_ubo = radv_load_ubo;
   ctx.abi.load_ssbo = radv_load_ssbo;
   ctx.abi.load_sampler_desc = radv_get_sampler_desc;
   ctx.abi.clamp_shadow_reference = false;
   ctx.abi.robust_buffer_access = options->robust_buffer_access_llvm;
            bool is_ngg = is_pre_gs_stage(shaders[0]->info.stage) && info->is_ngg;
   if (shader_count >= 2 || is_ngg)
            if (args->ac.vertex_id.used)
         if (args->ac.vs_rel_patch_id.used)
         if (args->ac.instance_id.used)
            if (options->info->has_ls_vgpr_init_bug && shaders[shader_count - 1]->info.stage == MESA_SHADER_TESS_CTRL)
            if (is_ngg) {
      if (!info->is_ngg_passthrough)
            if (ctx.stage == MESA_SHADER_GEOMETRY) {
      /* Scratch space used by NGG GS for repacking vertices at the end. */
   LLVMTypeRef ai32 = LLVMArrayType(ctx.ac.i32, 8);
   LLVMValueRef gs_ngg_scratch =
         LLVMSetInitializer(gs_ngg_scratch, LLVMGetUndef(ai32));
                  /* Vertex emit space used by NGG GS for storing all vertex attributes. */
   LLVMValueRef gs_ngg_emit =
         LLVMSetInitializer(gs_ngg_emit, LLVMGetUndef(ai32));
   LLVMSetLinkage(gs_ngg_emit, LLVMExternalLinkage);
               /* GFX10 hang workaround - there needs to be an s_barrier before gs_alloc_req always */
   if (ctx.ac.gfx_level == GFX10 && shader_count == 1)
               for (int shader_idx = 0; shader_idx < shader_count; ++shader_idx) {
      ctx.stage = shaders[shader_idx]->info.stage;
   ctx.shader = shaders[shader_idx];
            if (shader_idx && !(shaders[shader_idx]->info.stage == MESA_SHADER_GEOMETRY && info->is_ngg)) {
      /* Execute a barrier before the second shader in
   * a merged shader.
   *
   * Execute the barrier inside the conditional block,
   * so that empty waves can jump directly to s_endpgm,
   * which will also signal the barrier.
   *
   * This is possible in gfx9, because an empty wave
   * for the second shader does not participate in
   * the epilogue. With NGG, empty waves may still
   * be required to export data (e.g. GS output vertices),
   * so we cannot let them exit early.
   *
   * If the shader is TCS and the TCS epilog is present
   * and contains a barrier, it will wait there and then
   * reach s_endpgm.
   */
   ac_build_waitcnt(&ctx.ac, AC_WAIT_LGKM);
               nir_foreach_shader_out_variable (variable, shaders[shader_idx])
            bool check_merged_wave_info = shader_count >= 2 && !(is_ngg && shader_idx == 1);
            if (check_merged_wave_info) {
      LLVMValueRef fn = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx.ac.builder));
                  LLVMValueRef count =
         LLVMValueRef thread_id = ac_get_thread_id(&ctx.ac);
                              if (shaders[shader_idx]->info.stage == MESA_SHADER_GEOMETRY && !info->is_ngg)
            if (!ac_nir_translate(&ctx.ac, &ctx.abi, &args->ac, shaders[shader_idx])) {
                  if (check_merged_wave_info) {
      LLVMBuildBr(ctx.ac.builder, merge_block);
                           if (options->dump_preoptir) {
      fprintf(stderr, "%s LLVM IR:\n\n", radv_get_shader_name(info, shaders[shader_count - 1]->info.stage));
   ac_dump_module(ctx.ac.module);
                                    }
      static void
   ac_diagnostic_handler(LLVMDiagnosticInfoRef di, void *context)
   {
      unsigned *retval = (unsigned *)context;
   LLVMDiagnosticSeverity severity = LLVMGetDiagInfoSeverity(di);
            if (severity == LLVMDSError) {
      *retval = 1;
                  }
      static unsigned
   radv_llvm_compile(LLVMModuleRef M, char **pelf_buffer, size_t *pelf_size, struct ac_llvm_compiler *ac_llvm)
   {
      unsigned retval = 0;
            /* Setup Diagnostic Handler*/
                     /* Compile IR*/
   if (!radv_compile_to_elf(ac_llvm, M, pelf_buffer, pelf_size))
            }
      static void
   ac_compile_llvm_module(struct ac_llvm_compiler *ac_llvm, LLVMModuleRef llvm_module, struct radv_shader_binary **rbinary,
         {
      char *elf_buffer = NULL;
   size_t elf_size = 0;
            if (options->dump_shader) {
      fprintf(stderr, "%s LLVM IR:\n\n", name);
   ac_dump_module(llvm_module);
               if (options->record_ir) {
      char *llvm_ir = LLVMPrintModuleToString(llvm_module);
   llvm_ir_string = strdup(llvm_ir);
               int v = radv_llvm_compile(llvm_module, &elf_buffer, &elf_size, ac_llvm);
   if (v) {
                  LLVMContextRef ctx = LLVMGetModuleContext(llvm_module);
   LLVMDisposeModule(llvm_module);
            size_t llvm_ir_size = llvm_ir_string ? strlen(llvm_ir_string) : 0;
   size_t alloc_size = sizeof(struct radv_shader_binary_rtld) + elf_size + llvm_ir_size + 1;
   struct radv_shader_binary_rtld *rbin = calloc(1, alloc_size);
   memcpy(rbin->data, elf_buffer, elf_size);
   if (llvm_ir_string)
            rbin->base.type = RADV_BINARY_TYPE_RTLD;
   rbin->base.total_size = alloc_size;
   rbin->elf_size = elf_size;
   rbin->llvm_ir_size = llvm_ir_size;
            free(llvm_ir_string);
      }
      static void
   radv_compile_nir_shader(struct ac_llvm_compiler *ac_llvm, const struct radv_nir_compiler_options *options,
               {
                           ac_compile_llvm_module(ac_llvm, llvm_module, rbinary, radv_get_shader_name(info, nir[nir_count - 1]->info.stage),
      }
      void
   llvm_compile_shader(const struct radv_nir_compiler_options *options, const struct radv_shader_info *info,
               {
      enum ac_target_machine_options tm_options = 0;
            tm_options |= AC_TM_SUPPORTS_SPILL;
   if (options->check_ir)
                        }
