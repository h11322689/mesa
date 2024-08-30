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
      #include "bi_builder.h"
   #include "compiler.h"
      /* Bifrost v7 can preload up to two messages of the form:
   *
   * 1. +LD_VAR_IMM, register_format f32/f16, sample mode
   * 2. +VAR_TEX, register format f32/f16, sample mode (TODO)
   *
   * Analyze the shader for these instructions and push accordingly.
   */
      static bool
   bi_is_regfmt_float(enum bi_register_format regfmt)
   {
      return (regfmt == BI_REGISTER_FORMAT_F32) ||
      }
      /*
   * Preloaded varyings are interpolated at the sample location. Check if an
   * instruction can use this interpolation mode.
   */
   static bool
   bi_can_interp_at_sample(bi_instr *I)
   {
      /* .sample mode with r61 corresponds to per-sample interpolation */
   if (I->sample == BI_SAMPLE_SAMPLE)
            /* If the shader runs with pixel-frequency shading, .sample is
   * equivalent to .center, so allow .center
   *
   * If the shader runs with sample-frequency shading, .sample and .center
   * are not equivalent. However, the ESSL 3.20 specification
   * stipulates in section 4.5 ("Interpolation Qualifiers"):
   *
   *    for fragment shader input variables qualified with neither
   *    centroid nor sample, the value of the assigned variable may be
   *    interpolated anywhere within the pixel and a single value may be
   *    assigned to each sample within the pixel, to the extent permitted
   *    by the OpenGL ES Specification.
   *
   * We only produce .center for variables qualified with neither centroid
   * nor sample, so if .center is specified this section applies. This
   * suggests that, although per-pixel interpolation is allowed, it is not
   * mandated ("may" rather than "must" or "should"). Therefore it appears
   * safe to substitute sample.
   */
      }
      static bool
   bi_can_preload_ld_var(bi_instr *I)
   {
      return (I->op == BI_OPCODE_LD_VAR_IMM) && bi_can_interp_at_sample(I) &&
      }
      static bool
   bi_is_var_tex(enum bi_opcode op)
   {
         }
      void
   bi_opt_message_preload(bi_context *ctx)
   {
               /* We only preload from the first block */
   bi_block *block = bi_start_block(&ctx->blocks);
            bi_foreach_instr_in_block_safe(block, I) {
      if (I->nr_dests != 1)
                     if (bi_can_preload_ld_var(I)) {
      msg = (struct bifrost_message_preload){
      .enabled = true,
   .varying_index = I->varying_index,
   .fp16 = (I->register_format == BI_REGISTER_FORMAT_F16),
         } else if (bi_is_var_tex(I->op)) {
      msg = (struct bifrost_message_preload){
      .enabled = true,
   .texture = true,
   .varying_index = I->varying_index,
   .texture_index = I->texture_index,
   .fp16 = (I->op == BI_OPCODE_VAR_TEX_F16),
   .skip = I->skip,
         } else {
                  /* Report the preloading */
            /* Replace with a collect of preloaded registers. The collect
   * kills the moves, so the collect is free (it is coalesced).
   */
            unsigned nr = bi_count_write_registers(I, 0);
            /* The registers themselves must be preloaded at the start of
   * the program. Preloaded registers are coalesced, so these
   * moves are free.
   */
   b.cursor = bi_before_block(block);
   bi_foreach_src(collect, i) {
                                    /* Maximum number of preloaded messages */
   if ((++nr_preload) == 2)
         }
