   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "compiler/shader_enums.h"
   #include "nir/nir.h"
   #include "rogue.h"
   #include "util/macros.h"
   #include "util/os_file.h"
   #include "util/ralloc.h"
   #include "util/u_dynarray.h"
      #include <getopt.h>
   #include <stdbool.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
      /* Number of hex columns to print before starting a new line. */
   #define ARRAY_PRINT_COLS 16
      /**
   * \file vk_compiler.c
   *
   * \brief Rogue offline Vulkan shader compiler.
   */
      static const struct option cmdline_opts[] = {
      /* Arguments. */
   { "stage", required_argument, NULL, 's' },
   { "file", required_argument, NULL, 'f' },
            /* Options. */
   { "help", no_argument, NULL, 'h' },
               };
      typedef struct compiler_opts {
      gl_shader_stage stage;
   char *file;
   char *entry;
      } compiler_opts;
      static void usage(const char *argv0)
   {
      /* clang-format off */
   printf("Rogue offline Vulkan shader compiler.\n");
   printf("Usage: %s -s <stage> -f <file> [-e <entry>] [-o <file>] [-h]\n", argv0);
            printf("Required arguments:\n");
   printf("\t-s, --stage <stage> Shader stage (supported options: frag, vert).\n");
   printf("\t-f, --file <file>   Shader SPIR-V filename.\n");
            printf("Options:\n");
   printf("\t-h, --help          Prints this help message.\n");
   printf("\t-e, --entry <entry> Overrides the shader entry-point name (default: 'main').\n");
   printf("\t-o, --out <file>    Overrides the output filename (default: 'out.bin').\n");
   printf("\n");
      }
      static bool parse_cmdline(int argc, char *argv[], struct compiler_opts *opts)
   {
      int opt;
            while (
      (opt = getopt_long(argc, argv, "hs:f:e:o:", cmdline_opts, &longindex)) !=
   -1) {
   switch (opt) {
   case 'e':
                                 case 'f':
                                 case 'o':
                                 case 's':
                     if (!strcmp(optarg, "frag") || !strcmp(optarg, "f"))
         else if (!strcmp(optarg, "vert") || !strcmp(optarg, "v"))
         else {
      fprintf(stderr, "Unsupported stage \"%s\".\n", optarg);
   usage(argv[0]);
                     case 'h':
   default:
      usage(argv[0]);
                  if (opts->stage == MESA_SHADER_NONE || !opts->file) {
      fprintf(stderr,
         "%s: --stage and --file are required arguments.\n",
   usage(argv[0]);
               if (!opts->out_file)
            if (!opts->entry)
               }
      int main(int argc, char *argv[])
   {
      /* Command-line options. */
   /* N.B. MESA_SHADER_NONE != 0 */
            /* Input file data. */
   char *input_data;
            /* Compiler context. */
            /* Multi-stage build context. */
            /* Output file. */
   FILE *fp;
            /* Parse command-line options. */
   if (!parse_cmdline(argc, argv, &opts))
            /* Load SPIR-V input file. */
   input_data = os_read_file(opts.file, &input_size);
   if (!input_data) {
      fprintf(stderr, "Failed to read file \"%s\".\n", opts.file);
               /* Create compiler context. */
   compiler = rogue_compiler_create(NULL);
   if (!compiler) {
      fprintf(stderr, "Failed to set up compiler context.\n");
               /* Create build context. */
   ctx = rogue_build_context_create(compiler, NULL);
   if (!ctx) {
      fprintf(stderr, "Failed to set up build context.\n");
               /* SPIR-V -> NIR. */
   ctx->nir[opts.stage] = rogue_spirv_to_nir(ctx,
                                       if (!ctx->nir[opts.stage]) {
      fprintf(stderr, "Failed to translate SPIR-V input to NIR.\n");
               /* NIR -> Rogue. */
   ctx->rogue[opts.stage] = rogue_nir_to_rogue(ctx, ctx->nir[opts.stage]);
   if (!ctx->rogue[opts.stage]) {
      fprintf(stderr, "Failed to translate NIR input to Rogue.\n");
                        /* Write shader binary to disk. */
   fp = fopen(opts.out_file, "wb");
   if (!fp) {
      fprintf(stderr, "Failed to open output file \"%s\".\n", opts.out_file);
               bytes_written = fwrite(util_dynarray_begin(&ctx->binary[opts.stage]),
                     if (bytes_written != ctx->binary[opts.stage].size) {
      fprintf(
      stderr,
   "Failed to write to output file \"%s\" (%zu bytes of %u written).\n",
   opts.out_file,
   bytes_written,
                  /* Clean up. */
   fclose(fp);
   ralloc_free(ctx);
   ralloc_free(compiler);
                  err_close_outfile:
         err_free_build_context:
         err_destroy_compiler:
         err_free_input:
                  }
