   /*
   * Copyright 2014 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
   /* based on pieces from si_pipe.c and radeon_llvm_emit.c */
   #include "ac_llvm_util.h"
      #include "ac_llvm_build.h"
   #include "c11/threads.h"
   #include "util/bitscan.h"
   #include "util/u_math.h"
   #include <llvm-c/Core.h>
   #include <llvm-c/Support.h>
      #include <assert.h>
   #include <stdio.h>
   #include <string.h>
      static void ac_init_llvm_target(void)
   {
      LLVMInitializeAMDGPUTargetInfo();
   LLVMInitializeAMDGPUTarget();
   LLVMInitializeAMDGPUTargetMC();
            /* For inline assembly. */
            /* For ACO disassembly. */
            const char *argv[] = {
      /* error messages prefix */
   "mesa",
               ac_reset_llvm_all_options_occurrences();
               }
      PUBLIC void ac_init_shared_llvm_once(void)
   {
      static once_flag ac_init_llvm_target_once_flag = ONCE_FLAG_INIT;
      }
      #if !LLVM_IS_SHARED
   static once_flag ac_init_static_llvm_target_once_flag = ONCE_FLAG_INIT;
   static void ac_init_static_llvm_once(void)
   {
         }
   #endif
      void ac_init_llvm_once(void)
   {
   #if LLVM_IS_SHARED
         #else
         #endif
   }
      LLVMTargetRef ac_get_llvm_target(const char *triple)
   {
      LLVMTargetRef target = NULL;
            if (LLVMGetTargetFromTriple(triple, &target, &err_message)) {
      fprintf(stderr, "Cannot find target for triple %s ", triple);
   if (err_message) {
         }
   LLVMDisposeMessage(err_message);
      }
      }
      static LLVMTargetMachineRef ac_create_target_machine(enum radeon_family family,
                     {
      assert(family >= CHIP_TAHITI);
   const char *triple = (tm_options & AC_TM_SUPPORTS_SPILL) ? "amdgcn-mesa-mesa3d" : "amdgcn--";
   LLVMTargetRef target = ac_get_llvm_target(triple);
            LLVMTargetMachineRef tm =
      LLVMCreateTargetMachine(target, triple, name, "", level,
         if (!ac_is_llvm_processor_supported(tm, name)) {
      LLVMDisposeTargetMachine(tm);
   fprintf(stderr, "amd: LLVM doesn't support %s, bailing out...\n", name);
               if (out_triple)
               }
      LLVMAttributeRef ac_get_llvm_attribute(LLVMContextRef ctx, const char *str)
   {
         }
      void ac_add_function_attr(LLVMContextRef ctx, LLVMValueRef function, int attr_idx,
         {
      assert(LLVMIsAFunction(function));
      }
      void ac_dump_module(LLVMModuleRef module)
   {
      char *str = LLVMPrintModuleToString(module);
   fprintf(stderr, "%s", str);
      }
      void ac_llvm_add_target_dep_function_attr(LLVMValueRef F, const char *name, unsigned value)
   {
               snprintf(str, sizeof(str), "0x%x", value);
      }
      void ac_llvm_set_workgroup_size(LLVMValueRef F, unsigned size)
   {
      if (!size)
            char str[32];
   snprintf(str, sizeof(str), "%u,%u", size, size);
      }
      void ac_llvm_set_target_features(LLVMValueRef F, struct ac_llvm_context *ctx, bool wgp_mode)
   {
               snprintf(features, sizeof(features), "+DumpCode%s%s%s",
            /* GFX9 has broken VGPR indexing, so always promote alloca to scratch. */
   ctx->gfx_level == GFX9 ? ",-promote-alloca" : "",
   /* Wave32 is the default. */
   ctx->gfx_level >= GFX10 && ctx->wave_size == 64 ?
            }
      bool ac_init_llvm_compiler(struct ac_llvm_compiler *compiler, enum radeon_family family,
         {
      const char *triple;
            compiler->tm = ac_create_target_machine(family, tm_options, LLVMCodeGenLevelDefault, &triple);
   if (!compiler->tm)
            if (tm_options & AC_TM_CREATE_LOW_OPT) {
      compiler->low_opt_tm =
         if (!compiler->low_opt_tm)
               compiler->target_library_info = ac_create_target_library_info(triple);
   if (!compiler->target_library_info)
            compiler->passmgr =
         if (!compiler->passmgr)
               fail:
      ac_destroy_llvm_compiler(compiler);
      }
      void ac_destroy_llvm_compiler(struct ac_llvm_compiler *compiler)
   {
      ac_destroy_llvm_passes(compiler->passes);
            if (compiler->passmgr)
         if (compiler->target_library_info)
         if (compiler->low_opt_tm)
         if (compiler->tm)
      }
