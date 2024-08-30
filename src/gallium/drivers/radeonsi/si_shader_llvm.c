   /*
   * Copyright 2016 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_nir.h"
   #include "ac_nir_to_llvm.h"
   #include "ac_rtld.h"
   #include "si_pipe.h"
   #include "si_shader_internal.h"
   #include "sid.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
      struct si_llvm_diagnostics {
      struct util_debug_callback *debug;
      };
      static void si_diagnostic_handler(LLVMDiagnosticInfoRef di, void *context)
   {
      struct si_llvm_diagnostics *diag = (struct si_llvm_diagnostics *)context;
   LLVMDiagnosticSeverity severity = LLVMGetDiagInfoSeverity(di);
            switch (severity) {
   case LLVMDSError:
      severity_str = "error";
      case LLVMDSWarning:
      severity_str = "warning";
      case LLVMDSRemark:
   case LLVMDSNote:
   default:
                           util_debug_message(diag->debug, SHADER_INFO, "LLVM diagnostic (%s): %s", severity_str,
            if (severity == LLVMDSError) {
      diag->retval = 1;
                  }
      bool si_compile_llvm(struct si_screen *sscreen, struct si_shader_binary *binary,
                     {
               if (si_can_dump_shader(sscreen, stage, SI_DUMP_LLVM_IR)) {
               fprintf(stderr, "%s LLVM IR:\n\n", name);
   ac_dump_module(ac->module);
               if (sscreen->record_llvm_ir) {
      char *ir = LLVMPrintModuleToString(ac->module);
   binary->llvm_ir_string = strdup(ir);
               if (!si_replace_shader(count, binary)) {
               if (less_optimized && compiler->low_opt_passes)
            struct si_llvm_diagnostics diag = {debug};
            if (!ac_compile_module_to_elf(passes, ac->module, (char **)&binary->code_buffer,
                  if (diag.retval != 0) {
      util_debug_message(debug, SHADER_INFO, "LLVM compilation failed");
                           struct ac_rtld_binary rtld;
   if (!ac_rtld_open(&rtld, (struct ac_rtld_open_info){
                              .info = &sscreen->info,
   .shader_type = stage,
         bool ok = ac_rtld_read_config(&sscreen->info, &rtld, conf);
   ac_rtld_close(&rtld);
      }
      void si_llvm_context_init(struct si_shader_context *ctx, struct si_screen *sscreen,
                     {
      memset(ctx, 0, sizeof(*ctx));
   ctx->screen = sscreen;
            ac_llvm_context_init(&ctx->ac, compiler, &sscreen->info, float_mode,
      }
      void si_llvm_create_func(struct si_shader_context *ctx, const char *name, LLVMTypeRef *return_types,
         {
      LLVMTypeRef ret_type;
            if (num_return_elems)
         else
                     /* LS is merged into HS (TCS), and ES is merged into GS. */
   if (ctx->screen->info.gfx_level >= GFX9 && ctx->stage <= MESA_SHADER_GEOMETRY) {
      if (ctx->shader->key.ge.as_ls)
         else if (ctx->shader->key.ge.as_es || ctx->shader->key.ge.as_ngg)
               switch (real_stage) {
   case MESA_SHADER_VERTEX:
   case MESA_SHADER_TESS_EVAL:
      call_conv = AC_LLVM_AMDGPU_VS;
      case MESA_SHADER_TESS_CTRL:
      call_conv = AC_LLVM_AMDGPU_HS;
      case MESA_SHADER_GEOMETRY:
      call_conv = AC_LLVM_AMDGPU_GS;
      case MESA_SHADER_FRAGMENT:
      call_conv = AC_LLVM_AMDGPU_PS;
      case MESA_SHADER_COMPUTE:
      call_conv = AC_LLVM_AMDGPU_CS;
      default:
                  /* Setup the function */
   ctx->return_type = ret_type;
   ctx->main_fn = ac_build_main(&ctx->args->ac, &ctx->ac, call_conv, name, ret_type, ctx->ac.module);
            if (ctx->screen->info.address32_hi) {
      ac_llvm_add_target_dep_function_attr(ctx->main_fn.value, "amdgpu-32bit-address-high-bits",
               if (ctx->stage <= MESA_SHADER_GEOMETRY && ctx->shader->key.ge.as_ngg &&
      si_shader_uses_streamout(ctx->shader))
         ac_llvm_set_workgroup_size(ctx->main_fn.value, max_workgroup_size);
      }
      void si_llvm_create_main_func(struct si_shader_context *ctx)
   {
      struct si_shader *shader = ctx->shader;
   LLVMTypeRef returns[AC_MAX_ARGS];
            for (i = 0; i < ctx->args->ac.num_sgprs_returned; i++)
         for (; i < ctx->args->ac.return_count; i++)
            si_llvm_create_func(ctx, "main", returns, ctx->args->ac.return_count,
            /* Reserve register locations for VGPR inputs the PS prolog may need. */
   if (ctx->stage == MESA_SHADER_FRAGMENT && !ctx->shader->is_monolithic) {
      ac_llvm_add_target_dep_function_attr(
                  if (ctx->stage <= MESA_SHADER_GEOMETRY &&
      (shader->key.ge.as_ls || ctx->stage == MESA_SHADER_TESS_CTRL)) {
   /* The LSHS size is not known until draw time, so we append it
   * at the end of whatever LDS use there may be in the rest of
   * the shader (currently none, unless LLVM decides to do its
   * own LDS-based lowering).
   */
   ctx->ac.lds = (struct ac_llvm_pointer) {
      .value = LLVMAddGlobalInAddressSpace(ctx->ac.module, LLVMArrayType(ctx->ac.i32, 0),
            };
               if (ctx->stage == MESA_SHADER_VERTEX) {
      ctx->abi.vertex_id = ac_get_arg(&ctx->ac, ctx->args->ac.vertex_id);
   ctx->abi.instance_id = ac_get_arg(&ctx->ac, ctx->args->ac.instance_id);
   if (ctx->args->ac.vs_rel_patch_id.used)
            /* Non-monolithic shaders apply the LS-HS input VGPR hw bug workaround in
   * the VS prolog, while monolithic shaders apply it here.
   */
   if (shader->is_monolithic && shader->key.ge.part.vs.prolog.ls_vgpr_fix)
         }
      void si_llvm_optimize_module(struct si_shader_context *ctx)
   {
      /* Dump LLVM IR before any optimization passes */
   if (si_can_dump_shader(ctx->screen, ctx->stage, SI_DUMP_INIT_LLVM_IR))
            /* Run the pass */
   LLVMRunPassManager(ctx->compiler->passmgr, ctx->ac.module);
      }
      void si_llvm_dispose(struct si_shader_context *ctx)
   {
      LLVMDisposeModule(ctx->ac.module);
   LLVMContextDispose(ctx->ac.context);
      }
      /**
   * Load a dword from a constant buffer.
   */
   LLVMValueRef si_buffer_load_const(struct si_shader_context *ctx, LLVMValueRef resource,
         {
      return ac_build_buffer_load(&ctx->ac, resource, 1, NULL, offset, NULL, ctx->ac.f32,
      }
      void si_llvm_build_ret(struct si_shader_context *ctx, LLVMValueRef ret)
   {
      if (LLVMGetTypeKind(LLVMTypeOf(ret)) == LLVMVoidTypeKind)
         else
      }
      LLVMValueRef si_insert_input_ret(struct si_shader_context *ctx, LLVMValueRef ret,
         {
         }
      LLVMValueRef si_insert_input_ret_float(struct si_shader_context *ctx, LLVMValueRef ret,
         {
      LLVMBuilderRef builder = ctx->ac.builder;
               }
      LLVMValueRef si_insert_input_ptr(struct si_shader_context *ctx, LLVMValueRef ret,
         {
      LLVMBuilderRef builder = ctx->ac.builder;
   LLVMValueRef ptr = ac_get_arg(&ctx->ac, param);
   ptr = LLVMBuildPtrToInt(builder, ptr, ctx->ac.i32, "");
      }
      LLVMValueRef si_prolog_get_internal_binding_slot(struct si_shader_context *ctx, unsigned slot)
   {
      LLVMValueRef list = LLVMBuildIntToPtr(
      ctx->ac.builder, ac_get_arg(&ctx->ac, ctx->args->internal_bindings),
               return ac_build_load_to_sgpr(&ctx->ac,
            }
      /* Ensure that the esgs ring is declared.
   *
   * We declare it with 64KB alignment as a hint that the
   * pointer value will always be 0.
   */
   static void si_llvm_declare_lds_esgs_ring(struct si_shader_context *ctx)
   {
      if (ctx->ac.lds.value)
                     LLVMValueRef esgs_ring =
      LLVMAddGlobalInAddressSpace(ctx->ac.module, LLVMArrayType(ctx->ac.i32, 0),
      LLVMSetLinkage(esgs_ring, LLVMExternalLinkage);
            ctx->ac.lds.value = esgs_ring;
      }
      static void si_init_exec_from_input(struct si_shader_context *ctx, struct ac_arg param,
         {
      LLVMValueRef args[] = {
      ac_get_arg(&ctx->ac, param),
      };
      }
      /**
   * Get the value of a shader input parameter and extract a bitfield.
   */
   static LLVMValueRef unpack_llvm_param(struct si_shader_context *ctx, LLVMValueRef value,
         {
      if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMFloatTypeKind)
            if (rshift)
            if (rshift + bitwidth < 32) {
      unsigned mask = (1 << bitwidth) - 1;
                  }
      LLVMValueRef si_unpack_param(struct si_shader_context *ctx, struct ac_arg param, unsigned rshift,
         {
                  }
      static void si_llvm_declare_compute_memory(struct si_shader_context *ctx)
   {
      struct si_shader_selector *sel = ctx->shader->selector;
            LLVMTypeRef i8p = LLVMPointerType(ctx->ac.i8, AC_ADDR_SPACE_LDS);
                     LLVMTypeRef type = LLVMArrayType(ctx->ac.i8, lds_size);
   var = LLVMAddGlobalInAddressSpace(ctx->ac.module, type,
                  ctx->ac.lds = (struct ac_llvm_pointer) {
      .value = LLVMBuildBitCast(ctx->ac.builder, var, i8p, ""),
         }
      /**
   * Given two parts (LS/HS or ES/GS) of a merged shader, build a wrapper function that
   * runs them in sequence to form a monolithic shader.
   */
   static void si_build_wrapper_function(struct si_shader_context *ctx,
               {
               for (unsigned i = 0; i < 2; ++i) {
      ac_add_function_attr(ctx->ac.context, parts[i].value, -1, "alwaysinline");
                        if (same_thread_count) {
         } else {
               LLVMValueRef count = ac_get_arg(&ctx->ac, ctx->args->ac.merged_wave_info);
            LLVMValueRef ena = LLVMBuildICmp(builder, LLVMIntULT, ac_get_thread_id(&ctx->ac), count, "");
               LLVMValueRef params[AC_MAX_ARGS];
   unsigned num_params = LLVMCountParams(ctx->main_fn.value);
            /* wrapper function has same parameter as first part shader */
   LLVMValueRef ret =
            if (same_thread_count) {
      LLVMTypeRef type = LLVMTypeOf(ret);
            /* output of first part shader is the input of the second part */
   num_params = LLVMCountStructElementTypes(type);
            for (unsigned i = 0; i < num_params; i++) {
               /* Convert return value to same type as next shader's input param. */
   LLVMTypeRef ret_type = LLVMTypeOf(params[i]);
   LLVMTypeRef param_type = LLVMTypeOf(LLVMGetParam(parts[1].value, i));
                  if (ret_type != param_type) {
      if (LLVMGetTypeKind(param_type) == LLVMPointerTypeKind) {
                        } else {
                  } else {
               if (ctx->stage == MESA_SHADER_TESS_CTRL) {
      LLVMValueRef count = ac_get_arg(&ctx->ac, ctx->args->ac.merged_wave_info);
                  LLVMValueRef ena = LLVMBuildICmp(builder, LLVMIntULT, ac_get_thread_id(&ctx->ac), count, "");
               /* The second half of the merged shader should use
   * the inputs from the toplevel (wrapper) function,
   * not the return value from the last call.
   *
   * That's because the last call was executed condi-
   * tionally, so we can't consume it in the main
   * block.
            /* Second part params are same as the preceeding params of the first part. */
                        /* Close the conditional wrapping the second shader. */
   if (ctx->stage == MESA_SHADER_TESS_CTRL && !same_thread_count)
               }
      static LLVMValueRef si_llvm_load_intrinsic(struct ac_shader_abi *abi, nir_intrinsic_instr *intrin)
   {
               switch (intrin->intrinsic) {
   case nir_intrinsic_load_tess_rel_patch_id_amd:
            case nir_intrinsic_load_lds_ngg_scratch_base_amd:
            case nir_intrinsic_load_lds_ngg_gs_out_vertex_base_amd:
            default:
            }
      static LLVMValueRef si_llvm_load_sampler_desc(struct ac_shader_abi *abi, LLVMValueRef index,
         {
      struct si_shader_context *ctx = si_shader_context_from_abi(abi);
            if (index && LLVMTypeOf(index) == ctx->ac.i32) {
               switch (desc_type) {
   case AC_DESC_IMAGE:
      /* The image is at [0:7]. */
   index = LLVMBuildMul(builder, index, LLVMConstInt(ctx->ac.i32, 2, 0), "");
      case AC_DESC_BUFFER:
      /* The buffer is in [4:7]. */
   index = ac_build_imad(&ctx->ac, index, LLVMConstInt(ctx->ac.i32, 4, 0), ctx->ac.i32_1);
   is_vec4 = true;
      case AC_DESC_FMASK:
      /* The FMASK is at [8:15]. */
   assert(ctx->screen->info.gfx_level < GFX11);
   index = ac_build_imad(&ctx->ac, index, LLVMConstInt(ctx->ac.i32, 2, 0), ctx->ac.i32_1);
      case AC_DESC_SAMPLER:
      /* The sampler state is at [12:15]. */
   index = ac_build_imad(&ctx->ac, index, LLVMConstInt(ctx->ac.i32, 4, 0),
         is_vec4 = true;
      default:
                  struct ac_llvm_pointer list = {
      .value = ac_get_arg(&ctx->ac, ctx->args->samplers_and_images),
                              }
      static bool si_llvm_translate_nir(struct si_shader_context *ctx, struct si_shader *shader,
         {
      struct si_shader_selector *sel = shader->selector;
            ctx->shader = shader;
            ctx->num_const_buffers = info->base.num_ubos;
            ctx->num_samplers = BITSET_LAST_BIT(info->base.textures_used);
            ctx->abi.intrinsic_load = si_llvm_load_intrinsic;
                     switch (ctx->stage) {
   case MESA_SHADER_VERTEX:
      /* preload instance_divisor_constbuf to be used for input load after culling */
   if (ctx->shader->key.ge.opt.ngg_culling &&
      ctx->shader->key.ge.part.vs.prolog.instance_divisor_is_fetched) {
   struct ac_llvm_pointer buf = ac_get_ptr_arg(&ctx->ac, &ctx->args->ac, ctx->args->internal_bindings);
   ctx->instance_divisor_constbuf =
      ac_build_load_to_sgpr(
   }
         case MESA_SHADER_TESS_CTRL:
      si_llvm_init_tcs_callbacks(ctx);
         case MESA_SHADER_GEOMETRY:
      if (ctx->shader->key.ge.as_ngg) {
      LLVMTypeRef ai32 = LLVMArrayType(ctx->ac.i32, gfx10_ngg_get_scratch_dw_size(shader));
   ctx->gs_ngg_scratch = (struct ac_llvm_pointer) {
      .value = LLVMAddGlobalInAddressSpace(ctx->ac.module, ai32, "ngg_scratch", AC_ADDR_SPACE_LDS),
      };
                  ctx->gs_ngg_emit = LLVMAddGlobalInAddressSpace(
         LLVMSetLinkage(ctx->gs_ngg_emit, LLVMExternalLinkage);
      }
         case MESA_SHADER_FRAGMENT: {
      ctx->abi.kill_ps_if_inf_interp =
      ctx->screen->options.no_infinite_interp &&
   (ctx->shader->selector->info.uses_persp_center ||
   ctx->shader->selector->info.uses_persp_centroid ||
                  case MESA_SHADER_COMPUTE:
      if (ctx->shader->selector->info.base.shared_size)
               default:
                  bool is_merged_esgs_stage =
      ctx->screen->info.gfx_level >= GFX9 && ctx->stage <= MESA_SHADER_GEOMETRY &&
         bool is_nogs_ngg_stage =
      (ctx->stage == MESA_SHADER_VERTEX || ctx->stage == MESA_SHADER_TESS_EVAL) &&
         /* Declare the ESGS ring as an explicit LDS symbol.
   * When NGG VS/TES, unconditionally declare for streamout and vertex compaction.
   * Whether space is actually allocated is determined during linking / PM4 creation.
   */
   if (is_merged_esgs_stage || is_nogs_ngg_stage)
            /* This is really only needed when streamout and / or vertex
   * compaction is enabled.
   */
   if (is_nogs_ngg_stage &&
      (si_shader_uses_streamout(shader) || shader->key.ge.opt.ngg_culling)) {
   LLVMTypeRef asi32 = LLVMArrayType(ctx->ac.i32, gfx10_ngg_get_scratch_dw_size(shader));
   ctx->gs_ngg_scratch = (struct ac_llvm_pointer) {
      .value = LLVMAddGlobalInAddressSpace(ctx->ac.module, asi32, "ngg_scratch",
            };
   LLVMSetInitializer(ctx->gs_ngg_scratch.value, LLVMGetUndef(asi32));
               /* For merged shaders (VS-TCS, VS-GS, TES-GS): */
   if (ctx->screen->info.gfx_level >= GFX9 && si_is_merged_shader(shader)) {
      /* Set EXEC = ~0 before the first shader. For monolithic shaders, the wrapper
   * function does this.
   */
   if (ctx->stage == MESA_SHADER_TESS_EVAL) {
      /* TES has only 1 shader part, therefore it doesn't use the wrapper function. */
   if (!shader->is_monolithic || !shader->key.ge.as_es)
      } else if (ctx->stage == MESA_SHADER_VERTEX) {
      if (shader->is_monolithic) {
      /* Only mono VS with TCS/GS present has wrapper function. */
   if (!shader->key.ge.as_ls && !shader->key.ge.as_es)
      } else {
      /* If the prolog is present, EXEC is set there instead. */
   if (!si_vs_needs_prolog(sel, &shader->key.ge.part.vs.prolog))
                  /* NGG VS and NGG TES: nir ngg lowering send gs_alloc_req at the beginning when culling
   * is disabled, but GFX10 may hang if not all waves are launched before gs_alloc_req.
   * We work around this HW bug by inserting a barrier before gs_alloc_req.
   */
   if (ctx->screen->info.gfx_level == GFX10 &&
      (ctx->stage == MESA_SHADER_VERTEX || ctx->stage == MESA_SHADER_TESS_EVAL) &&
                        if ((ctx->stage == MESA_SHADER_GEOMETRY && !shader->key.ge.as_ngg) ||
      (ctx->stage == MESA_SHADER_TESS_CTRL && !shader->is_monolithic)) {
   /* Wrap both shaders in an if statement according to the number of enabled threads
   * there. For monolithic TCS, the if statement is inserted by the wrapper function,
   * not here. For NGG GS, the if statement is inserted by nir lowering.
   */
      } else if ((shader->key.ge.as_ls || shader->key.ge.as_es) && !shader->is_monolithic) {
      /* For monolithic LS (VS before TCS) and ES (VS before GS and TES before GS),
   * the if statement is inserted by the wrapper function.
   */
               if (thread_enabled) {
      ctx->merged_wrap_if_entry_block = LLVMGetInsertBlock(ctx->ac.builder);
   ctx->merged_wrap_if_label = 11500;
               /* Execute a barrier before the second shader in
   * a merged shader.
   *
   * Execute the barrier inside the conditional block,
   * so that empty waves can jump directly to s_endpgm,
   * which will also signal the barrier.
   *
   * This is possible in gfx9, because an empty wave for the second shader does not insert
   * any ending. With NGG, empty waves may still be required to export data (e.g. GS output
   * vertices), so we cannot let them exit early.
   *
   * If the shader is TCS and the TCS epilog is present
   * and contains a barrier, it will wait there and then
   * reach s_endpgm.
   */
   if (ctx->stage == MESA_SHADER_TESS_CTRL) {
      /* We need the barrier only if TCS inputs are read from LDS. */
   if (!shader->key.ge.opt.same_patch_vertices ||
      shader->selector->info.base.inputs_read &
                  /* If both input and output patches are wholly in one wave, we don't need a barrier.
   * That's true when both VS and TCS have the same number of patch vertices and
   * the wave size is a multiple of the number of patch vertices.
   */
   if (!shader->key.ge.opt.same_patch_vertices ||
      ctx->ac.wave_size % sel->info.base.tess.tcs_vertices_out != 0)
      } else if (ctx->stage == MESA_SHADER_GEOMETRY) {
      ac_build_waitcnt(&ctx->ac, AC_WAIT_LGKM);
                  ctx->abi.clamp_shadow_reference = true;
   ctx->abi.robust_buffer_access = true;
   ctx->abi.load_grid_size_from_user_sgpr = true;
   ctx->abi.clamp_div_by_zero = ctx->screen->options.clamp_div_by_zero ||
                  bool ls_need_output =
      ctx->stage == MESA_SHADER_VERTEX && shader->key.ge.as_ls &&
         bool tcs_need_output =
                     if (ls_need_output || tcs_need_output || ps_need_output) {
      for (unsigned i = 0; i < info->num_outputs; i++) {
               /* Only FS uses unpacked f16. Other stages pack 16-bit outputs into low and high bits of f32. */
   if (nir->info.stage == MESA_SHADER_FRAGMENT &&
                  for (unsigned j = 0; j < 4; j++) {
      ctx->abi.outputs[i * 4 + j] = ac_build_alloca_undef(&ctx->ac, type, "");
                     if (!ac_nir_translate(&ctx->ac, &ctx->abi, &ctx->args->ac, nir))
            switch (ctx->stage) {
   case MESA_SHADER_VERTEX:
      if (shader->key.ge.as_ls)
         else if (shader->key.ge.as_es)
               case MESA_SHADER_TESS_CTRL:
      if (!shader->is_monolithic)
               case MESA_SHADER_TESS_EVAL:
      if (ctx->shader->key.ge.as_es)
               case MESA_SHADER_GEOMETRY:
      if (!ctx->shader->key.ge.as_ngg)
               case MESA_SHADER_FRAGMENT:
      if (!shader->is_monolithic)
               default:
                           if (free_nir)
            }
      static bool si_should_optimize_less(struct ac_llvm_compiler *compiler,
         {
      if (!compiler->low_opt_passes)
            /* Assume a slow CPU. */
            /* For a crazy dEQP test containing 2597 memory opcodes, mostly
   * buffer stores. */
      }
      bool si_llvm_compile_shader(struct si_screen *sscreen, struct ac_llvm_compiler *compiler,
               {
      struct si_shader_selector *sel = shader->selector;
   struct si_shader_context ctx;
   enum ac_float_mode float_mode = nir->info.stage == MESA_SHADER_KERNEL ? AC_FLOAT_MODE_DEFAULT : AC_FLOAT_MODE_DEFAULT_OPENGL;
   bool exports_color_null = false;
            if (sel->stage == MESA_SHADER_FRAGMENT) {
      exports_color_null = sel->info.colors_written;
   exports_mrtz = sel->info.writes_z || sel->info.writes_stencil || shader->ps.writes_samplemask;
   if (!exports_mrtz && !exports_color_null)
               si_llvm_context_init(&ctx, sscreen, compiler, shader->wave_size, exports_color_null, exports_mrtz,
                  if (!si_llvm_translate_nir(&ctx, shader, nir, false)) {
      si_llvm_dispose(&ctx);
               /* For merged shader stage. */
   if (shader->is_monolithic && sscreen->info.gfx_level >= GFX9 &&
      (sel->stage == MESA_SHADER_TESS_CTRL || sel->stage == MESA_SHADER_GEOMETRY)) {
   /* LS or ES shader. */
            bool free_nir;
            struct ac_llvm_pointer parts[2];
            if (!si_llvm_translate_nir(&ctx, &prev_shader, nir, free_nir)) {
      si_llvm_dispose(&ctx);
                        /* Reset the shader context. */
   ctx.shader = shader;
            bool same_thread_count = shader->key.ge.opt.same_patch_vertices;
                        /* Make sure the input is a pointer and not integer followed by inttoptr. */
            /* Compile to bytecode. */
   if (!si_compile_llvm(sscreen, &shader->binary, &shader->config, compiler, &ctx.ac, debug,
                  si_llvm_dispose(&ctx);
   fprintf(stderr, "LLVM failed to compile shader\n");
               si_llvm_dispose(&ctx);
      }
      bool si_llvm_build_shader_part(struct si_screen *sscreen, gl_shader_stage stage,
                     {
               struct si_shader_selector sel = {};
            struct si_shader shader = {};
   shader.selector = &sel;
   bool wave32 = false;
   bool exports_color_null = false;
            switch (stage) {
   case MESA_SHADER_VERTEX:
      shader.key.ge.as_ls = key->vs_prolog.as_ls;
   shader.key.ge.as_es = key->vs_prolog.as_es;
   shader.key.ge.as_ngg = key->vs_prolog.as_ngg;
   wave32 = key->vs_prolog.wave32;
      case MESA_SHADER_TESS_CTRL:
      assert(!prolog);
   shader.key.ge.part.tcs.epilog = key->tcs_epilog.states;
   wave32 = key->tcs_epilog.wave32;
      case MESA_SHADER_FRAGMENT:
      if (prolog) {
      shader.key.ps.part.prolog = key->ps_prolog.states;
   wave32 = key->ps_prolog.wave32;
      } else {
      shader.key.ps.part.epilog = key->ps_epilog.states;
   wave32 = key->ps_epilog.wave32;
   exports_color_null = key->ps_epilog.colors_written;
   exports_mrtz = key->ps_epilog.writes_z || key->ps_epilog.writes_stencil ||
         if (!exports_mrtz && !exports_color_null)
      }
      default:
                  struct si_shader_context ctx;
   si_llvm_context_init(&ctx, sscreen, compiler, wave32 ? 32 : 64, exports_color_null, exports_mrtz,
            ctx.shader = &shader;
            struct si_shader_args args;
                     switch (stage) {
   case MESA_SHADER_VERTEX:
      build = si_llvm_build_vs_prolog;
      case MESA_SHADER_TESS_CTRL:
      build = si_llvm_build_tcs_epilog;
      case MESA_SHADER_FRAGMENT:
      build = prolog ? si_llvm_build_ps_prolog : si_llvm_build_ps_epilog;
      default:
                           /* Compile. */
            bool ret = si_compile_llvm(sscreen, &result->binary, &result->config, compiler,
            si_llvm_dispose(&ctx);
      }
