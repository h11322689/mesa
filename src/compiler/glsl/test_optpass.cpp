   /*
   * Copyright © 2011 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * \file test_optpass.cpp
   *
   * Standalone test for optimization passes.
   *
   * This file provides the "optpass" command for the standalone
   * glsl_test app.  It accepts either GLSL or high-level IR as input,
   * and performs the optimiation passes specified on the command line.
   * It outputs the IR, both before and after optimiations.
   */
      #include <string>
   #include <iostream>
   #include <sstream>
   #include <getopt.h>
      #include "ast.h"
   #include "ir_optimization.h"
   #include "program.h"
   #include "ir_reader.h"
   #include "standalone_scaffolding.h"
   #include "main/mtypes.h"
      using namespace std;
      static string read_stdin_to_eof()
   {
      stringbuf sb;
   cin.get(sb, '\0');
      }
      static GLboolean
   do_optimization(struct exec_list *ir, const char *optimization,
         {
      int int_0;
   int int_1;
   int int_2;
            if (sscanf(optimization, "do_common_optimization ( %d ) ", &int_0) == 1) {
         } else if (strcmp(optimization, "do_algebraic") == 0) {
         } else if (strcmp(optimization, "do_dead_code") == 0) {
         } else if (strcmp(optimization, "do_dead_code_local") == 0) {
         } else if (strcmp(optimization, "do_dead_code_unlinked") == 0) {
         } else if (strcmp(optimization, "do_dead_functions") == 0) {
         } else if (strcmp(optimization, "do_function_inlining") == 0) {
         } else if (sscanf(optimization,
                  return do_lower_jumps(ir, int_0 != 0, int_1 != 0, int_2 != 0,
      } else if (strcmp(optimization, "do_if_simplification") == 0) {
         } else if (strcmp(optimization, "do_mat_op_to_vec") == 0) {
         } else if (strcmp(optimization, "do_tree_grafting") == 0) {
         } else if (strcmp(optimization, "do_vec_index_to_cond_assign") == 0) {
         } else if (sscanf(optimization, "lower_instructions ( %d ) ",
               } else {
      printf("Unrecognized optimization %s\n", optimization);
   exit(EXIT_FAILURE);
         }
      static GLboolean
   do_optimization_passes(struct exec_list *ir, char **optimizations,
               {
               for (int i = 0; i < num_optimizations; ++i) {
      const char *optimization = optimizations[i];
   if (!quiet) {
         }
   GLboolean progress = do_optimization(ir, optimization, options);
   if (!quiet) {
         }
                           }
      int test_optpass(int argc, char **argv)
   {
      int input_format_ir = 0; /* 0=glsl, 1=ir */
   int loop = 0;
   int shader_type = GL_VERTEX_SHADER;
   int quiet = 0;
            const struct option optpass_opts[] = {
      { "input-ir", no_argument, &input_format_ir, 1 },
   { "input-glsl", no_argument, &input_format_ir, 0 },
   { "loop", no_argument, &loop, 1 },
   { "vertex-shader", no_argument, &shader_type, GL_VERTEX_SHADER },
   { "fragment-shader", no_argument, &shader_type, GL_FRAGMENT_SHADER },
   { "quiet", no_argument, &quiet, 1 },
               int idx = 0;
   int c;
   while ((c = getopt_long(argc, argv, "", optpass_opts, &idx)) != -1) {
      if (c != 0) {
      printf("*** usage: %s optpass <optimizations> <options>\n", argv[0]);
   printf("\n");
   printf("Possible options are:\n");
   printf("  --input-ir: input format is IR\n");
   printf("  --input-glsl: input format is GLSL (the default)\n");
   printf("  --loop: run optimizations repeatedly until no progress\n");
   printf("  --vertex-shader: test with a vertex shader (the default)\n");
   printf("  --fragment-shader: test with a fragment shader\n");
                           struct gl_context local_ctx;
   struct gl_context *ctx = &local_ctx;
                     struct gl_shader *shader = rzalloc(NULL, struct gl_shader);
   shader->Type = shader_type;
                     struct _mesa_glsl_parse_state *state
            if (input_format_ir) {
      shader->ir = new(shader) exec_list;
   _mesa_glsl_initialize_types(state);
      } else {
      shader->Source = input.c_str();
   const char *source = shader->Source;
   state->error = glcpp_preprocess(state, &source, &state->info_log,
            if (!state->error) {
      _mesa_glsl_lexer_ctor(state, source);
   _mesa_glsl_parse(state);
               shader->ir = new(shader) exec_list;
   if (!state->error && !state->translation_unit.is_empty())
               /* Print out the initial IR */
   if (!state->error && !quiet) {
      printf("*** pre-optimization IR:\n");
   _mesa_print_ir(stdout, shader->ir, state);
               /* Optimization passes */
   if (!state->error) {
      GLboolean progress;
   const struct gl_shader_compiler_options *options =
         do {
      progress = do_optimization_passes(shader->ir, &argv[optind],
                  /* Print out the resulting IR */
   if (!state->error) {
      if (!quiet) {
         }
   _mesa_print_ir(stdout, shader->ir, state);
   if (!quiet) {
                     if (state->error) {
      printf("*** error(s) occurred:\n");
   printf("%s\n", state->info_log);
                        ralloc_free(state);
                        }
   