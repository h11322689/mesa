   /*
   * Copyright (C) 2020 Collabora Ltd.
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
   *      Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   */
      #include "bi_builder.h"
   #include "compiler.h"
      /* Arguments common to worklist, passed by value for convenience */
      struct bi_worklist {
      /* # of instructions in the block */
            /* Instructions in the block */
            /* Bitset of instructions in the block ready for scheduling */
            /* The backwards dependency graph. nr_dependencies is the number of
   * unscheduled instructions that must still be scheduled after (before)
   * this instruction. dependents are which instructions need to be
   * scheduled before (after) this instruction. */
   unsigned *dep_counts;
      };
      /* State of a single tuple and clause under construction */
      struct bi_reg_state {
      /* Number of register writes */
            /* Register reads, expressed as (equivalence classes of)
   * sources. Only 3 reads are allowed, but up to 2 may spill as
   * "forced" for the next scheduled tuple, provided such a tuple
   * can be constructed */
   bi_index reads[5];
            /* The previous tuple scheduled (= the next tuple executed in the
   * program) may require certain writes, in order to bypass the register
   * file and use a temporary passthrough for the value. Up to 2 such
   * constraints are architecturally satisfiable */
   unsigned forced_count;
      };
      struct bi_tuple_state {
      /* Is this the last tuple in the clause */
            /* Scheduled ADD instruction, or null if none */
            /* Reads for previous (succeeding) tuple */
   bi_index prev_reads[5];
   unsigned nr_prev_reads;
            /* Register slot state for current tuple */
            /* Constants are shared in the tuple. If constant_count is nonzero, it
   * is a size for constant count. Otherwise, fau is the slot read from
   * FAU, or zero if none is assigned. Ordinarily FAU slot 0 reads zero,
   * but within a tuple, that should be encoded as constant_count != 0
   * and constants[0] = constants[1] = 0 */
            union {
      uint32_t constants[2];
                  };
      struct bi_const_state {
      unsigned constant_count;
   bool pcrel; /* applies to first const */
            /* Index of the constant into the clause */
      };
      enum bi_ftz_state {
      /* No flush-to-zero state assigned yet */
            /* Never flush-to-zero */
            /* Always flush-to-zero */
      };
      /* At this point, pseudoinstructions have been lowered so sources/destinations
   * are limited to what's physically supported.
   */
   #define BI_MAX_PHYS_SRCS  4
   #define BI_MAX_PHYS_DESTS 2
      struct bi_clause_state {
      /* Has a message-passing instruction already been assigned? */
            /* Indices already accessed, this needs to be tracked to avoid hazards
   * around message-passing instructions */
   unsigned access_count;
            unsigned tuple_count;
            /* Numerical state of the clause */
      };
      /* Determines messsage type by checking the table and a few special cases. Only
   * case missing is tilebuffer instructions that access depth/stencil, which
   * require a Z_STENCIL message (to implement
   * ARM_shader_framebuffer_fetch_depth_stencil) */
      static enum bifrost_message_type
   bi_message_type_for_instr(bi_instr *ins)
   {
      enum bifrost_message_type msg = bi_opcode_props[ins->op].message;
            if (ld_var_special && ins->varying_name == BI_VARYING_NAME_FRAG_Z)
            if (msg == BIFROST_MESSAGE_LOAD && ins->seg == BI_SEG_UBO)
               }
      /* Attribute, texture, and UBO load (attribute message) instructions support
   * bindless, so just check the message type */
      ASSERTED static bool
   bi_supports_dtsel(bi_instr *ins)
   {
      switch (bi_message_type_for_instr(ins)) {
   case BIFROST_MESSAGE_ATTRIBUTE:
         case BIFROST_MESSAGE_TEX:
         default:
            }
      /* Adds an edge to the dependency graph */
      static void
   bi_push_dependency(unsigned parent, unsigned child, BITSET_WORD **dependents,
         {
      if (!BITSET_TEST(dependents[parent], child)) {
      BITSET_SET(dependents[parent], child);
         }
      static void
   add_dependency(struct util_dynarray *table, unsigned index, unsigned child,
         {
      assert(index < 64);
   util_dynarray_foreach(table + index, unsigned, parent)
      }
      static void
   mark_access(struct util_dynarray *table, unsigned index, unsigned parent)
   {
      assert(index < 64);
      }
      static bool
   bi_is_sched_barrier(bi_instr *I)
   {
      switch (I->op) {
   case BI_OPCODE_BARRIER:
   case BI_OPCODE_DISCARD_F32:
         default:
            }
      static void
   bi_create_dependency_graph(struct bi_worklist st, bool inorder, bool is_blend)
   {
               for (unsigned i = 0; i < 64; ++i) {
      util_dynarray_init(&last_read[i], NULL);
               /* Initialize dependency graph */
   for (unsigned i = 0; i < st.count; ++i) {
                                    /* Populate dependency graph */
   for (signed i = st.count - 1; i >= 0; --i) {
               bi_foreach_src(ins, s) {
      if (ins->src[s].type != BI_INDEX_REGISTER)
                  for (unsigned c = 0; c < count; ++c)
      add_dependency(last_write, ins->src[s].value + c, i, st.dependents,
            /* Keep message-passing ops in order. (This pass only cares
   * about bundling; reordering of message-passing instructions
            if (bi_message_type_for_instr(ins)) {
                                 /* Handle schedule barriers, adding All the deps */
   if (inorder || bi_is_sched_barrier(ins)) {
      for (unsigned j = 0; j < st.count; ++j) {
                     bi_push_dependency(MAX2(i, j), MIN2(i, j), st.dependents,
                  bi_foreach_dest(ins, d) {
                              for (unsigned c = 0; c < count; ++c) {
      add_dependency(last_read, dest + c, i, st.dependents,
         add_dependency(last_write, dest + c, i, st.dependents,
                        /* Blend shaders are allowed to clobber R0-R15. Treat these
   * registers like extra destinations for scheduling purposes.
   */
   if (ins->op == BI_OPCODE_BLEND && !is_blend) {
      for (unsigned c = 0; c < 16; ++c) {
      add_dependency(last_read, c, i, st.dependents, st.dep_counts);
   add_dependency(last_write, c, i, st.dependents, st.dep_counts);
                  bi_foreach_src(ins, s) {
                              for (unsigned c = 0; c < count; ++c)
                  /* If there is a branch, all instructions depend on it, as interblock
            bi_instr *last = st.instructions[st.count - 1];
   if (last->branch_target || last->op == BI_OPCODE_JUMP) {
      for (signed i = st.count - 2; i >= 0; --i)
               /* Free the intermediate structures */
   for (unsigned i = 0; i < 64; ++i) {
      util_dynarray_fini(&last_read[i]);
         }
      /* Scheduler pseudoinstruction lowerings to enable instruction pairings.
   * Currently only support CUBEFACE -> *CUBEFACE1/+CUBEFACE2
   */
      static bi_instr *
   bi_lower_cubeface(bi_context *ctx, struct bi_clause_state *clause,
         {
      bi_instr *pinstr = tuple->add;
   bi_builder b = bi_init_builder(ctx, bi_before_instr(pinstr));
   bi_instr *cubeface1 = bi_cubeface1_to(&b, pinstr->dest[0], pinstr->src[0],
            pinstr->op = BI_OPCODE_CUBEFACE2;
   pinstr->dest[0] = pinstr->dest[1];
            pinstr->src[0] = cubeface1->dest[0];
               }
      /* Psuedo arguments are (rbase, address lo, address hi). We need *ATOM_C.i32 to
   * have the arguments (address lo, address hi, rbase), and +ATOM_CX to have the
   * arguments (rbase, address lo, address hi, rbase) */
      static bi_instr *
   bi_lower_atom_c(bi_context *ctx, struct bi_clause_state *clause,
         {
      bi_instr *pinstr = tuple->add;
   bi_builder b = bi_init_builder(ctx, bi_before_instr(pinstr));
   bi_instr *atom_c = bi_atom_c_return_i32(&b, pinstr->src[1], pinstr->src[2],
            if (bi_is_null(pinstr->dest[0]))
            bi_instr *atom_cx =
      bi_atom_cx_to(&b, pinstr->dest[0], pinstr->src[0], pinstr->src[1],
      tuple->add = atom_cx;
               }
      static bi_instr *
   bi_lower_atom_c1(bi_context *ctx, struct bi_clause_state *clause,
         {
      bi_instr *pinstr = tuple->add;
   bi_builder b = bi_init_builder(ctx, bi_before_instr(pinstr));
   bi_instr *atom_c = bi_atom_c1_return_i32(&b, pinstr->src[0], pinstr->src[1],
            if (bi_is_null(pinstr->dest[0]))
            bi_instr *atom_cx =
      bi_atom_cx_to(&b, pinstr->dest[0], bi_null(), pinstr->src[0],
      tuple->add = atom_cx;
               }
      static bi_instr *
   bi_lower_seg_add(bi_context *ctx, struct bi_clause_state *clause,
         {
      bi_instr *pinstr = tuple->add;
            bi_instr *fma = bi_seg_add_to(&b, pinstr->dest[0], pinstr->src[0],
            pinstr->op = BI_OPCODE_SEG_ADD;
   pinstr->src[0] = pinstr->src[1];
            assert(pinstr->dest[0].type == BI_INDEX_REGISTER);
               }
      static bi_instr *
   bi_lower_dtsel(bi_context *ctx, struct bi_clause_state *clause,
         {
      bi_instr *add = tuple->add;
            bi_instr *dtsel =
         assert(add->nr_srcs >= 1);
            assert(bi_supports_dtsel(add));
      }
      /* Flatten linked list to array for O(1) indexing */
      static bi_instr **
   bi_flatten_block(bi_block *block, unsigned *len)
   {
      if (list_is_empty(&block->instructions))
            *len = list_length(&block->instructions);
                     bi_foreach_instr_in_block(block, ins)
               }
      /* The worklist would track instructions without outstanding dependencies. For
   * debug, force in-order scheduling (no dependency graph is constructed).
   */
      static struct bi_worklist
   bi_initialize_worklist(bi_block *block, bool inorder, bool is_blend)
   {
      struct bi_worklist st = {};
            if (!st.count)
            st.dependents = calloc(st.count, sizeof(st.dependents[0]));
            bi_create_dependency_graph(st, inorder, is_blend);
            for (unsigned i = 0; i < st.count; ++i) {
      if (st.dep_counts[i] == 0)
                  }
      static void
   bi_free_worklist(struct bi_worklist st)
   {
      free(st.dep_counts);
   free(st.dependents);
   free(st.instructions);
      }
      static void
   bi_update_worklist(struct bi_worklist st, unsigned idx)
   {
               if (!st.dependents[idx])
            /* Iterate each dependent to remove one dependency (`done`),
            unsigned i;
   BITSET_FOREACH_SET(i, st.dependents[idx], st.count) {
      assert(st.dep_counts[i] != 0);
            if (new_deps == 0)
                  }
      /* Scheduler predicates */
      /* IADDC.i32 can implement IADD.u32 if no saturation or swizzling is in use */
   static bool
   bi_can_iaddc(bi_instr *ins)
   {
      return (ins->op == BI_OPCODE_IADD_U32 && !ins->saturate &&
            }
      /*
   * The encoding of *FADD.v2f16 only specifies a single abs flag. All abs
   * encodings are permitted by swapping operands; however, this scheme fails if
   * both operands are equal. Test for this case.
   */
   static bool
   bi_impacted_abs(bi_instr *I)
   {
      return I->src[0].abs && I->src[1].abs &&
      }
      bool
   bi_can_fma(bi_instr *ins)
   {
      /* +IADD.i32 -> *IADDC.i32 */
   if (bi_can_iaddc(ins))
            /* +MUX -> *CSEL */
   if (bi_can_replace_with_csel(ins))
            /* *FADD.v2f16 has restricted abs modifiers, use +FADD.v2f16 instead */
   if (ins->op == BI_OPCODE_FADD_V2F16 && bi_impacted_abs(ins))
            /* TODO: some additional fp16 constraints */
      }
      static bool
   bi_impacted_fadd_widens(bi_instr *I)
   {
      enum bi_swizzle swz0 = I->src[0].swizzle;
            return (swz0 == BI_SWIZZLE_H00 && swz1 == BI_SWIZZLE_H11) ||
            }
      bool
   bi_can_add(bi_instr *ins)
   {
      /* +FADD.v2f16 lacks clamp modifier, use *FADD.v2f16 instead */
   if (ins->op == BI_OPCODE_FADD_V2F16 && ins->clamp)
            /* +FCMP.v2f16 lacks abs modifier, use *FCMP.v2f16 instead */
   if (ins->op == BI_OPCODE_FCMP_V2F16 && (ins->src[0].abs || ins->src[1].abs))
            /* +FADD.f32 has restricted widens, use +FADD.f32 for the full set */
   if (ins->op == BI_OPCODE_FADD_F32 && bi_impacted_fadd_widens(ins))
            /* TODO: some additional fp16 constraints */
      }
      /* Architecturally, no single instruction has a "not last" constraint. However,
   * pseudoinstructions writing multiple destinations (expanding to multiple
   * paired instructions) can run afoul of the "no two writes on the last clause"
   * constraint, so we check for that here.
   *
   * Exception to the exception: TEXC_DUAL, which writes to multiple sets of
   * staging registers. Staging registers bypass the usual register write
   * mechanism so this restriction does not apply.
   */
      static bool
   bi_must_not_last(bi_instr *ins)
   {
         }
      /* Check for a message-passing instruction. +DISCARD.f32 is special-cased; we
   * treat it as a message-passing instruction for the purpose of scheduling
   * despite no passing no logical message. Otherwise invalid encoding faults may
   * be raised for unknown reasons (possibly an errata).
   */
      bool
   bi_must_message(bi_instr *ins)
   {
      return (bi_opcode_props[ins->op].message != BIFROST_MESSAGE_NONE) ||
      }
      static bool
   bi_fma_atomic(enum bi_opcode op)
   {
      switch (op) {
   case BI_OPCODE_ATOM_C_I32:
   case BI_OPCODE_ATOM_C_I64:
   case BI_OPCODE_ATOM_C1_I32:
   case BI_OPCODE_ATOM_C1_I64:
   case BI_OPCODE_ATOM_C1_RETURN_I32:
   case BI_OPCODE_ATOM_C1_RETURN_I64:
   case BI_OPCODE_ATOM_C_RETURN_I32:
   case BI_OPCODE_ATOM_C_RETURN_I64:
   case BI_OPCODE_ATOM_POST_I32:
   case BI_OPCODE_ATOM_POST_I64:
   case BI_OPCODE_ATOM_PRE_I64:
         default:
            }
      bool
   bi_reads_zero(bi_instr *ins)
   {
         }
      bool
   bi_reads_temps(bi_instr *ins, unsigned src)
   {
      switch (ins->op) {
   /* Cannot permute a temporary */
   case BI_OPCODE_CLPER_I32:
   case BI_OPCODE_CLPER_OLD_I32:
            /* ATEST isn't supposed to be restricted, but in practice it always
   * wants to source its coverage mask input (source 0) from register 60,
   * which won't work properly if we put the input in a temp. This
   * requires workarounds in both RA and clause scheduling.
   */
   case BI_OPCODE_ATEST:
            case BI_OPCODE_IMULD:
         default:
            }
      static bool
   bi_impacted_t_modifiers(bi_instr *I, unsigned src)
   {
      assert(src < I->nr_srcs);
            switch (I->op) {
   case BI_OPCODE_F16_TO_F32:
   case BI_OPCODE_F16_TO_S32:
   case BI_OPCODE_F16_TO_U32:
   case BI_OPCODE_MKVEC_V2I16:
   case BI_OPCODE_S16_TO_F32:
   case BI_OPCODE_S16_TO_S32:
   case BI_OPCODE_U16_TO_F32:
   case BI_OPCODE_U16_TO_U32:
            case BI_OPCODE_BRANCH_F32:
   case BI_OPCODE_LOGB_F32:
   case BI_OPCODE_ILOGB_F32:
   case BI_OPCODE_FADD_F32:
   case BI_OPCODE_FCMP_F32:
   case BI_OPCODE_FREXPE_F32:
   case BI_OPCODE_FREXPM_F32:
   case BI_OPCODE_FROUND_F32:
            case BI_OPCODE_IADD_S32:
   case BI_OPCODE_IADD_U32:
   case BI_OPCODE_ISUB_S32:
   case BI_OPCODE_ISUB_U32:
   case BI_OPCODE_IADD_V4S8:
   case BI_OPCODE_IADD_V4U8:
   case BI_OPCODE_ISUB_V4S8:
   case BI_OPCODE_ISUB_V4U8:
            case BI_OPCODE_S8_TO_F32:
   case BI_OPCODE_S8_TO_S32:
   case BI_OPCODE_U8_TO_F32:
   case BI_OPCODE_U8_TO_U32:
            case BI_OPCODE_V2S8_TO_V2F16:
   case BI_OPCODE_V2S8_TO_V2S16:
   case BI_OPCODE_V2U8_TO_V2F16:
   case BI_OPCODE_V2U8_TO_V2U16:
            case BI_OPCODE_IADD_V2S16:
   case BI_OPCODE_IADD_V2U16:
   case BI_OPCODE_ISUB_V2S16:
   case BI_OPCODE_ISUB_V2U16:
         #if 0
         /* Restriction on IADD in 64-bit clauses on G72 */
   case BI_OPCODE_IADD_S64:
   case BI_OPCODE_IADD_U64:
   #endif
         default:
            }
      bool
   bi_reads_t(bi_instr *ins, unsigned src)
   {
      /* Branch offset cannot come from passthrough */
   if (bi_opcode_props[ins->op].branch)
            /* Table can never read passthrough */
   if (bi_opcode_props[ins->op].table)
            /* Staging register reads may happen before the succeeding register
   * block encodes a write, so effectively there is no passthrough */
   if (bi_is_staging_src(ins, src))
            /* Bifrost cores newer than Mali G71 have restrictions on swizzles on
   * same-cycle temporaries. Check the list for these hazards. */
   if (bi_impacted_t_modifiers(ins, src))
            /* Descriptor must not come from a passthrough */
   switch (ins->op) {
   case BI_OPCODE_LD_CVT:
   case BI_OPCODE_LD_TILE:
   case BI_OPCODE_ST_CVT:
   case BI_OPCODE_ST_TILE:
   case BI_OPCODE_TEXC:
   case BI_OPCODE_TEXC_DUAL:
         case BI_OPCODE_BLEND:
            /* +JUMP can't read the offset from T */
   case BI_OPCODE_JUMP:
            /* Else, just check if we can read any temps */
   default:
            }
      /* Counts the number of 64-bit constants required by a clause. TODO: We
   * might want to account for merging, right now we overestimate, but
   * that's probably fine most of the time */
      static unsigned
   bi_nconstants(struct bi_clause_state *clause)
   {
               for (unsigned i = 0; i < ARRAY_SIZE(clause->consts); ++i)
               }
      /* Would there be space for constants if we added one tuple? */
      static bool
   bi_space_for_more_constants(struct bi_clause_state *clause)
   {
         }
      /* Updates the FAU assignment for a tuple. A valid FAU assignment must be
   * possible (as a precondition), though not necessarily on the selected unit;
   * this is gauranteed per-instruction by bi_lower_fau and per-tuple by
   * bi_instr_schedulable */
      static bool
   bi_update_fau(struct bi_clause_state *clause, struct bi_tuple_state *tuple,
         {
      /* Maintain our own constants, for nondestructive mode */
   uint32_t copied_constants[2], copied_count;
   unsigned *constant_count = &tuple->constant_count;
   uint32_t *constants = tuple->constants;
            if (!destructive) {
      memcpy(copied_constants, tuple->constants,
                  constant_count = &copied_count;
               bi_foreach_src(instr, s) {
               if (src.type == BI_INDEX_FAU) {
      bool no_constants = *constant_count == 0;
                  if (destructive) {
      assert(mergable);
      } else if (!mergable) {
                     } else if (src.type == BI_INDEX_CONSTANT) {
      /* No need to reserve space if we have a fast 0 */
                  /* If there is a branch target, #0 by convention is the
   * PC-relative offset to the target */
                  for (unsigned i = 0; i < *constant_count; ++i) {
                  /* pcrel constants are unique, so don't match */
                                                   if (pcrel)
                                    /* Constants per clause may be limited by tuple count */
   bool room_for_constants =
            if (destructive)
         else if (!room_for_constants)
               }
      /* Given an in-progress tuple, a candidate new instruction to add to the tuple,
   * and a source (index) from that candidate, determine whether this source is
   * "new", in the sense of requiring an additional read slot. That is, checks
   * whether the specified source reads from the register file via a read slot
   * (determined by its type and placement) and whether the source was already
   * specified by a prior read slot (to avoid double counting) */
      static bool
   bi_tuple_is_new_src(bi_instr *instr, struct bi_reg_state *reg, unsigned src_idx)
   {
      assert(src_idx < instr->nr_srcs);
            /* Only consider sources which come from the register file */
   if (!(src.type == BI_INDEX_NORMAL || src.type == BI_INDEX_REGISTER))
            /* Staging register reads bypass the usual register file mechanism */
   if (bi_is_staging_src(instr, src_idx))
            /* If a source is already read in the tuple, it is already counted */
   for (unsigned t = 0; t < reg->nr_reads; ++t)
      if (bi_is_word_equiv(src, reg->reads[t]))
         /* If a source is read in _this instruction_, it is already counted */
   for (unsigned t = 0; t < src_idx; ++t)
      if (bi_is_word_equiv(src, instr->src[t]))
            }
      /* Given two tuples in source order, count the number of register reads of the
   * successor, determined as the number of unique words accessed that aren't
   * written by the predecessor (since those are tempable).
   */
      static unsigned
   bi_count_succ_reads(bi_index t0, bi_index t1, bi_index *succ_reads,
         {
               for (unsigned i = 0; i < nr_succ_reads; ++i) {
               for (unsigned j = 0; j < i; ++j)
                  if (!unique)
            if (bi_is_word_equiv(succ_reads[i], t0))
            if (bi_is_word_equiv(succ_reads[i], t1))
                           }
      /* Not all instructions can read from the staging passthrough (as determined by
   * reads_t), check if a given pair of instructions has such a restriction. Note
   * we also use this mechanism to prevent data races around staging register
   * reads, so we allow the input source to potentially be vector-valued */
      static bool
   bi_has_staging_passthrough_hazard(bi_index fma, bi_instr *add)
   {
      bi_foreach_src(add, s) {
               if (src.type != BI_INDEX_REGISTER)
            unsigned count = bi_count_read_registers(add, s);
            for (unsigned d = 0; d < count; ++d)
            if (read && !bi_reads_t(add, s))
                  }
      /* Likewise for cross-tuple passthrough (reads_temps) */
      static bool
   bi_has_cross_passthrough_hazard(bi_tuple *succ, bi_instr *ins)
   {
      if (ins->nr_dests == 0)
            bi_foreach_instr_in_tuple(succ, pins) {
      bi_foreach_src(pins, s) {
      if (bi_is_word_equiv(ins->dest[0], pins->src[s]) &&
      !bi_reads_temps(pins, s))
                  }
      /* Is a register written other than the staging mechanism? ATEST is special,
   * writing to both a staging register and a regular register (fixed packing).
   * BLEND is special since it has to write r48 the normal way even if it never
   * gets read. This depends on liveness analysis, as a register is not needed
   * for a write that will be discarded after one tuple. */
      static unsigned
   bi_write_count(bi_instr *instr, uint64_t live_after_temp)
   {
      if (instr->op == BI_OPCODE_ATEST || instr->op == BI_OPCODE_BLEND)
                     bi_foreach_dest(instr, d) {
      if (d == 0 && bi_opcode_props[instr->op].sr_write)
            assert(instr->dest[0].type == BI_INDEX_REGISTER);
   if (live_after_temp & BITFIELD64_BIT(instr->dest[0].value))
                  }
      /*
   * Test if an instruction required flush-to-zero mode. Currently only supported
   * for f16<-->f32 conversions to implement fquantize16
   */
   static bool
   bi_needs_ftz(bi_instr *I)
   {
      return (I->op == BI_OPCODE_F16_TO_F32 ||
            }
      /*
   * Test if an instruction would be numerically incompatible with the clause. At
   * present we only consider flush-to-zero modes.
   */
   static bool
   bi_numerically_incompatible(struct bi_clause_state *clause, bi_instr *instr)
   {
      return (clause->ftz != BI_FTZ_STATE_NONE) &&
      }
      /* Instruction placement entails two questions: what subset of instructions in
   * the block can legally be scheduled? and of those which is the best? That is,
   * we seek to maximize a cost function on a subset of the worklist satisfying a
   * particular predicate. The necessary predicate is determined entirely by
   * Bifrost's architectural limitations and is described in the accompanying
   * whitepaper. The cost function is a heuristic. */
      static bool
   bi_instr_schedulable(bi_instr *instr, struct bi_clause_state *clause,
               {
      /* The units must match */
   if ((fma && !bi_can_fma(instr)) || (!fma && !bi_can_add(instr)))
            /* There can only be one message-passing instruction per clause */
   if (bi_must_message(instr) && clause->message)
            /* Some instructions have placement requirements */
   if (bi_opcode_props[instr->op].last && !tuple->last)
            if (bi_must_not_last(instr) && tuple->last)
            /* Numerical properties must be compatible with the clause */
   if (bi_numerically_incompatible(clause, instr))
            /* Message-passing instructions are not guaranteed write within the
   * same clause (most likely they will not), so if a later instruction
   * in the clause accesses the destination, the message-passing
   * instruction can't be scheduled */
   if (bi_opcode_props[instr->op].sr_write) {
      bi_foreach_dest(instr, d) {
      unsigned nr = bi_count_write_registers(instr, d);
                  for (unsigned i = 0; i < clause->access_count; ++i) {
      bi_index idx = clause->accesses[i];
   for (unsigned d = 0; d < nr; ++d) {
      if (bi_is_equiv(bi_register(reg + d), idx))
                        if (bi_opcode_props[instr->op].sr_read && !bi_is_null(instr->src[0])) {
      unsigned nr = bi_count_read_registers(instr, 0);
   assert(instr->src[0].type == BI_INDEX_REGISTER);
            for (unsigned i = 0; i < clause->access_count; ++i) {
      bi_index idx = clause->accesses[i];
   for (unsigned d = 0; d < nr; ++d) {
      if (bi_is_equiv(bi_register(reg + d), idx))
                     /* If FAU is already assigned, we may not disrupt that. Do a
   * non-disruptive test update */
   if (!bi_update_fau(clause, tuple, instr, fma, false))
            /* If this choice of FMA would force a staging passthrough, the ADD
   * instruction must support such a passthrough */
   if (tuple->add && instr->nr_dests &&
      bi_has_staging_passthrough_hazard(instr->dest[0], tuple->add))
         /* If this choice of destination would force a cross-tuple passthrough, the
   * next tuple must support that */
   if (tuple->prev && bi_has_cross_passthrough_hazard(tuple->prev, instr))
            /* Register file writes are limited */
   unsigned total_writes = tuple->reg.nr_writes;
            /* Last tuple in a clause can only write a single value */
   if (tuple->last && total_writes > 1)
                              bi_foreach_src(instr, s) {
      if (bi_tuple_is_new_src(instr, &tuple->reg, s))
                        bool can_spill_to_moves = (!tuple->add);
   can_spill_to_moves &=
                  /* However, we can get an extra 1 or 2 sources by inserting moves */
   if (total_srcs > (can_spill_to_moves ? 4 : 3))
            /* Count effective reads for the successor */
            if (instr->nr_dests) {
      bool has_t1 = tuple->add && tuple->add->nr_dests;
   succ_reads = bi_count_succ_reads(instr->dest[0],
                     /* Successor must satisfy R+W <= 4, so we require W <= 4-R */
   if ((signed)total_writes > (4 - (signed)succ_reads))
               }
      static signed
   bi_instr_cost(bi_instr *instr, struct bi_tuple_state *tuple)
   {
               /* Instructions that can schedule to either FMA or to ADD should be
   * deprioritized since they're easier to reschedule elsewhere */
   if (bi_can_fma(instr) && bi_can_add(instr))
            /* Message-passing instructions impose constraints on the registers
   * later in the clause, so schedule them as late within a clause as
   * possible (<==> prioritize them since we're backwards <==> decrease
   * cost) */
   if (bi_must_message(instr))
            /* Last instructions are big constraints (XXX: no effect on shader-db) */
   if (bi_opcode_props[instr->op].last)
               }
      static unsigned
   bi_choose_index(struct bi_worklist st, struct bi_clause_state *clause,
               {
      unsigned i, best_idx = ~0;
            BITSET_FOREACH_SET(i, st.worklist, st.count) {
               if (!bi_instr_schedulable(instr, clause, tuple, live_after_temp, fma))
                     /* Tie break in favour of later instructions, under the
   * assumption this promotes temporary usage (reducing pressure
   * on the register file). This is a side effect of a prepass
            if (cost <= best_cost) {
      best_idx = i;
                     }
      static void
   bi_pop_instr(struct bi_clause_state *clause, struct bi_tuple_state *tuple,
         {
               assert(clause->access_count + instr->nr_srcs + instr->nr_dests <=
            memcpy(clause->accesses + clause->access_count, instr->src,
                  memcpy(clause->accesses + clause->access_count, instr->dest,
                           bi_foreach_src(instr, s) {
      if (bi_tuple_is_new_src(instr, &tuple->reg, s))
               /* This could be optimized to allow pairing integer instructions with
   * special flush-to-zero instructions, but punting on this until we have
   * a workload that cares.
   */
   clause->ftz =
      }
      /* Choose the best instruction and pop it off the worklist. Returns NULL if no
   * instruction is available. This function is destructive. */
      static bi_instr *
   bi_take_instr(bi_context *ctx, struct bi_worklist st,
               {
      if (tuple->add && tuple->add->op == BI_OPCODE_CUBEFACE)
         else if (tuple->add && tuple->add->op == BI_OPCODE_ATOM_RETURN_I32)
         else if (tuple->add && tuple->add->op == BI_OPCODE_ATOM1_RETURN_I32)
         else if (tuple->add && tuple->add->op == BI_OPCODE_SEG_ADD_I64)
         else if (tuple->add && tuple->add->table)
            /* TODO: Optimize these moves */
   if (!fma && tuple->nr_prev_reads > 3) {
      /* Only spill by one source for now */
            /* Pick a source to spill */
            /* Schedule the spill */
   bi_builder b = bi_init_builder(ctx, bi_before_tuple(tuple->prev));
   bi_instr *mov = bi_mov_i32_to(&b, src, src);
   bi_pop_instr(clause, tuple, mov, live_after_temp, fma);
            #ifndef NDEBUG
      /* Don't pair instructions if debugging */
   if ((bifrost_debug & BIFROST_DBG_NOSCHED) && tuple->add)
      #endif
                  if (idx >= st.count)
            /* Update state to reflect taking the instruction */
            BITSET_CLEAR(st.worklist, idx);
   bi_update_worklist(st, idx);
            /* Fixups */
            if (instr->op == BI_OPCODE_IADD_U32 && fma) {
      assert(bi_can_iaddc(instr));
   bi_instr *iaddc = bi_iaddc_i32_to(&b, instr->dest[0], instr->src[0],
            bi_remove_instruction(instr);
      } else if (fma && bi_can_replace_with_csel(instr)) {
               bi_remove_instruction(instr);
                  }
      /* Variant of bi_rewrite_index_src_single that uses word-equivalence, rewriting
   * to a passthrough register. If except_sr is true, the staging sources are
   * skipped, so staging register reads are not accidentally encoded as
   * passthrough (which is impossible) */
      static void
   bi_use_passthrough(bi_instr *ins, bi_index old, enum bifrost_packed_src new,
         {
      /* Optional for convenience */
   if (!ins)
                     bi_foreach_src(ins, i) {
      if ((i == 0 || i == 4) && except_sr)
            if (bi_is_word_equiv(ins->src[i], old)) {
      ins->src[i].type = BI_INDEX_PASS;
   ins->src[i].value = new;
            }
      /* Rewrites an adjacent pair of tuples _prec_eding and _succ_eding to use
   * intertuple passthroughs where necessary. Passthroughs are allowed as a
   * post-condition of scheduling. Note we rewrite ADD first, FMA second --
   * opposite the order of execution. This is deliberate -- if both FMA and ADD
   * write to the same logical register, the next executed tuple will get the
   * latter result. There's no interference issue under the assumption of correct
   * register allocation. */
      static void
   bi_rewrite_passthrough(bi_tuple prec, bi_tuple succ)
   {
               if (prec.add && prec.add->nr_dests) {
      bi_use_passthrough(succ.fma, prec.add->dest[0], BIFROST_SRC_PASS_ADD,
         bi_use_passthrough(succ.add, prec.add->dest[0], BIFROST_SRC_PASS_ADD,
               if (prec.fma && prec.fma->nr_dests) {
      bi_use_passthrough(succ.fma, prec.fma->dest[0], BIFROST_SRC_PASS_FMA,
         bi_use_passthrough(succ.add, prec.fma->dest[0], BIFROST_SRC_PASS_FMA,
         }
      static void
   bi_rewrite_fau_to_pass(bi_tuple *tuple)
   {
      bi_foreach_instr_and_src_in_tuple(tuple, ins, s) {
      if (ins->src[s].type != BI_INDEX_FAU)
            bi_index pass = bi_passthrough(ins->src[s].offset ? BIFROST_SRC_FAU_HI
                  }
      static void
   bi_rewrite_zero(bi_instr *ins, bool fma)
   {
               bi_foreach_src(ins, s) {
               if (src.type == BI_INDEX_CONSTANT && src.value == 0)
         }
      /* Assumes #0 to {T, FAU} rewrite has already occurred */
      static void
   bi_rewrite_constants_to_pass(bi_tuple *tuple, uint64_t constant, bool pcrel)
   {
      bi_foreach_instr_and_src_in_tuple(tuple, ins, s) {
      if (ins->src[s].type != BI_INDEX_CONSTANT)
                     ASSERTED bool lo = (cons == (constant & 0xffffffff));
            /* PC offsets always live in the upper half, set to zero by
   * convention before pack time. (This is safe, since if you
   * wanted to compare against zero, you would use a BRANCHZ
   * instruction instead.) */
   if (cons == 0 && ins->branch_target != NULL) {
      assert(pcrel);
   hi = true;
      } else if (pcrel) {
                           bi_replace_src(
         }
      /* Constructs a constant state given a tuple state. This has the
   * postcondition that pcrel applies to the first constant by convention,
   * and PC-relative constants will be #0 by convention here, so swap to
   * match if needed */
      static struct bi_const_state
   bi_get_const_state(struct bi_tuple_state *tuple)
   {
      struct bi_const_state consts = {
      .constant_count = tuple->constant_count,
   .constants[0] = tuple->constants[0],
   .constants[1] = tuple->constants[1],
               /* pcrel applies to the first constant by convention, and
   * PC-relative constants will be #0 by convention here, so swap
   * to match if needed */
   if (consts.pcrel && consts.constants[0]) {
      assert(consts.constant_count == 2);
            consts.constants[1] = consts.constants[0];
                  }
      /* Merges constants in a clause, satisfying the following rules, assuming no
   * more than one tuple has pcrel:
   *
   * 1. If a tuple has two constants, they must be packed together. If one is
   * pcrel, it must be the high constant to use the M1=4 modification [sx64(E0) +
   * (PC << 32)]. Otherwise choose an arbitrary order.
   *
   * 4. If a tuple has one constant, it may be shared with an existing
   * pair that already contains that constant, or it may be combined with another
   * (distinct) tuple of a single constant.
   *
   * This gaurantees a packing is possible. The next routine handles modification
   * related swapping, to satisfy format 12 and the lack of modification for
   * tuple count 5/8 in EC0.
   */
      static uint64_t
   bi_merge_u32(uint32_t c0, uint32_t c1, bool pcrel)
   {
      /* At this point in the constant merge algorithm, pcrel constants are
   * treated as zero, so pcrel implies at least one constants is zero */
            /* Order: pcrel, maximum non-pcrel, minimum non-pcrel */
   uint32_t hi = pcrel ? 0 : MAX2(c0, c1);
            /* Merge in the selected order */
      }
      static unsigned
   bi_merge_pairs(struct bi_const_state *consts, unsigned tuple_count,
         {
               for (unsigned t = 0; t < tuple_count; ++t) {
      if (consts[t].constant_count != 2)
            unsigned idx = ~0;
   uint64_t val = bi_merge_u32(consts[t].constants[0],
            /* Skip the pcrel pair if assigned, because if one is assigned,
   * this one is not pcrel by uniqueness so it's a mismatch */
   for (unsigned s = 0; s < merge_count; ++s) {
      if (merged[s] == val && (*pcrel_pair) != s) {
      idx = s;
                  if (idx == ~0) {
                     if (consts[t].pcrel)
                              }
      static unsigned
   bi_merge_singles(struct bi_const_state *consts, unsigned tuple_count,
         {
      bool pending = false, pending_pcrel = false;
            for (unsigned t = 0; t < tuple_count; ++t) {
      if (consts[t].constant_count != 1)
            uint32_t val = consts[t].constants[0];
            /* Try to match, but don't match pcrel with non-pcrel, even
   * though we can merge a pcrel with a non-pcrel single */
   for (unsigned i = 0; i < pair_count; ++i) {
      bool lo = ((pairs[i] & 0xffffffff) == val);
   bool hi = ((pairs[i] >> 32) == val);
   bool match = (lo || hi);
   match &= ((*pcrel_pair) != i);
   if (match && !consts[t].pcrel) {
      idx = i;
                  if (idx == ~0) {
               if (pending && pending_single != val) {
                                                } else {
      pending = true;
   pending_pcrel = consts[t].pcrel;
                              /* Shift so it works whether pending_pcrel is set or not */
   if (pending) {
      if (pending_pcrel)
                           }
      static unsigned
   bi_merge_constants(struct bi_const_state *consts, uint64_t *pairs,
         {
      unsigned pair_count = bi_merge_pairs(consts, 8, pairs, pcrel_idx);
      }
      /* Swap two constants at word i and i+1 by swapping their actual positions and
   * swapping all references so the meaning of the clause is preserved */
      static void
   bi_swap_constants(struct bi_const_state *consts, uint64_t *pairs, unsigned i)
   {
      uint64_t tmp_pair = pairs[i + 0];
   pairs[i + 0] = pairs[i + 1];
            for (unsigned t = 0; t < 8; ++t) {
      if (consts[t].word_idx == i)
         else if (consts[t].word_idx == (i + 1))
         }
      /* Given merged constants, one of which might be PC-relative, fix up the M
   * values so the PC-relative constant (if it exists) has the M1=4 modification
   * and other constants are used as-is (which might require swapping) */
      static unsigned
   bi_apply_constant_modifiers(struct bi_const_state *consts, uint64_t *pairs,
               {
               /* Clauses with these tuple counts lack an M field for the packed EC0,
   * so EC0 cannot be PC-relative, which might require swapping (and
            if (*pcrel_idx == 0 && (tuple_count == 5 || tuple_count == 8)) {
      constant_count = MAX2(constant_count, 2);
   *pcrel_idx = 1;
               /* EC0 might be packed free, after that constants are packed in pairs
            for (unsigned i = start; i < constant_count; i += 2) {
      bool swap = false;
            unsigned A1 = (pairs[i] >> 60);
            if (*pcrel_idx == i || *pcrel_idx == (i + 1)) {
                     /* Set M1 = 4 by noting (A - B) mod 16 = 4 is
   * equivalent to A = (B + 4) mod 16 and that we can
   * control A */
   unsigned B = swap ? A1 : B1;
                  /* Swapped if swap set, identity if swap not set */
      } else {
                     /* For M1 = 0 or M1 >= 8, the constants are unchanged,
   * we have 0 < (A1 - B1) % 16 < 8, which implies (B1 -
   * A1) % 16 >= 8, so swapping will let them be used
                  /* However, we can't swap the last constant, so we
   * force M1 = 0 instead for this case */
   if (last && swap) {
      pairs[i + 1] |= pairs[i] & (0xfull << 60);
                  if (swap) {
      assert(!last);
                     }
      /* Schedule a single clause. If no instructions remain, return NULL. */
      static bi_clause *
   bi_schedule_clause(bi_context *ctx, bi_block *block, struct bi_worklist st,
         {
      struct bi_clause_state clause_state = {0};
   bi_clause *clause = rzalloc(ctx, bi_clause);
                     /* TODO: Decide flow control better */
            /* The last clause can only write one instruction, so initialize that */
   struct bi_reg_state reg_state = {};
   bi_index prev_reads[5] = {bi_null()};
            /* We need to track future liveness. The main *live set tracks what is
   * live at the current point int he program we are scheduling, but to
   * determine temp eligibility, we instead want what will be live after
   * the next tuple in the program. If you scheduled forwards, you'd need
   * a crystall ball for this. Luckily we schedule backwards, so we just
   * delay updates to the live_after_temp by an extra tuple. */
   uint64_t live_after_temp = *live;
            do {
      struct bi_tuple_state tuple_state = {
      .last = (clause->tuple_count == 0),
   .reg = reg_state,
   .nr_prev_reads = nr_prev_reads,
   .prev = tuple,
               assert(nr_prev_reads < ARRAY_SIZE(prev_reads));
                              if (clause->message && bi_opcode_props[clause->message->op].sr_read &&
      !bi_is_null(clause->message->src[0])) {
   unsigned nr = bi_count_read_registers(clause->message, 0);
   live_after_temp |=
               /* Since we schedule backwards, we schedule ADD first */
   tuple_state.add = bi_take_instr(ctx, st, &clause_state, &tuple_state,
         tuple->fma = bi_take_instr(ctx, st, &clause_state, &tuple_state,
                  /* Update liveness from the new instructions */
   if (tuple->add)
            if (tuple->fma)
            /* Rotate in the new per-tuple liveness */
   live_after_temp = live_next_tuple;
            /* We may have a message, but only one per clause */
   if (tuple->add && bi_must_message(tuple->add)) {
                                    /* We don't need to set dependencies for blend shaders
   * because the BLEND instruction in the fragment
   * shader should have already done the wait */
   if (!ctx->inputs->is_blend) {
      switch (tuple->add->op) {
   case BI_OPCODE_ATEST:
      clause->dependencies |= (1 << BIFROST_SLOT_ELDEST_DEPTH);
      case BI_OPCODE_LD_TILE:
   case BI_OPCODE_ST_TILE:
      clause->dependencies |= (1 << BIFROST_SLOT_ELDEST_COLOUR);
      case BI_OPCODE_BLEND:
      clause->dependencies |= (1 << BIFROST_SLOT_ELDEST_DEPTH);
   clause->dependencies |= (1 << BIFROST_SLOT_ELDEST_COLOUR);
      default:
                                 /* Before merging constants, eliminate zeroes, otherwise the
   * merging will fight over the #0 that never gets read (and is
   * never marked as read by update_fau) */
   if (tuple->fma && bi_reads_zero(tuple->fma))
            /* Rewrite away FAU, constant write is deferred */
   if (!tuple_state.constant_count) {
      tuple->fau_idx = tuple_state.fau;
               /* Use passthrough register for cross-stage accesses. Since
   * there are just FMA and ADD stages, that means we rewrite to
   * passthrough the sources of the ADD that read from the
            if (tuple->fma && tuple->fma->nr_dests) {
      bi_use_passthrough(tuple->add, tuple->fma->dest[0], BIFROST_SRC_STAGE,
               /* Don't add an empty tuple, unless the worklist has nothing
   * but a (pseudo)instruction failing to schedule due to a "not
            int some_instruction = __bitset_ffs(st.worklist, BITSET_WORDS(st.count));
   bool not_last = (some_instruction > 0) &&
                     if (!(tuple->fma || tuple->add || insert_empty))
                     /* Adding enough tuple might overflow constants */
   if (!bi_space_for_more_constants(&clause_state))
      #ifndef NDEBUG
         /* Don't schedule more than 1 tuple if debugging */
   if ((bifrost_debug & BIFROST_DBG_NOSCHED) && !insert_empty)
   #endif
            /* Link through the register state */
   STATIC_ASSERT(sizeof(prev_reads) == sizeof(tuple_state.reg.reads));
   memcpy(prev_reads, tuple_state.reg.reads, sizeof(prev_reads));
   nr_prev_reads = tuple_state.reg.nr_reads;
               /* Don't schedule an empty clause */
   if (!clause->tuple_count)
            /* Before merging, rewrite away any tuples that read only zero */
   for (unsigned i = max_tuples - clause->tuple_count; i < max_tuples; ++i) {
      bi_tuple *tuple = &clause->tuples[i];
            if (st->constant_count == 0 || st->constants[0] || st->constants[1] ||
                  bi_foreach_instr_in_tuple(tuple, ins)
            /* Constant has been demoted to FAU, so don't pack it separately */
            /* Default */
               uint64_t constant_pairs[8] = {0};
   unsigned pcrel_idx = ~0;
   unsigned constant_words =
            constant_words = bi_apply_constant_modifiers(
      clause_state.consts, constant_pairs, &pcrel_idx, clause->tuple_count,
                  for (unsigned i = max_tuples - clause->tuple_count; i < max_tuples; ++i) {
               /* If no constants, leave FAU as it is, possibly defaulting to 0 */
   if (clause_state.consts[i].constant_count == 0)
            /* FAU is already handled */
            unsigned word_idx = clause_state.consts[i].word_idx;
            /* We could try to merge regardless of bottom bits as well, but
   * that's probably diminishing returns */
   uint64_t pair = constant_pairs[word_idx];
            tuple->fau_idx = bi_constant_field(word_idx) | lo;
               clause->constant_count = constant_words;
            /* Branches must be last, so this can be factored out */
   bi_instr *last = clause->tuples[max_tuples - 1].add;
   clause->next_clause_prefetch = !last || (last->op != BI_OPCODE_JUMP);
                     /* We emit in reverse and emitted to the back of the tuples array, so
   * move it up front for easy indexing */
   memmove(clause->tuples, clause->tuples + (max_tuples - clause->tuple_count),
            /* Use passthrough register for cross-tuple accesses. Note this is
   * after the memmove, so this is forwards. Skip the first tuple since
            for (unsigned t = 1; t < clause->tuple_count; ++t)
               }
      static void
   bi_schedule_block(bi_context *ctx, bi_block *block)
   {
               /* Copy list to dynamic array */
   struct bi_worklist st = bi_initialize_worklist(
            if (!st.count) {
      bi_free_worklist(st);
               /* We need to track liveness during scheduling in order to determine whether
   * we can use temporary (passthrough) registers */
            /* Schedule as many clauses as needed to fill the block */
   bi_clause *u = NULL;
   while ((u = bi_schedule_clause(ctx, block, st, &live)))
            /* Back-to-back bit affects only the last clause of a block,
   * the rest are implicitly true */
   if (!list_is_empty(&block->clauses)) {
      bi_clause *last_clause =
         if (bi_reconverge_branches(block))
               /* Reorder instructions to match the new schedule. First remove
            bi_foreach_instr_in_block_safe(block, ins) {
                  bi_foreach_clause_in_block(block, clause) {
      for (unsigned i = 0; i < clause->tuple_count; ++i) {
      bi_foreach_instr_in_tuple(&clause->tuples[i], ins) {
                              #ifndef NDEBUG
      unsigned i;
            BITSET_FOREACH_SET(i, st.worklist, st.count) {
      bi_print_instr(st.instructions[i], stderr);
               if (incomplete)
      #endif
            }
      static bool
   bi_check_fau_src(bi_instr *ins, unsigned s, uint32_t *constants,
         {
      assert(s < ins->nr_srcs);
            /* Staging registers can't have FAU accesses */
   if (bi_is_staging_src(ins, s))
            if (src.type == BI_INDEX_CONSTANT) {
      /* Allow fast zero */
   if (src.value == 0 && bi_opcode_props[ins->op].fma && bi_reads_zero(ins))
            if (!bi_is_null(*fau))
            /* Else, try to inline a constant */
   for (unsigned i = 0; i < *cwords; ++i) {
      if (src.value == constants[i])
               if (*cwords >= 2)
               } else if (src.type == BI_INDEX_FAU) {
      if (*cwords != 0)
            /* Can only read from one pair of FAU words */
   if (!bi_is_null(*fau) && (src.value != fau->value))
            /* If there is a target, we'll need a PC-relative constant */
   if (ins->branch_target)
                           }
      void
   bi_lower_fau(bi_context *ctx)
   {
      bi_foreach_instr_global_safe(ctx, ins) {
               uint32_t constants[2];
   unsigned cwords = 0;
            /* ATEST must have the ATEST datum encoded, not any other
   * uniform. See to it this is the case. */
   if (ins->op == BI_OPCODE_ATEST)
            /* Dual texturing requires the texture operation descriptor
   * encoded as an immediate so we can fix up.
   */
   if (ins->op == BI_OPCODE_TEXC_DUAL) {
      assert(ins->src[3].type == BI_INDEX_CONSTANT);
               /* Phis get split up into moves so are unrestricted */
   if (ins->op == BI_OPCODE_PHI)
            bi_foreach_src(ins, s) {
                     bi_index copy = bi_mov_i32(&b, ins->src[s]);
            }
      /* Only v7 allows specifying a dependency on the tilebuffer for the first
   * clause of a shader. v6 requires adding a NOP clause with the depedency. */
      static void
   bi_add_nop_for_atest(bi_context *ctx)
   {
      /* Only needed on v6 */
   if (ctx->arch >= 7)
            if (list_is_empty(&ctx->blocks))
            /* Fetch the first clause of the shader */
   bi_block *block = list_first_entry(&ctx->blocks, bi_block, link);
            if (!clause || !(clause->dependencies & ((1 << BIFROST_SLOT_ELDEST_DEPTH) |
                  /* Add a NOP so we can wait for the dependencies required by the first
            bi_instr *I = rzalloc(ctx, bi_instr);
            bi_clause *new_clause = ralloc(ctx, bi_clause);
   *new_clause = (bi_clause){
      .flow_control = BIFROST_FLOW_NBTB,
   .next_clause_prefetch = true,
            .tuple_count = 1,
   .tuples[0] =
      {
                     }
      void
   bi_schedule(bi_context *ctx)
   {
      /* Fed into both scheduling and DCE */
            bi_foreach_block(ctx, block) {
                  bi_opt_dce_post_ra(ctx);
      }
