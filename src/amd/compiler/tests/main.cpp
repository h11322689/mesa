   /*
   * Copyright © 2020 Valve Corporation
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
   #include "aco_ir.h"
      #include <llvm-c/Target.h>
      #include "framework.h"
   #include <getopt.h>
   #include <map>
   #include <set>
   #include <stdarg.h>
   #include <stdio.h>
   #include <string.h>
   #include <string>
   #include <unistd.h>
   #include <vector>
      static const char* help_message =
      "Usage: %s [-h] [-l --list] [--no-check] [TEST [TEST ...]]\n"
   "\n"
   "Run ACO unit test(s). If TEST is not provided, all tests are run.\n"
   "\n"
   "positional arguments:\n"
   "  TEST        Run TEST. If TEST ends with a '.', run tests with names\n"
   "              starting with TEST. The test variant (after the '/') can\n"
   "              be omitted to run all variants\n"
   "\n"
   "optional arguments:\n"
   "  -h, --help  Show this help message and exit.\n"
   "  -l --list   List unit tests.\n"
         std::map<std::string, TestDef> tests;
   FILE* output = NULL;
      static TestDef current_test;
   static unsigned tests_written = 0;
   static FILE* checker_stdin = NULL;
   static char* checker_stdin_data = NULL;
   static size_t checker_stdin_size = 0;
      static char* output_data = NULL;
   static size_t output_size = 0;
   static size_t output_offset = 0;
      static char current_variant[64] = {0};
   static std::set<std::string>* variant_filter = NULL;
      bool test_failed = false;
   bool test_skipped = false;
   static char fail_message[256] = {0};
      void
   write_test()
   {
      if (!checker_stdin) {
      /* not entirely correct, but shouldn't matter */
   tests_written++;
               fflush(output);
   if (output_offset == output_size && !test_skipped && !test_failed)
            char* data = output_data + output_offset;
            fwrite("test", 1, 4, checker_stdin);
   fwrite(current_test.name, 1, strlen(current_test.name) + 1, checker_stdin);
   fwrite(current_variant, 1, strlen(current_variant) + 1, checker_stdin);
   fwrite(current_test.source_file, 1, strlen(current_test.source_file) + 1, checker_stdin);
   if (test_failed || test_skipped) {
      const char* res = test_failed ? "failed" : "skipped";
   fwrite("\x01", 1, 1, checker_stdin);
   fwrite(res, 1, strlen(res) + 1, checker_stdin);
      } else {
         }
   fwrite(&size, 4, 1, checker_stdin);
            tests_written++;
      }
      bool
   set_variant(const char* name)
   {
      if (variant_filter && !variant_filter->count(name))
            write_test();
   test_failed = false;
   test_skipped = false;
                        }
      void
   fail_test(const char* fmt, ...)
   {
      va_list args;
            test_failed = true;
               }
      void
   skip_test(const char* fmt, ...)
   {
      va_list args;
            test_skipped = true;
               }
      void
   run_test(TestDef def)
   {
      current_test = def;
   output_data = NULL;
   output_size = 0;
   output_offset = 0;
   test_failed = false;
   test_skipped = false;
            if (checker_stdin)
         else
            current_test.func();
            if (checker_stdin)
            }
      int
   check_output(char** argv)
   {
      fflush(stdout);
                     int stdin_pipe[2];
            pid_t child_pid = fork();
   if (child_pid == -1) {
      fprintf(stderr, "%s: fork() failed: %s\n", argv[0], strerror(errno));
      } else if (child_pid != 0) {
      /* Evaluate test output externally using Python */
   dup2(stdin_pipe[0], STDIN_FILENO);
   close(stdin_pipe[0]);
            execlp(ACO_TEST_PYTHON_BIN, ACO_TEST_PYTHON_BIN, ACO_TEST_SOURCE_DIR "/check_output.py",
         fprintf(stderr, "%s: execlp() failed: %s\n", argv[0], strerror(errno));
      } else {
      /* Feed input data to the Python process. Writing large streams to
   * stdin will block eventually, so this is done in a forked process
   * to let the test checker process chunks of data as they arrive */
   write(stdin_pipe[1], checker_stdin_data, checker_stdin_size);
   close(stdin_pipe[0]);
   close(stdin_pipe[1]);
         }
      bool
   match_test(std::string name, std::string pattern)
   {
      if (name.length() < pattern.length())
         if (pattern.back() == '.')
            }
      int
   main(int argc, char** argv)
   {
      int print_help = 0;
   int do_list = 0;
   int do_check = 1;
   const struct option opts[] = {{"help", no_argument, &print_help, 1},
                        int c;
   while ((c = getopt_long(argc, argv, "hl", opts, NULL)) != -1) {
      switch (c) {
   case 'h': print_help = 1; break;
   case 'l': do_list = 1; break;
   case 0: break;
   case '?':
   default: fprintf(stderr, "%s: Invalid argument\n", argv[0]); return 99;
               if (print_help) {
      fprintf(stderr, help_message, argv[0]);
               if (do_list) {
      for (auto test : tests)
                     std::vector<std::pair<std::string, std::string>> names;
   for (int i = optind; i < argc; i++) {
      std::string name = argv[i];
   std::string variant;
   size_t pos = name.find('/');
   if (pos != std::string::npos) {
      variant = name.substr(pos + 1);
      }
               if (do_check)
            LLVMInitializeAMDGPUTargetInfo();
   LLVMInitializeAMDGPUTarget();
   LLVMInitializeAMDGPUTargetMC();
                     for (auto pair : tests) {
      bool found = names.empty();
   bool all_variants = names.empty();
   std::set<std::string> variants;
   for (const std::pair<std::string, std::string>& name : names) {
      if (match_test(pair.first, name.first)) {
      found = true;
   if (name.second.empty())
         else
                  if (found) {
      variant_filter = all_variants ? NULL : &variants;
   printf("Running '%s'\n", pair.first.c_str());
         }
   if (!tests_written) {
      fprintf(stderr, "%s: No matching tests\n", argv[0]);
               if (checker_stdin) {
      printf("\n");
      } else {
      printf("Tests ran\n");
         }
