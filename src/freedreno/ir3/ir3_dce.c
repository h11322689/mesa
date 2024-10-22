   /*
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "util/u_math.h"
      #include "ir3.h"
   #include "ir3_shader.h"
      /*
   * Dead code elimination:
   */
      static void
   mark_array_use(struct ir3_instruction *instr, struct ir3_register *reg)
   {
      if (reg->flags & IR3_REG_ARRAY) {
      struct ir3_array *arr =
               }
      static void
   instr_dce(struct ir3_instruction *instr, bool falsedep)
   {
      /* don't mark falsedep's as used, but otherwise process them normally: */
   if (!falsedep)
            if (ir3_instr_check_mark(instr))
            foreach_dst (dst, instr) {
      if (is_dest_gpr(dst))
               foreach_src (reg, instr)
            foreach_ssa_src_n (src, i, instr) {
            }
      static bool
   remove_unused_by_block(struct ir3_block *block)
   {
      bool progress = false;
   foreach_instr_safe (instr, &block->instr_list) {
      if (instr->opc == OPC_END || instr->opc == OPC_CHSH ||
      instr->opc == OPC_CHMASK || instr->opc == OPC_LOCK ||
   instr->opc == OPC_UNLOCK)
      if (instr->flags & IR3_INSTR_UNUSED) {
      if (instr->opc == OPC_META_SPLIT) {
      struct ir3_instruction *src = ssa(instr->srcs[0]);
   /* tex (cat5) instructions have a writemask, so we can
   * mask off unused components.  Other instructions do not.
   */
   if (src && is_tex_or_prefetch(src) && (src->dsts[0]->wrmask > 1)) {
                     /* prune false-deps, etc: */
   foreach_ssa_use (use, instr)
      foreach_ssa_srcp_n (srcp, n, use)
               list_delinit(&instr->node);
         }
      }
      static bool
   find_and_remove_unused(struct ir3 *ir, struct ir3_shader_variant *so)
   {
      unsigned i;
                     /* initially mark everything as unused, we'll clear the flag as we
   * visit the instructions:
   */
   foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      if (instr->opc == OPC_META_INPUT) {
      /* Without GS header geometry shader is never invoked. */
   if (instr->input.sysval == SYSTEM_VALUE_GS_HEADER_IR3)
                              foreach_array (arr, &ir->array_list)
            foreach_block (block, &ir->block_list) {
      for (i = 0; i < block->keeps_count; i++)
            /* We also need to account for if-condition: */
   if (block->condition)
               /* remove un-used instructions: */
   foreach_block (block, &ir->block_list) {
                  /* remove un-used arrays: */
   foreach_array_safe (arr, &ir->array_list) {
      if (arr->unused)
               /* fixup wrmask of split instructions to account for adjusted tex
   * wrmask's:
   */
   foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
                     struct ir3_instruction *src = ssa(instr->srcs[0]);
                                 for (i = 0; i < ir->a0_users_count; i++) {
      struct ir3_instruction *instr = ir->a0_users[i];
   if (instr && (instr->flags & IR3_INSTR_UNUSED))
               for (i = 0; i < ir->a1_users_count; i++) {
      struct ir3_instruction *instr = ir->a1_users[i];
   if (instr && (instr->flags & IR3_INSTR_UNUSED))
               for (i = 0; i < ir->predicates_count; i++) {
      struct ir3_instruction *instr = ir->predicates[i];
   if (instr && (instr->flags & IR3_INSTR_UNUSED))
               /* cleanup unused inputs: */
   foreach_input_n (in, n, ir)
      if (in->flags & IR3_INSTR_UNUSED)
            }
      bool
   ir3_dce(struct ir3 *ir, struct ir3_shader_variant *so)
   {
      void *mem_ctx = ralloc_context(NULL);
                     do {
      progress = find_and_remove_unused(ir, so);
                           }
