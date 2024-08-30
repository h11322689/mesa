   /*
   * Copyright © 2023 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_debug.h"
   #include <string.h>
   #include <stdlib.h>
      int main(int argc, char **argv)
   {
      if (argc < 3) {
      fprintf(stderr, "Usage: [LLVM processor] [IB filenames]\n");
               const char *gpu = argv[1];
   enum amd_gfx_level gfx_level;
            for (unsigned i = 0; i < CHIP_LAST; i++) {
      if (!strcmp(gpu, ac_get_llvm_processor_name(i))) {
      family = i;
   gfx_level = ac_get_gfx_level(i);
                  if (family == CHIP_UNKNOWN) {
      fprintf(stderr, "Unknown LLVM processor.\n");
               for (unsigned i = 2; i < argc; i++) {
               FILE *f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "Can't open IB: %s\n", filename);
               fseek(f, 0, SEEK_END);
   int size = ftell(f);
                     if (fread(ib, size, 1, f) != 1) {
      fprintf(stderr, "Can't read IB: %s\n", filename);
   fclose(f);
   free(ib);
      }
            ac_parse_ib(stdout, ib, size / 4, NULL, 0, filename, gfx_level, family, AMD_IP_GFX, NULL, NULL);
                  }
