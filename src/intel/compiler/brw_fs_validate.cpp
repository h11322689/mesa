   /*
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
      /** @file brw_fs_validate.cpp
   *
   * Implements a pass that validates various invariants of the IR.  The current
   * pass only validates that GRF's uses are sane.  More can be added later.
   */
      #include "brw_fs.h"
   #include "brw_cfg.h"
      #define fsv_assert(assertion)                                           \
      {                                                                    \
      if (!(assertion)) {                                               \
      fprintf(stderr, "ASSERT: Scalar %s validation failed!\n",      \
         dump_instruction(inst, stderr);                                \
   fprintf(stderr, "%s:%d: '%s' failed\n", __FILE__, __LINE__, #assertion);  \
               #define fsv_assert_eq(first, second)                                    \
      {                                                                    \
      unsigned f = (first);                                             \
   unsigned s = (second);                                            \
   if (f != s) {                                                     \
      fprintf(stderr, "ASSERT: Scalar %s validation failed!\n",      \
         dump_instruction(inst, stderr);                                \
   fprintf(stderr, "%s:%d: A == B failed\n", __FILE__, __LINE__); \
   fprintf(stderr, "  A = %s = %u\n", #first, f);                 \
   fprintf(stderr, "  B = %s = %u\n", #second, s);                \
               #define fsv_assert_ne(first, second)                                    \
      {                                                                    \
      unsigned f = (first);                                             \
   unsigned s = (second);                                            \
   if (f == s) {                                                     \
      fprintf(stderr, "ASSERT: Scalar %s validation failed!\n",      \
         dump_instruction(inst, stderr);                                \
   fprintf(stderr, "%s:%d: A != B failed\n", __FILE__, __LINE__); \
   fprintf(stderr, "  A = %s = %u\n", #first, f);                 \
   fprintf(stderr, "  B = %s = %u\n", #second, s);                \
               #define fsv_assert_lte(first, second)                                   \
      {                                                                    \
      unsigned f = (first);                                             \
   unsigned s = (second);                                            \
   if (f > s) {                                                      \
      fprintf(stderr, "ASSERT: Scalar %s validation failed!\n",      \
         dump_instruction(inst, stderr);                                \
   fprintf(stderr, "%s:%d: A <= B failed\n", __FILE__, __LINE__); \
   fprintf(stderr, "  A = %s = %u\n", #first, f);                 \
   fprintf(stderr, "  B = %s = %u\n", #second, s);                \
               #ifndef NDEBUG
   void
   fs_visitor::validate()
   {
      foreach_block_and_inst (block, fs_inst, inst, cfg) {
      switch (inst->opcode) {
   case SHADER_OPCODE_SEND:
                  case BRW_OPCODE_MOV:
                  default:
                  if (inst->is_3src(compiler)) {
      const unsigned integer_sources =
      brw_reg_type_is_integer(inst->src[0].type) +
   brw_reg_type_is_integer(inst->src[1].type) +
      const unsigned float_sources =
      brw_reg_type_is_floating_point(inst->src[0].type) +
                              if (devinfo->ver >= 10) {
      for (unsigned i = 0; i < 3; i++) {
                     switch (inst->src[i].vstride) {
   case BRW_VERTICAL_STRIDE_0:
   case BRW_VERTICAL_STRIDE_4:
                                                                  default:
      fsv_assert(!"invalid vstride");
            } else if (grf_used != 0) {
      /* Only perform the pre-Gfx10 checks after register allocation has
   * occured.
   *
   * Many passes (e.g., constant copy propagation) will genenerate
   * invalid 3-source instructions with the expectation that later
   * passes (e.g., combine constants) will fix them.
   */
                     /* A stride of 1 (the usual case) or 0, with a special
   * "repctrl" bit, is allowed. The repctrl bit doesn't work for
   * 64-bit datatypes, so if the source type is 64-bit then only
   * a stride of 1 is allowed. From the Broadwell PRM, Volume 7
   * "3D Media GPGPU", page 944:
   *
   *    This is applicable to 32b datatypes and 16b datatype. 64b
                        if (type_sz(inst->src[i].type) > 4)
                     if (inst->dst.file == VGRF) {
      fsv_assert_lte(inst->dst.offset / REG_SIZE + regs_written(inst),
               for (unsigned i = 0; i < inst->sources; i++) {
      if (inst->src[i].file == VGRF) {
      fsv_assert_lte(inst->src[i].offset / REG_SIZE + regs_read(inst, i),
                  /* Accumulator Registers, bspec 47251:
   *
   * "When destination is accumulator with offset 0, destination
   * horizontal stride must be 1."
   */
   if (intel_needs_workaround(devinfo, 14014617373) &&
      inst->dst.is_accumulator() &&
   inst->dst.offset == 0) {
            }
   #endif
