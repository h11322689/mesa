   /*
   * Copyright © 2012 Intel Corporation
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
      /** @file brw_fs_register_coalesce.cpp
   *
   * Implements register coalescing: Checks if the two registers involved in a
   * raw move don't interfere, in which case they can both be stored in the same
   * place and the MOV removed.
   *
   * To do this, all uses of the source of the MOV in the shader are replaced
   * with the destination of the MOV. For example:
   *
   * add vgrf3:F, vgrf1:F, vgrf2:F
   * mov vgrf4:F, vgrf3:F
   * mul vgrf5:F, vgrf5:F, vgrf4:F
   *
   * becomes
   *
   * add vgrf4:F, vgrf1:F, vgrf2:F
   * mul vgrf5:F, vgrf5:F, vgrf4:F
   */
      #include "brw_fs.h"
   #include "brw_cfg.h"
   #include "brw_fs_live_variables.h"
      using namespace brw;
      static bool
   is_nop_mov(const fs_inst *inst)
   {
      if (inst->opcode == SHADER_OPCODE_LOAD_PAYLOAD) {
      fs_reg dst = inst->dst;
   for (int i = 0; i < inst->sources; i++) {
      if (!dst.equals(inst->src[i])) {
         }
   dst.offset += (i < inst->header_size ? REG_SIZE :
            }
      } else if (inst->opcode == BRW_OPCODE_MOV) {
                     }
      static bool
   is_coalesce_candidate(const fs_visitor *v, const fs_inst *inst)
   {
      if ((inst->opcode != BRW_OPCODE_MOV &&
      inst->opcode != SHADER_OPCODE_LOAD_PAYLOAD) ||
   inst->is_partial_write() ||
   inst->saturate ||
   inst->src[0].file != VGRF ||
   inst->src[0].negate ||
   inst->src[0].abs ||
   !inst->src[0].is_contiguous() ||
   inst->dst.file != VGRF ||
   inst->dst.type != inst->src[0].type) {
               if (v->alloc.sizes[inst->src[0].nr] >
      v->alloc.sizes[inst->dst.nr])
         if (inst->opcode == SHADER_OPCODE_LOAD_PAYLOAD) {
      if (!is_coalescing_payload(v->alloc, inst)) {
                        }
      static bool
   can_coalesce_vars(const fs_live_variables &live, const cfg_t *cfg,
               {
      if (!live.vars_interfere(src_var, dst_var))
            int dst_start = live.start[dst_var];
   int dst_end = live.end[dst_var];
   int src_start = live.start[src_var];
            /* Variables interfere and one line range isn't a subset of the other. */
   if ((dst_end > src_end && src_start < dst_start) ||
      (src_end > dst_end && dst_start < src_start))
         /* Check for a write to either register in the intersection of their live
   * ranges.
   */
   int start_ip = MAX2(dst_start, src_start);
            foreach_block(scan_block, cfg) {
      if (scan_block->end_ip < start_ip)
                     bool seen_src_write = false;
   bool seen_copy = false;
   foreach_inst_in_block(fs_inst, scan_inst, scan_block) {
               /* Ignore anything before the intersection of the live ranges */
                  /* Ignore the copying instruction itself */
   if (scan_inst == inst) {
      seen_copy = true;
                              if (seen_src_write && !seen_copy) {
      /* In order to satisfy the guarantee of register coalescing, we
   * must ensure that the two registers always have the same value
   * during the intersection of their live ranges.  One way to do
   * this is to simply ensure that neither is ever written apart
   * from the one copy which syncs up the two registers.  However,
   * this can be overly conservative and only works in the case
   * where the destination live range is entirely contained in the
   * source live range.
   *
   * To handle the other case where the source is contained in the
   * destination, we allow writes to the source register as long as
   * they happen before the copy, in the same block as the copy, and
   * the destination is never read between first such write and the
   * copy.  This effectively moves the write from the copy up.
   */
   for (int j = 0; j < scan_inst->sources; j++) {
      if (regions_overlap(scan_inst->src[j], scan_inst->size_read(j),
                        /* The MOV being coalesced had better be the only instruction which
   * writes to the coalesce destination in the intersection.
   */
   if (regions_overlap(scan_inst->dst, scan_inst->size_written,
                  /* See the big comment above */
   if (regions_overlap(scan_inst->dst, scan_inst->size_written,
            if (seen_copy || scan_block != block ||
      (scan_inst->force_writemask_all && !inst->force_writemask_all))
                           }
      bool
   fs_visitor::register_coalesce()
   {
      bool progress = false;
   fs_live_variables &live = live_analysis.require();
   int src_size = 0;
   int channels_remaining = 0;
   unsigned src_reg = ~0u, dst_reg = ~0u;
   int *dst_reg_offset = new int[MAX_VGRF_SIZE(devinfo)];
   fs_inst **mov = new fs_inst *[MAX_VGRF_SIZE(devinfo)];
   int *dst_var = new int[MAX_VGRF_SIZE(devinfo)];
            foreach_block_and_inst(block, fs_inst, inst, cfg) {
      if (!is_coalesce_candidate(this, inst))
            if (is_nop_mov(inst)) {
      inst->opcode = BRW_OPCODE_NOP;
   progress = true;
               if (src_reg != inst->src[0].nr) {
                                                         if (dst_reg != inst->dst.nr)
            if (inst->opcode == SHADER_OPCODE_LOAD_PAYLOAD) {
      for (int i = 0; i < src_size; i++) {
         }
   mov[0] = inst;
      } else {
      const int offset = inst->src[0].offset / REG_SIZE;
   if (mov[offset]) {
      /* This is the second time that this offset in the register has
   * been set.  This means, in particular, that inst->dst was
   * live before this instruction and that the live ranges of
   * inst->dst and inst->src[0] overlap and we can't coalesce the
   * two variables.  Let's ensure that doesn't happen.
   */
   channels_remaining = -1;
      }
   for (unsigned i = 0; i < MAX2(inst->size_written / REG_SIZE, 1); i++)
         mov[offset] = inst;
               if (channels_remaining)
            bool can_coalesce = true;
   for (int i = 0; i < src_size; i++) {
      if (dst_reg_offset[i] != dst_reg_offset[0] + i) {
      /* Registers are out-of-order. */
   can_coalesce = false;
   src_reg = ~0u;
                              if (!can_coalesce_vars(live, cfg, block, inst, dst_var[i], src_var[i])) {
      can_coalesce = false;
   src_reg = ~0u;
                  if (!can_coalesce)
                     for (int i = 0; i < src_size; i++) {
                     if (mov[i]->conditional_mod == BRW_CONDITIONAL_NONE) {
      mov[i]->opcode = BRW_OPCODE_NOP;
   mov[i]->dst = reg_undef;
   for (int j = 0; j < mov[i]->sources; j++) {
            } else {
      /* If we have a conditional modifier, rewrite the MOV to be a
   * MOV.cmod from the coalesced register.  Hopefully, cmod
   * propagation will clean this up and move it to the instruction
   * that writes the register.  If not, this keeps things correct
   * while still letting us coalesce.
   */
   assert(mov[i]->opcode == BRW_OPCODE_MOV);
   assert(mov[i]->sources == 1);
   mov[i]->src[0] = mov[i]->dst;
                  foreach_block_and_inst(block, fs_inst, scan_inst, cfg) {
      if (scan_inst->dst.file == VGRF &&
      scan_inst->dst.nr == src_reg) {
   scan_inst->dst.nr = dst_reg;
   scan_inst->dst.offset = scan_inst->dst.offset % REG_SIZE +
               for (int j = 0; j < scan_inst->sources; j++) {
      if (scan_inst->src[j].file == VGRF &&
      scan_inst->src[j].nr == src_reg) {
   scan_inst->src[j].nr = dst_reg;
   scan_inst->src[j].offset = scan_inst->src[j].offset % REG_SIZE +
                     for (int i = 0; i < src_size; i++) {
      live.start[dst_var[i]] = MIN2(live.start[dst_var[i]],
         live.end[dst_var[i]] = MAX2(live.end[dst_var[i]],
      }
               if (progress) {
      foreach_block_and_inst_safe (block, backend_instruction, inst, cfg) {
      if (inst->opcode == BRW_OPCODE_NOP) {
                                          delete[] src_var;
   delete[] dst_var;
   delete[] mov;
               }
