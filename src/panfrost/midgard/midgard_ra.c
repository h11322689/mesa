   /*
   * Copyright (C) 2018-2019 Alyssa Rosenzweig <alyssa@rosenzweig.io>
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
   */
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "compiler.h"
   #include "midgard_ops.h"
   #include "midgard_quirks.h"
      struct phys_reg {
      /* Physical register: 0-31 */
            /* Byte offset into the physical register: 0-15 */
            /* log2(bytes per component) for fast mul/div */
      };
      /* Shift up by reg_offset and horizontally by dst_offset. */
      static void
   offset_swizzle(unsigned *swizzle, unsigned reg_offset, unsigned srcshift,
         {
               signed reg_comp = reg_offset >> srcshift;
                     assert(reg_comp << srcshift == reg_offset);
            for (signed c = 0; c < MIR_VEC_COMPONENTS; ++c) {
      signed comp = MAX2(c - dst_comp, 0);
                  }
      /* Helper to return the default phys_reg for a given register */
      static struct phys_reg
   default_phys_reg(int reg, unsigned shift)
   {
      struct phys_reg r = {
      .reg = reg,
   .offset = 0,
                  }
      /* Determine which physical register, swizzle, and mask a virtual
   * register corresponds to */
      static struct phys_reg
   index_to_reg(compiler_context *ctx, struct lcra_state *l, unsigned reg,
         {
      /* Check for special cases */
   if (reg == ~0)
         else if (reg >= SSA_FIXED_MINIMUM)
         else if (!l)
            struct phys_reg r = {
      .reg = l->solutions[reg] / 16,
   .offset = l->solutions[reg] & 0xF,
                        if (r.reg < 16)
               }
      static void
   set_class(unsigned *classes, unsigned node, unsigned class)
   {
      if (node < SSA_FIXED_MINIMUM && class != classes[node]) {
      assert(classes[node] == REG_CLASS_WORK);
         }
      /* Special register classes impose special constraints on who can read their
   * values, so check that */
      static bool ASSERTED
   check_read_class(unsigned *classes, unsigned tag, unsigned node)
   {
      /* Non-nodes are implicitly ok */
   if (node >= SSA_FIXED_MINIMUM)
            switch (classes[node]) {
   case REG_CLASS_LDST:
         case REG_CLASS_TEXR:
         case REG_CLASS_TEXW:
         case REG_CLASS_WORK:
         default:
            }
      static bool ASSERTED
   check_write_class(unsigned *classes, unsigned tag, unsigned node)
   {
      /* Non-nodes are implicitly ok */
   if (node >= SSA_FIXED_MINIMUM)
            switch (classes[node]) {
   case REG_CLASS_TEXR:
         case REG_CLASS_TEXW:
         case REG_CLASS_LDST:
   case REG_CLASS_WORK:
         default:
            }
      /* Prepass before RA to ensure special class restrictions are met. The idea is
   * to create a bit field of types of instructions that read a particular index.
   * Later, we'll add moves as appropriate and rewrite to specialize by type. */
      static void
   mark_node_class(unsigned *bitfield, unsigned node)
   {
      if (node < SSA_FIXED_MINIMUM)
      }
      void
   mir_lower_special_reads(compiler_context *ctx)
   {
      mir_compute_temp_count(ctx);
            /* Bitfields for the various types of registers we could have. aluw can
            unsigned *alur = calloc(sz, 1);
   unsigned *aluw = calloc(sz, 1);
   unsigned *brar = calloc(sz, 1);
   unsigned *ldst = calloc(sz, 1);
   unsigned *texr = calloc(sz, 1);
                     mir_foreach_instr_global(ctx, ins) {
      switch (ins->type) {
   case TAG_ALU_4:
      mark_node_class(aluw, ins->dest);
   mark_node_class(alur, ins->src[0]);
                                       case TAG_LOAD_STORE_4:
      mark_node_class(aluw, ins->dest);
   mark_node_class(ldst, ins->src[0]);
   mark_node_class(ldst, ins->src[1]);
   mark_node_class(ldst, ins->src[2]);
               case TAG_TEXTURE_4:
      mark_node_class(texr, ins->src[0]);
   mark_node_class(texr, ins->src[1]);
   mark_node_class(texr, ins->src[2]);
               default:
                     /* Pass #2 is lowering now that we've analyzed all the classes.
   * Conceptually, if an index is only marked for a single type of use,
   * there is nothing to lower. If it is marked for different uses, we
   * split up based on the number of types of uses. To do so, we divide
   * into N distinct classes of use (where N>1 by definition), emit N-1
   * moves from the index to copies of the index, and finally rewrite N-1
                     for (unsigned i = 0; i < ctx->temp_count; ++i) {
      bool is_alur = BITSET_TEST(alur, i);
   bool is_aluw = BITSET_TEST(aluw, i);
   bool is_brar = BITSET_TEST(brar, i);
   bool is_ldst = BITSET_TEST(ldst, i);
   bool is_texr = BITSET_TEST(texr, i);
            /* Analyse to check how many distinct uses there are. ALU ops
   * (alur) can read the results of the texture pipeline (texw)
   * but not ldst or texr. Load/store ops (ldst) cannot read
   * anything but load/store inputs. Texture pipeline cannot read
            bool collision = (is_alur && (is_ldst || is_texr)) ||
                              if (!collision)
            /* Use the index as-is as the work copy. Emit copies for
            unsigned classes[] = {TAG_LOAD_STORE_4, TAG_TEXTURE_4, TAG_TEXTURE_4,
                  for (unsigned j = 0; j < ARRAY_SIZE(collisions); ++j) {
                     /* When the hazard is from reading, we move and rewrite
   * sources (typical case). When it's from writing, we
                                                   mir_foreach_block(ctx, block_) {
                     mir_foreach_instr_in_block_safe(block, pre_use) {
                                             midgard_instruction m = v_mov(idx, i);
                        midgard_instruction *use = mir_next_op(pre_use);
   assert(use);
   mir_insert_instruction_before(ctx, use, m);
                              unsigned mask = mir_from_bytemask(
                        if (mov == NULL || !mir_is_ssa(i)) {
      midgard_instruction m = v_mov(i, spill_idx++);
   m.mask = mask;
                                                   free(alur);
   free(aluw);
   free(brar);
   free(ldst);
   free(texr);
      }
      static void
   mir_compute_interference(compiler_context *ctx, struct lcra_state *l)
   {
      /* First, we need liveness information to be computed per block */
                     if (ctx->inputs->is_blend) {
               mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
   mir_foreach_instr_in_block_rev(block, ins) {
      if (ins->writeout)
               if (r1w != ~0)
               mir_foreach_instr_global(ctx, ins) {
      if (ins->dest < ctx->temp_count)
      lcra_add_node_interference(l, ins->dest, mir_bytemask(ins), r1w,
               /* Now that every block has live_in/live_out computed, we can determine
   * interference by walking each block linearly. Take live_out at the
            mir_foreach_block(ctx, _blk) {
               /* The scalar and vector units run in parallel. We need to make
   * sure they don't write to same portion of the register file
   * otherwise the result is undefined. Add interferences to
   * avoid this situation.
   */
   util_dynarray_foreach(&blk->bundles, midgard_bundle, bundle) {
                     for (unsigned i = 0; i < bundle->instruction_count; i++) {
      if (bundle->instructions[i]->unit == UNIT_VMUL ||
      bundle->instructions[i]->unit == UNIT_SADD)
      else
               for (unsigned i = 0; i < ARRAY_SIZE(instr_count); i++) {
                                                                        lcra_add_node_interference(l, ins_b->dest,
                              uint16_t *live =
            mir_foreach_instr_in_block_rev(blk, ins) {
                              if (dest < ctx->temp_count) {
      for (unsigned i = 0; i < ctx->temp_count; ++i) {
      if (live[i]) {
      unsigned mask = mir_bytemask(ins);
                     /* Add blend shader interference: blend shaders might
   * clobber r0-r3. */
   if (ins->compact_branch && ins->writeout) {
      for (unsigned i = 0; i < ctx->temp_count; ++i) {
                     for (unsigned j = 0; j < 4; j++) {
      lcra_add_node_interference(l, ctx->temp_count + j, 0xFFFF, i,
                     /* Update live_in */
                     }
      static bool
   mir_is_64(midgard_instruction *ins)
   {
      if (nir_alu_type_get_type_size(ins->dest_type) == 64)
            mir_foreach_src(ins, v) {
      if (nir_alu_type_get_type_size(ins->src_types[v]) == 64)
                  }
      /*
   * Determine if a shader needs a contiguous workgroup. This impacts register
   * allocation. TODO: Optimize if barriers and local memory are unused.
   */
   static bool
   needs_contiguous_workgroup(compiler_context *ctx)
   {
         }
      /*
   * Determine an upper-bound on the number of threads in a workgroup. The GL
   * driver reports 128 for the maximum number of threads (the minimum-maximum in
   * OpenGL ES 3.1), so we pessimistically assume 128 threads for variable
   * workgroups.
   */
   static unsigned
   max_threads_per_workgroup(compiler_context *ctx)
   {
      if (ctx->nir->info.workgroup_size_variable) {
         } else {
      return ctx->nir->info.workgroup_size[0] *
               }
      /*
   * Calculate the maximum number of work registers available to the shader.
   * Architecturally, Midgard shaders may address up to 16 work registers, but
   * various features impose other limits:
   *
   * 1. Blend shaders are limited to 8 registers by ABI.
   * 2. If there are more than 8 register-mapped uniforms, then additional
   *    register-mapped uniforms use space that otherwise would be used for work
   *    registers.
   * 3. If more than 4 registers are used, at most 128 threads may be spawned. If
   *    more than 8 registers are used, at most 64 threads may be spawned. These
   *    limits are architecturally visible in compute kernels that require an
   *    entire workgroup to be spawned at once (for barriers or local memory to
   *    work properly).
   */
   static unsigned
   max_work_registers(compiler_context *ctx)
   {
      if (ctx->inputs->is_blend)
            unsigned rmu_vec4 = ctx->info->push.count / 4;
            if (needs_contiguous_workgroup(ctx)) {
      unsigned threads = max_threads_per_workgroup(ctx);
            if (threads > 64)
                  }
      /* This routine performs the actual register allocation. It should be succeeded
   * by install_registers */
      static struct lcra_state *
   allocate_registers(compiler_context *ctx, bool *spilled)
   {
               /* No register allocation to do with no SSA */
   mir_compute_temp_count(ctx);
   if (!ctx->temp_count)
            /* Initialize LCRA. Allocate extra node at the end for r1-r3 for
            struct lcra_state *l = lcra_alloc_equations(ctx->temp_count + 4, 5);
            /* Starts of classes, in bytes */
   l->class_start[REG_CLASS_WORK] = 16 * 0;
   l->class_start[REG_CLASS_LDST] = 16 * 26;
   l->class_start[REG_CLASS_TEXR] = 16 * 28;
            l->class_size[REG_CLASS_WORK] = 16 * work_count;
   l->class_size[REG_CLASS_LDST] = 16 * 2;
   l->class_size[REG_CLASS_TEXR] = 16 * 2;
                     /* To save space on T*20, we don't have real texture registers.
   * Instead, tex inputs reuse the load/store pipeline registers, and
   * tex outputs use work r0/r1. Note we still use TEXR/TEXW classes,
            if (ctx->quirks & MIDGARD_INTERPIPE_REG_ALIASING) {
      l->class_start[REG_CLASS_TEXR] = l->class_start[REG_CLASS_LDST];
               unsigned *found_class = calloc(sizeof(unsigned), ctx->temp_count);
   unsigned *min_alignment = calloc(sizeof(unsigned), ctx->temp_count);
            mir_foreach_instr_global(ctx, ins) {
      /* Swizzles of 32-bit sources on 64-bit instructions need to be
   * aligned to either bottom (xy) or top (zw). More general
   * swizzle lowering should happen prior to scheduling (TODO),
   * but once we get RA we shouldn't disrupt this further. Align
            if (ins->type == TAG_ALU_4 && mir_is_64(ins)) {
                        if (s < ctx->temp_count)
                  if (ins->type == TAG_LOAD_STORE_4 && OP_HAS_ADDRESS(ins->op)) {
      mir_foreach_src(ins, v) {
                     if (s < ctx->temp_count)
                  /* Anything read as 16-bit needs proper alignment to ensure the
   * resulting code can be packed.
   */
   mir_foreach_src(ins, s) {
      unsigned src_size = nir_alu_type_get_type_size(ins->src_types[s]);
   if (src_size == 16 && ins->src[s] < SSA_FIXED_MINIMUM)
               /* Everything after this concerns only the destination, not the
   * sources.
   */
   if (ins->dest >= SSA_FIXED_MINIMUM)
                     if (ins->is_pack)
            /* 0 for x, 1 for xy, 2 for xyz, 3 for xyzw */
                     /* Use the largest class if there's ambiguity, this
            int dest = ins->dest;
            min_alignment[dest] =
      MAX2(min_alignment[dest], (size == 16) ? 1 : /* (1 << 1) = 2-byte */
                                 /* We can't cross xy/zw boundaries. TODO: vec8 can */
   if (size == 16 && min_alignment[dest] != 4)
            /* We don't have a swizzle for the conditional and we don't
   * want to muck with the conditional itself, so just force
            if (ins->type == TAG_ALU_4 && OP_IS_CSEL_V(ins->op)) {
               /* LCRA assumes bound >= alignment */
               /* Since ld/st swizzles and masks are 32-bit only, we need them
   * aligned to enable final packing */
   if (ins->type == TAG_LOAD_STORE_4)
               for (unsigned i = 0; i < ctx->temp_count; ++i) {
      lcra_set_alignment(l, i, min_alignment[i] ? min_alignment[i] : 2,
                     free(found_class);
   free(min_alignment);
            /* Next, we'll determine semantic class. We default to zero (work).
   * But, if we're used with a special operation, that will force us to a
   * particular class. Each node must be assigned to exactly one class; a
   * prepass before RA should have lowered what-would-have-been
   * multiclass nodes into a series of moves to break it up into multiple
            mir_foreach_instr_global(ctx, ins) {
               if (ins->type == TAG_LOAD_STORE_4) {
      set_class(l->class, ins->src[0], REG_CLASS_LDST);
   set_class(l->class, ins->src[1], REG_CLASS_LDST);
                  if (OP_IS_VEC4_ONLY(ins->op)) {
      lcra_restrict_range(l, ins->dest, 16);
   lcra_restrict_range(l, ins->src[0], 16);
   lcra_restrict_range(l, ins->src[1], 16);
   lcra_restrict_range(l, ins->src[2], 16);
         } else if (ins->type == TAG_TEXTURE_4) {
      set_class(l->class, ins->dest, REG_CLASS_TEXW);
   set_class(l->class, ins->src[0], REG_CLASS_TEXR);
   set_class(l->class, ins->src[1], REG_CLASS_TEXR);
   set_class(l->class, ins->src[2], REG_CLASS_TEXR);
                  /* Check that the semantics of the class are respected */
   mir_foreach_instr_global(ctx, ins) {
      assert(check_write_class(l->class, ins->type, ins->dest));
   assert(check_read_class(l->class, ins->type, ins->src[0]));
   assert(check_read_class(l->class, ins->type, ins->src[1]));
   assert(check_read_class(l->class, ins->type, ins->src[2]));
               /* Mark writeout to r0, depth to r1.x, stencil to r1.y,
   * render target to r1.z, unknown to r1.w */
   mir_foreach_instr_global(ctx, ins) {
      if (!(ins->compact_branch && ins->writeout))
            if (ins->src[0] < ctx->temp_count)
            if (ins->src[2] < ctx->temp_count)
            if (ins->src[3] < ctx->temp_count)
            if (ins->src[1] < ctx->temp_count)
            if (ins->dest < ctx->temp_count)
               /* Destinations of instructions in a writeout block cannot be assigned
   * to r1 unless they are actually used as r1 from the writeout itself,
   * since the writes to r1 are special. A code sequence like:
   *
   *      sadd.fmov r1.x, [...]
   *      vadd.fadd r0, r1, r2
   *      [writeout branch]
   *
   * will misbehave since the r1.x write will be interpreted as a
   * gl_FragDepth write so it won't show up correctly when r1 is read in
   * the following segment. We model this as interference.
            for (unsigned i = 0; i < 4; ++i)
            mir_foreach_block(ctx, _blk) {
               mir_foreach_bundle_in_block(blk, v) {
      /* We need at least a writeout and nonwriteout instruction */
                                                                                                         if (!used_as_r1)
      lcra_add_node_interference(l, ins->dest, mir_bytemask(ins),
                  /* Precolour blend input to r0. Note writeout is necessarily at the end
   * and blend shaders are single-RT only so there is only a single
   * writeout block, so this cannot conflict with the writeout r0 (there
            if (ctx->blend_input != ~0) {
      assert(ctx->blend_input < ctx->temp_count);
               /* Same for the dual-source blend input/output, except here we use r2,
            if (ctx->blend_src1 != ~0) {
      assert(ctx->blend_src1 < ctx->temp_count);
   l->solutions[ctx->blend_src1] = (16 * 2);
                        *spilled = !lcra_solve(l);
      }
      /* Once registers have been decided via register allocation
   * (allocate_registers), we need to rewrite the MIR to use registers instead of
   * indices */
      static void
   install_registers_instr(compiler_context *ctx, struct lcra_state *l,
         {
               for (unsigned i = 0; i < MIR_SRC_COUNT; ++i) {
      src_shift[i] =
               unsigned dest_shift =
            switch (ins->type) {
   case TAG_ALU_4:
   case TAG_ALU_8:
   case TAG_ALU_12:
   case TAG_ALU_16: {
      if (ins->compact_branch)
            struct phys_reg src1 = index_to_reg(ctx, l, ins->src[0], src_shift[0]);
   struct phys_reg src2 = index_to_reg(ctx, l, ins->src[1], src_shift[1]);
                     unsigned dest_offset =
            offset_swizzle(ins->swizzle[0], src1.offset, src1.shift, dest.shift,
         if (!ins->has_inline_constant)
      offset_swizzle(ins->swizzle[1], src2.offset, src2.shift, dest.shift,
      if (ins->src[0] != ~0)
         if (ins->src[1] != ~0)
         if (ins->dest != ~0)
                     case TAG_LOAD_STORE_4: {
      /* Which physical register we read off depends on
   * whether we are loading or storing -- think about the
                     if (encodes_src) {
                     ins->src[0] = SSA_FIXED_REGISTER(src.reg);
      } else {
               ins->dest = SSA_FIXED_REGISTER(dst.reg);
   offset_swizzle(ins->swizzle[0], 0, 2, dest_shift, dst.offset);
                        for (int i = 1; i <= 3; i++) {
      unsigned src_index = ins->src[i];
   if (src_index != ~0) {
      struct phys_reg src = index_to_reg(ctx, l, src_index, src_shift[i]);
   unsigned component = src.offset >> src.shift;
   assert(component << src.shift == src.offset);
   ins->src[i] = SSA_FIXED_REGISTER(src.reg);
                              case TAG_TEXTURE_4: {
      if (ins->op == midgard_tex_op_barrier)
            /* Grab RA results */
   struct phys_reg dest = index_to_reg(ctx, l, ins->dest, dest_shift);
   struct phys_reg coord = index_to_reg(ctx, l, ins->src[1], src_shift[1]);
   struct phys_reg lod = index_to_reg(ctx, l, ins->src[2], src_shift[2]);
            /* First, install the texture coordinate */
   if (ins->src[1] != ~0)
                  /* Next, install the destination */
   if (ins->dest != ~0)
         offset_swizzle(ins->swizzle[0], 0, 2, dest.shift,
                  /* If there is a register LOD/bias, use it */
   if (ins->src[2] != ~0) {
      assert(!(lod.offset & 3));
   ins->src[2] = SSA_FIXED_REGISTER(lod.reg);
               /* If there is an offset register, install it */
   if (ins->src[3] != ~0) {
      ins->src[3] = SSA_FIXED_REGISTER(offset.reg);
                           default:
            }
      static void
   install_registers(compiler_context *ctx, struct lcra_state *l)
   {
      mir_foreach_instr_global(ctx, ins)
      }
      /* If register allocation fails, find the best spill node */
      static signed
   mir_choose_spill_node(compiler_context *ctx, struct lcra_state *l)
   {
               mir_foreach_instr_global(ctx, ins) {
      if (ins->no_spill & (1 << l->spill_class)) {
               if (l->spill_class != REG_CLASS_WORK) {
      mir_foreach_src(ins, s)
                        }
      /* Once we've chosen a spill node, spill it */
      static void
   mir_spill_register(compiler_context *ctx, unsigned spill_node,
         {
      if (spill_class == REG_CLASS_WORK && ctx->inputs->is_blend)
                     /* We have a spill node, so check the class. Work registers
   * legitimately spill to TLS, but special registers just spill to work
            bool is_special = spill_class != REG_CLASS_WORK;
            /* Allocate TLS slot (maybe) */
            /* For special reads, figure out how many bytes we need */
            /* If multiple instructions write to this destination, we'll have to
   * fill from TLS before writing */
            mir_foreach_instr_global_safe(ctx, ins) {
      read_bytemask |= mir_bytemask_of_read_components(ins, spill_node);
   if (ins->dest == spill_node)
               /* For TLS, replace all stores to the spilled node. For
   * special reads, just keep as-is; the class will be demoted
            if (!is_special || is_special_w) {
      if (is_special_w)
            unsigned last_id = ~0;
   unsigned last_fill = ~0;
   unsigned last_spill_index = ~0;
            mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
   mir_foreach_instr_in_block_safe(block, ins) {
                     /* Note: it's important to match the mask of the spill
   * with the mask of the instruction whose destination
   * we're spilling, or otherwise we'll read invalid
                  if (is_special_w) {
      midgard_instruction st = v_mov(spill_node, spill_slot);
                                          } else {
                                                if (write_count > 1 && bytemask != 0xFFFF &&
      bundle != last_fill) {
   midgard_instruction read =
         mir_insert_instruction_before_scheduled(ctx, block, ins,
                                                   /* In the same bundle, reads of the destination
   * of the spilt instruction need to be direct */
   midgard_instruction *it = ins;
                                                   /* The spilt instruction will write to
   * a work register for `it` to read but
                                    if (last_id == bundle) {
      last_spill->mask |= write_mask;
   u_foreach_bit(c, write_mask)
      } else {
      midgard_instruction st =
                                                                                 if (!is_special)
                     /* Insert a load from TLS before the first consecutive
   * use of the node, rewriting to use spilled indices to
   * break up the live range. Or, for special, insert a
   * move. Ironically the latter *increases* register
   * pressure, but the two uses of the spilling mechanism
   * are somewhat orthogonal. (special spilling is to use
   * work registers to back special registers; TLS
            mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
   mir_foreach_instr_in_block(block, ins) {
      /* We can't rewrite the moves used to spill in the
   * first place. These moves are hinted. */
                  /* If we don't use the spilled value, nothing to do */
                                                            if (is_special) {
      /* Move */
   st = v_mov(spill_node, index);
      } else {
                                                         } else {
      /* Special writes already have their move spilled in */
                              if (!is_special)
                           mir_foreach_instr_global(ctx, ins) {
            }
      static void
   mir_demote_uniforms(compiler_context *ctx, unsigned new_cutoff)
   {
      unsigned uniforms = ctx->info->push.count / 4;
   unsigned old_work_count = 16 - MAX2(uniforms - 8, 0);
            unsigned min_demote = SSA_FIXED_REGISTER(old_work_count);
            mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
   mir_foreach_instr_in_block(block, ins) {
      mir_foreach_src(ins, i) {
                              unsigned temp = make_compiler_temp(ctx);
                           midgard_instruction ld = {
      .type = TAG_LOAD_STORE_4,
   .mask = 0xF,
   .dest = temp,
   .dest_type = ins->src_types[i],
   .src = {~0, ~0, ~0, ~0},
   .swizzle = SWIZZLE_IDENTITY_4,
   .op = midgard_op_ld_ubo_128,
   .load_store =
      {
                                                                     }
      /* Run register allocation in a loop, spilling until we succeed */
      void
   mir_ra(compiler_context *ctx)
   {
      struct lcra_state *l = NULL;
   bool spilled = false;
            /* Number of 128-bit slots in memory we've spilled into */
                     do {
      if (spilled) {
                     /* It's a lot cheaper to demote uniforms to get more
   * work registers than to spill to TLS. */
   if (l->spill_class == REG_CLASS_WORK && uniforms > 8) {
         } else if (spill_node == -1) {
      fprintf(stderr, "ERROR: Failed to choose spill node\n");
   lcra_free(l);
      } else {
                     mir_squeeze_index(ctx);
            if (l) {
      lcra_free(l);
                           if (iter_count <= 0) {
      fprintf(
      stderr,
                  /* Report spilling information. spill_count is in 128-bit slots (vec4 x
                                 }
