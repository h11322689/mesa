   /*
   * Copyright 2022 Google LLC
   * SPDX-License-Identifier: MIT
   */
      #include "ir3.h"
      /*
   * Mark (ul) on the last user of a0 before a0 is loaded again.  emit_block
   * makes sure a0 is loaded first if there is any user in the block.  This
   * allows us to process each block independently.
   *
   * Note that this must be called before passes that break the assumption, such
   * as ir3_lower_subgroups.
   */
      static bool
   is_reg_relative(const struct ir3_instruction *instr)
   {
      foreach_dst (reg, instr) {
      if (reg->flags & IR3_REG_RELATIV)
               foreach_src (reg, instr) {
      if (reg->flags & IR3_REG_RELATIV)
                  }
      static bool
   is_dst_a0(const struct ir3_instruction *instr)
   {
      foreach_dst (reg, instr) {
      if (reg->num == regid(REG_A0, 0))
                  }
      bool
   ir3_legalize_relative(struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list) {
               foreach_instr (instr, &block->instr_list) {
                     /* Is it valid to have address reg loaded from a relative src (ie.
   * mova a0, c<a0.x+4>)?  This marks the load (ul), which may or may
   * not be valid.
   */
   if (last_user && is_dst_a0(instr)) {
      last_user->flags |= IR3_INSTR_UL;
                  if (last_user)
                  }
