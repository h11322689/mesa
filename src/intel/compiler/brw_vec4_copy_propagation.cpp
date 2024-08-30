   /*
   * Copyright © 2011 Intel Corporation
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
      /**
   * @file brw_vec4_copy_propagation.cpp
   *
   * Implements tracking of values copied between registers, and
   * optimizations based on that: copy propagation and constant
   * propagation.
   */
      #include "brw_vec4.h"
   #include "brw_cfg.h"
   #include "brw_eu.h"
      namespace brw {
      struct copy_entry {
      src_reg *value[4];
      };
      static bool
   is_direct_copy(vec4_instruction *inst)
   {
      return (inst->opcode == BRW_OPCODE_MOV &&
   !inst->predicate &&
   inst->dst.file == VGRF &&
   inst->dst.offset % REG_SIZE == 0 &&
   !inst->dst.reladdr &&
   !inst->src[0].reladdr &&
   (inst->dst.type == inst->src[0].type ||
            }
      static bool
   is_dominated_by_previous_instruction(vec4_instruction *inst)
   {
      return (inst->opcode != BRW_OPCODE_DO &&
   inst->opcode != BRW_OPCODE_WHILE &&
   inst->opcode != BRW_OPCODE_ELSE &&
      }
      static bool
   is_channel_updated(vec4_instruction *inst, src_reg *values[4], int ch)
   {
               /* consider GRF only */
   assert(inst->dst.file == VGRF);
   if (!src || src->file != VGRF)
            return regions_overlap(*src, REG_SIZE, inst->dst, inst->size_written) &&
            }
      /**
   * Get the origin of a copy as a single register if all components present in
   * the given readmask originate from the same register and have compatible
   * regions, otherwise return a BAD_FILE register.
   */
   static src_reg
   get_copy_value(const copy_entry &entry, unsigned readmask)
   {
      unsigned swz[4] = {};
            for (unsigned i = 0; i < 4; i++) {
      if (readmask & (1 << i)) {
                        if (src.file == IMM) {
         } else {
      swz[i] = BRW_GET_SWZ(src.swizzle, i);
   /* Overwrite the original swizzle so the src_reg::equals call
   * below doesn't care about it, the correct swizzle will be
   * calculated once the swizzles of all components are known.
                     if (value.file == BAD_FILE) {
         } else if (!value.equals(src)) {
            } else {
                        return swizzle(value,
                  }
      static bool
   try_constant_propagate(vec4_instruction *inst,
         {
      /* For constant propagation, we only handle the same constant
   * across all 4 channels.  Some day, we should handle the 8-bit
   * float vector format, which would let us constant propagate
   * vectors better.
   * We could be more aggressive here -- some channels might not get used
   * based on the destination writemask.
   */
   src_reg value =
      get_copy_value(*entry,
               if (value.file != IMM)
            /* 64-bit types can't be used except for one-source instructions, which
   * higher levels should have constant folded away, so there's no point in
   * propagating immediates here.
   */
   if (type_sz(value.type) == 8 || type_sz(inst->src[arg].type) == 8)
            if (value.type == BRW_REGISTER_TYPE_VF) {
      /* The result of bit-casting the component values of a vector float
   * cannot in general be represented as an immediate.
   */
   if (inst->src[arg].type != BRW_REGISTER_TYPE_F)
      } else {
                  if (inst->src[arg].abs) {
      if (!brw_abs_immediate(value.type, &value.as_brw_reg()))
               if (inst->src[arg].negate) {
      if (!brw_negate_immediate(value.type, &value.as_brw_reg()))
                        switch (inst->opcode) {
   case BRW_OPCODE_MOV:
   case SHADER_OPCODE_BROADCAST:
      inst->src[arg] = value;
         case VEC4_OPCODE_UNTYPED_ATOMIC:
      if (arg == 1) {
      inst->src[arg] = value;
      }
         case SHADER_OPCODE_POW:
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_INT_REMAINDER:
         case BRW_OPCODE_DP2:
   case BRW_OPCODE_DP3:
   case BRW_OPCODE_DP4:
   case BRW_OPCODE_DPH:
   case BRW_OPCODE_BFI1:
   case BRW_OPCODE_ASR:
   case BRW_OPCODE_SHL:
   case BRW_OPCODE_SHR:
   case BRW_OPCODE_SUBB:
      if (arg == 1) {
      inst->src[arg] = value;
      }
         case BRW_OPCODE_MACH:
   case BRW_OPCODE_MUL:
   case SHADER_OPCODE_MULH:
   case BRW_OPCODE_ADD:
   case BRW_OPCODE_OR:
   case BRW_OPCODE_AND:
   case BRW_OPCODE_XOR:
   case BRW_OPCODE_ADDC:
      inst->src[arg] = value;
   return true;
         /* Fit this constant in by commuting the operands.  Exception: we
      * can't do this for 32-bit integer MUL/MACH because it's asymmetric.
      if ((inst->opcode == BRW_OPCODE_MUL ||
               (inst->src[1].type == BRW_REGISTER_TYPE_D ||
         inst->src[0] = inst->src[1];
   inst->src[1] = value;
   return true;
         }
      case GS_OPCODE_SET_WRITE_OFFSET:
      /* This is just a multiply by a constant with special strides.
   * The generator will handle immediates in both arguments (generating
   * a single MOV of the product).  So feel free to propagate in src0.
   */
   inst->src[arg] = value;
         case BRW_OPCODE_CMP:
      inst->src[arg] = value;
   return true;
         enum brw_conditional_mod new_cmod;
      new_cmod = brw_swap_cmod(inst->conditional_mod);
   if (new_cmod != BRW_CONDITIONAL_NONE) {
      /* Fit this constant in by swapping the operands and
      * flipping the test.
      inst->src[0] = inst->src[1];
   inst->src[1] = value;
   inst->conditional_mod = new_cmod;
      }
         }
         case BRW_OPCODE_SEL:
      inst->src[arg] = value;
   return true;
         inst->src[0] = inst->src[1];
   inst->src[1] = value;
      /* If this was predicated, flipping operands means
      * we also need to flip the predicate.
      if (inst->conditional_mod == BRW_CONDITIONAL_NONE) {
         }
   return true;
         }
         default:
                     }
      static bool
   is_align1_opcode(unsigned opcode)
   {
      switch (opcode) {
   case VEC4_OPCODE_DOUBLE_TO_F32:
   case VEC4_OPCODE_DOUBLE_TO_D32:
   case VEC4_OPCODE_DOUBLE_TO_U32:
   case VEC4_OPCODE_TO_DOUBLE:
   case VEC4_OPCODE_PICK_LOW_32BIT:
   case VEC4_OPCODE_PICK_HIGH_32BIT:
   case VEC4_OPCODE_SET_LOW_32BIT:
   case VEC4_OPCODE_SET_HIGH_32BIT:
         default:
            }
      static bool
   try_copy_propagate(const struct brw_compiler *compiler,
               {
               /* Build up the value we are propagating as if it were the source of a
   * single MOV
   */
   src_reg value =
      get_copy_value(*entry,
               /* Check that we can propagate that value */
   if (value.file != UNIFORM &&
      value.file != VGRF &&
   value.file != ATTR)
         /* Instructions that write 2 registers also need to read 2 registers. Make
   * sure we don't break that restriction by copy propagating from a uniform.
   */
   if (inst->size_written > REG_SIZE && is_uniform(value))
            /* There is a regioning restriction such that if execsize == width
   * and hstride != 0 then the vstride can't be 0. When we split instrutions
   * that take a single-precision source (like F->DF conversions) we end up
   * with a 4-wide source on an instruction with an execution size of 4.
   * If we then copy-propagate the source from a uniform we also end up with a
   * vstride of 0 and we violate the restriction.
   */
   if (inst->exec_size == 4 && value.file == UNIFORM &&
      type_sz(value.type) == 4)
         /* If the type of the copy value is different from the type of the
   * instruction then the swizzles and writemasks involved don't have the same
   * meaning and simply replacing the source would produce different semantics.
   */
   if (type_sz(value.type) != type_sz(inst->src[arg].type))
            if (inst->src[arg].offset % REG_SIZE || value.offset % REG_SIZE)
                     /* gfx6 math and gfx7+ SENDs from GRFs ignore source modifiers on
   * instructions.
   */
   if (has_source_modifiers && !inst->can_do_source_mods(devinfo))
            /* Reject cases that would violate register regioning restrictions. */
   if ((value.file == UNIFORM || value.swizzle != BRW_SWIZZLE_XYZW) &&
      ((devinfo->ver == 6 && inst->is_math()) ||
   inst->is_send_from_grf() ||
   inst->uses_indirect_addressing())) {
               if (has_source_modifiers &&
      value.type != inst->src[arg].type &&
   !inst->can_change_types())
         if (has_source_modifiers &&
      (inst->opcode == SHADER_OPCODE_GFX4_SCRATCH_WRITE ||
   inst->opcode == VEC4_OPCODE_PICK_HIGH_32BIT))
         unsigned composed_swizzle = brw_compose_swizzle(inst->src[arg].swizzle,
            /* Instructions that operate on vectors in ALIGN1 mode will ignore swizzles
   * so copy-propagation won't be safe if the composed swizzle is anything
   * other than the identity.
   */
   if (is_align1_opcode(inst->opcode) && composed_swizzle != BRW_SWIZZLE_XYZW)
            if (inst->is_3src(compiler) &&
      (value.file == UNIFORM ||
   (value.file == ATTR && attributes_per_reg != 1)) &&
   !brw_is_single_value_swizzle(composed_swizzle))
         if (inst->is_send_from_grf())
            /* we can't generally copy-propagate UD negations because we
   * end up accessing the resulting values as signed integers
   * instead. See also resolve_ud_negate().
   */
   if (value.negate &&
      value.type == BRW_REGISTER_TYPE_UD)
         /* Don't report progress if this is a noop. */
   if (value.equals(inst->src[arg]))
            const unsigned dst_saturate_mask = inst->dst.writemask &
            if (dst_saturate_mask) {
      /* We either saturate all or nothing. */
   if (dst_saturate_mask != inst->dst.writemask)
            /* Limit saturate propagation only to SEL with src1 bounded within 0.0
   * and 1.0, otherwise skip copy propagate altogether.
   */
   switch(inst->opcode) {
   case BRW_OPCODE_SEL:
      if (arg != 0 ||
      inst->src[0].type != BRW_REGISTER_TYPE_F ||
   inst->src[1].file != IMM ||
   inst->src[1].type != BRW_REGISTER_TYPE_F ||
   inst->src[1].f < 0.0 ||
   inst->src[1].f > 1.0) {
      }
   if (!inst->saturate)
            default:
                     /* Build the final value */
   if (inst->src[arg].abs) {
      value.negate = false;
      }
   if (inst->src[arg].negate)
            value.swizzle = composed_swizzle;
   if (has_source_modifiers &&
      value.type != inst->src[arg].type) {
   assert(inst->can_change_types());
   for (int i = 0; i < 3; i++) {
         }
      } else {
                  inst->src[arg] = value;
      }
      bool
   vec4_visitor::opt_copy_propagation(bool do_constant_prop)
   {
      /* If we are in dual instanced or single mode, then attributes are going
   * to be interleaved, so one register contains two attribute slots.
   */
   const int attributes_per_reg =
         bool progress = false;
                     foreach_block_and_inst(block, vec4_instruction, inst, cfg) {
      /* This pass only works on basic blocks.  If there's flow
   * control, throw out all our information and start from
   * scratch.
   *
   * This should really be fixed by using a structure like in
   * src/glsl/opt_copy_propagation.cpp to track available copies.
   */
   memset(&entries, 0, sizeof(entries));
   continue;
                  /* For each source arg, see if each component comes from a copy
   * from the same type file (IMM, VGRF, UNIFORM), and try
   * optimizing out access to the copy result
   */
   /* Copied values end up in GRFs, and we don't track reladdr
      * accesses.
      if (inst->src[i].file != VGRF ||
                           /* We only handle register-aligned single GRF copies. */
   if (inst->size_read(i) != REG_SIZE ||
                  const unsigned reg = (alloc.offsets[inst->src[i].nr] +
                  if (do_constant_prop && try_constant_propagate(inst, i, &entry))
         progress = true;
               /* Track available source registers. */
   const int reg =
            /* Update our destination's current channel values.  For a direct copy,
      * the value is the newly propagated source.  Otherwise, we don't know
   * the new value, so clear it.
      bool direct_copy = is_direct_copy(inst);
         for (int i = 0; i < 4; i++) {
      if (inst->dst.writemask & (1 << i)) {
               entries[reg].value[i] = direct_copy ? &inst->src[0] : NULL;
      }
      /* Clear the records for any registers whose current value came from
      * our destination's updated channels, as the two are no longer equal.
      if (inst->dst.reladdr)
         else {
      for (unsigned i = 0; i < alloc.total_size; i++) {
         if (is_channel_updated(inst, entries[i].value, j)) {
      entries[i].value[j] = NULL;
   entries[i].saturatemask &= ~(1 << j);
               }
                     if (progress)
      invalidate_analysis(DEPENDENCY_INSTRUCTION_DATA_FLOW |
            }
      } /* namespace brw */
