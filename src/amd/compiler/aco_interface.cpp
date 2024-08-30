   /*
   * Copyright © 2018 Google
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
   *
   */
      #include "aco_interface.h"
      #include "aco_ir.h"
      #include "util/memstream.h"
      #include <array>
   #include <iostream>
   #include <vector>
      static const std::array<aco_compiler_statistic_info, aco_num_statistics> statistic_infos = []()
   {
      std::array<aco_compiler_statistic_info, aco_num_statistics> ret{};
   ret[aco_statistic_hash] =
         ret[aco_statistic_instructions] =
         ret[aco_statistic_copies] =
         ret[aco_statistic_branches] = aco_compiler_statistic_info{"Branches", "Branch instructions"};
   ret[aco_statistic_latency] =
         ret[aco_statistic_inv_throughput] = aco_compiler_statistic_info{
         ret[aco_statistic_vmem_clauses] = aco_compiler_statistic_info{
         ret[aco_statistic_smem_clauses] = aco_compiler_statistic_info{
         ret[aco_statistic_sgpr_presched] =
         ret[aco_statistic_vgpr_presched] =
            }();
      const aco_compiler_statistic_info* aco_statistic_infos = statistic_infos.data();
      uint64_t
   aco_get_codegen_flags()
   {
      aco::init();
   /* Exclude flags which don't affect code generation. */
   uint64_t exclude = aco::DEBUG_VALIDATE_IR | aco::DEBUG_VALIDATE_RA | aco::DEBUG_PERFWARN |
            }
      static void
   validate(aco::Program* program)
   {
      if (!(aco::debug_flags & aco::DEBUG_VALIDATE_IR))
            ASSERTED bool is_valid = aco::validate_ir(program);
      }
      static std::string
   get_disasm_string(aco::Program* program, std::vector<uint32_t>& code, unsigned exec_size)
   {
               char* data = NULL;
   size_t disasm_size = 0;
   struct u_memstream mem;
   if (u_memstream_open(&mem, &data, &disasm_size)) {
      FILE* const memf = u_memstream_get(&mem);
   if (check_print_asm_support(program)) {
         } else {
   #ifndef LLVM_AVAILABLE
         #endif
                     }
   fputc(0, memf);
   u_memstream_close(&mem);
   disasm = std::string(data, data + disasm_size);
                  }
      static std::string
   aco_postprocess_shader(const struct aco_compiler_options* options,
         {
               if (options->dump_preoptir)
            ASSERTED bool is_valid = aco::validate_cfg(program.get());
            aco::live live_vars;
   if (!info->is_trap_handler_shader) {
      aco::dominator_tree(program.get());
   aco::lower_phis(program.get());
            /* Optimization */
   if (!options->optimisations_disabled) {
      if (!(aco::debug_flags & aco::DEBUG_NO_VN))
         if (!(aco::debug_flags & aco::DEBUG_NO_OPT))
               /* cleanup and exec mask handling */
   aco::setup_reduce_temp(program.get());
   aco::insert_exec_mask(program.get());
            /* spilling and scheduling */
   live_vars = aco::live_var_analysis(program.get());
               if (options->record_ir) {
      char* data = NULL;
   size_t size = 0;
   u_memstream mem;
   if (u_memstream_open(&mem, &data, &size)) {
      FILE* const memf = u_memstream_get(&mem);
   aco_print_program(program.get(), memf);
   fputc(0, memf);
               llvm_ir = std::string(data, data + size);
               if (program->collect_statistics)
            if ((aco::debug_flags & aco::DEBUG_LIVE_INFO) && options->dump_shader)
            if (!info->is_trap_handler_shader) {
      if (!options->optimisations_disabled && !(aco::debug_flags & aco::DEBUG_NO_SCHED))
                  /* Register Allocation */
            if (aco::validate_ra(program.get())) {
      aco_print_program(program.get(), stderr);
      } else if (options->dump_shader) {
                           /* Optimization */
   if (!options->optimisations_disabled && !(aco::debug_flags & aco::DEBUG_NO_OPT)) {
      aco::optimize_postRA(program.get());
                           /* Lower to HW Instructions */
   aco::lower_to_hw_instr(program.get());
            /* Insert Waitcnt */
   aco::insert_wait_states(program.get());
            if (program->gfx_level >= GFX10)
            if (program->collect_statistics || (aco::debug_flags & aco::DEBUG_PERF_INFO))
               }
      void
   aco_compile_shader(const struct aco_compiler_options* options, const struct aco_shader_info* info,
               {
               ac_shader_config config = {0};
            program->collect_statistics = options->record_stats;
   if (program->collect_statistics)
            program->debug.func = options->debug.func;
            /* Instruction Selection */
   if (info->is_trap_handler_shader)
         else
                     /* assembly */
   std::vector<uint32_t> code;
   std::vector<struct aco_symbol> symbols;
   /* OpenGL combine multi shader parts into one continous code block,
   * so only last part need the s_endpgm instruction.
   */
   bool append_endpgm = !(options->is_opengl && info->has_epilog);
            if (program->collect_statistics)
                     std::string disasm;
   if (get_disasm)
            size_t stats_size = 0;
   if (program->collect_statistics)
            (*build_binary)(binary, &config, llvm_ir.c_str(), llvm_ir.size(), disasm.c_str(), disasm.size(),
            }
      void
   aco_compile_rt_prolog(const struct aco_compiler_options* options,
                     {
               /* create program */
   ac_shader_config config = {0};
   std::unique_ptr<aco::Program> program{new aco::Program};
   program->collect_statistics = false;
   program->debug.func = NULL;
            aco::select_rt_prolog(program.get(), &config, options, info, in_args, out_args);
   validate(program.get());
   aco::insert_wait_states(program.get());
   aco::insert_NOPs(program.get());
   if (program->gfx_level >= GFX10)
            if (options->dump_shader)
            /* assembly */
   std::vector<uint32_t> code;
   code.reserve(align(program->blocks[0].instructions.size() * 2, 16));
                     std::string disasm;
   if (get_disasm)
            (*build_prolog)(binary, &config, NULL, 0, disasm.c_str(), disasm.size(), program->statistics, 0,
      }
      void
   aco_compile_vs_prolog(const struct aco_compiler_options* options,
                     {
               /* create program */
   ac_shader_config config = {0};
   std::unique_ptr<aco::Program> program{new aco::Program};
   program->collect_statistics = false;
   program->debug.func = NULL;
            /* create IR */
   aco::select_vs_prolog(program.get(), pinfo, &config, options, info, args);
   validate(program.get());
            if (options->dump_shader)
            /* assembly */
   std::vector<uint32_t> code;
   code.reserve(align(program->blocks[0].instructions.size() * 2, 16));
                     std::string disasm;
   if (get_disasm)
            (*build_prolog)(binary, config.num_sgprs, config.num_vgprs, code.data(), code.size(),
      }
      typedef void(select_shader_part_callback)(aco::Program* program, void* pinfo,
                              static void
   aco_compile_shader_part(const struct aco_compiler_options* options,
                           {
               ac_shader_config config = {0};
            program->collect_statistics = options->record_stats;
   if (program->collect_statistics)
            program->debug.func = options->debug.func;
                     /* Instruction selection */
                     /* assembly */
   std::vector<uint32_t> code;
   bool append_endpgm = !(options->is_opengl && is_prolog);
                     std::string disasm;
   if (get_disasm)
            (*build_binary)(binary, config.num_sgprs, config.num_vgprs, code.data(), code.size(),
      }
      void
   aco_compile_ps_epilog(const struct aco_compiler_options* options,
                     {
      aco_compile_shader_part(options, info, args, aco::select_ps_epilog, (void*)pinfo, build_epilog,
      }
      void
   aco_compile_tcs_epilog(const struct aco_compiler_options* options,
                     {
      aco_compile_shader_part(options, info, args, aco::select_tcs_epilog, (void*)pinfo, build_epilog,
      }
      void
   aco_compile_gl_vs_prolog(const struct aco_compiler_options* options,
                           {
      aco_compile_shader_part(options, info, args, aco::select_gl_vs_prolog, (void*)pinfo,
      }
      void
   aco_compile_ps_prolog(const struct aco_compiler_options* options,
                     {
      aco_compile_shader_part(options, info, args, aco::select_ps_prolog, (void*)pinfo, build_prolog,
      }
