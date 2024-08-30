   /*
   * Copyright (C) 2021 Collabora, Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <inttypes.h>
   #include <stdio.h>
   #include "disassemble.h"
      static inline uint8_t
   parse_nibble(const char c)
   {
         }
      /* Given a little endian 8 byte hexdump, parse out the 64-bit value */
   static uint64_t
   parse_hex(const char *in)
   {
               for (unsigned i = 0; i < 8; ++i) {
      uint8_t byte = (parse_nibble(in[0]) << 4) | parse_nibble(in[1]);
            /* Skip the space after the byte */
                  }
      int
   main(int argc, const char **argv)
   {
      if (argc < 2) {
      fprintf(stderr, "Expected case list\n");
                        if (fp == NULL) {
      fprintf(stderr, "Could not open the case list");
               char line[128];
            while (fgets(line, sizeof(line), fp) != NULL) {
      char *output = NULL;
   size_t sz = 0;
            /* Skip empty lines */
   if (len <= 1)
            /* Check for buffer overflow */
   if (len < 28) {
      fprintf(stderr, "Invalid reference %s\n", line);
               uint64_t bin = parse_hex(line);
   FILE *outputp = open_memstream(&output, &sz);
   va_disasm_instr(outputp, bin);
   fprintf(outputp, "\n");
            /* Skip hexdump: 8 bytes * (2 nibbles + 1 space) + 3 spaces */
   const char *reference = line + 27;
            if (fail) {
      /* Extra spaces after Got to align with Expected */
   printf("Got      %sExpected %s\n", output, reference);
      } else {
                              printf("Passed %u/%u tests.\n", nr_pass, nr_pass + nr_fail);
               }
