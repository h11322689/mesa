   /*
   * Copyright (C) 2019 Collabora, Ltd.
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
   *
   * Authors (Collabora):
   *   Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   */
      #include "compiler.h"
      /* When we're 'squeezing down' the values in the IR, we maintain a hash
   * as such */
      static unsigned
   find_or_allocate_temp(compiler_context *ctx, struct hash_table_u64 *map,
         {
      if (hash >= SSA_FIXED_MINIMUM)
                     if (temp)
            /* If no temp is find, allocate one */
   temp = ctx->temp_count++;
                        }
      /* Reassigns numbering to get rid of gaps in the indices and to prioritize
   * smaller register classes */
      void
   mir_squeeze_index(compiler_context *ctx)
   {
               /* Reset */
            /* We need to prioritize texture registers on older GPUs so we don't
   * fail RA trying to assign to work registers r0/r1 when a work
            mir_foreach_instr_global(ctx, ins) {
      if (ins->type == TAG_TEXTURE_4)
               mir_foreach_instr_global(ctx, ins) {
      if (ins->type != TAG_TEXTURE_4)
            for (unsigned i = 0; i < ARRAY_SIZE(ins->src); ++i)
               ctx->blend_input = find_or_allocate_temp(ctx, map, ctx->blend_input);
               }
