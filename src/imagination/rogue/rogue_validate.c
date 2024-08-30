   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "rogue.h"
   #include "util/bitscan.h"
   #include "util/macros.h"
   #include "util/u_dynarray.h"
      #include <stdbool.h>
      /**
   * \file rogue_validate.c
   *
   * \brief Contains functions to validate Rogue IR.
   */
      /* TODO: Rogue_validate should make sure that immediate (sources) don't have any
   * modifiers set... */
      /* TODO NEXT: Check for emit/end/etc. as last instruction in vertex shader, and
   * nop.end, or end flag set (or just pseudo-end) otherwise. */
      typedef struct rogue_validation_state {
      const rogue_shader *shader; /** The shader being validated. */
   const char *when; /** Description of the validation being done. */
   bool nonfatal; /** Don't stop at the first error.*/
   struct {
      const rogue_block *block; /** Current basic block being validated. */
   const rogue_instr *instr; /** Current instruction being validated. */
   const rogue_instr_group *group; /** Current instruction group being
         const rogue_ref *ref; /** Current reference being validated. */
   bool src; /** Current reference type (src/dst). */
      } ctx;
      } rogue_validation_state;
      /* Returns true if errors are present. */
   static bool validate_print_errors(rogue_validation_state *state)
   {
      if (!util_dynarray_num_elements(state->error_msgs, const char *))
            util_dynarray_foreach (state->error_msgs, const char *, msg) {
                           rogue_print_shader(stderr, state->shader);
               }
      static void PRINTFLIKE(2, 3)
         {
                        if (state->ctx.block) {
      if (state->ctx.block->label)
         else
               if (state->ctx.instr) {
                  if (state->ctx.ref) {
      ralloc_asprintf_append(&msg,
                                    va_list args;
   va_start(args, fmt);
   ralloc_vasprintf_append(&msg, fmt, args);
   util_dynarray_append(state->error_msgs, const char *, msg);
            if (!state->nonfatal) {
      validate_print_errors(state);
         }
      static rogue_validation_state *
   create_validation_state(const rogue_shader *shader, const char *when)
   {
               state->shader = shader;
   state->when = when;
            state->error_msgs = rzalloc_size(state, sizeof(*state->error_msgs));
               }
      static void validate_regarray(rogue_validation_state *state,
         {
      if (!regarray->size) {
      validate_log(state, "Register array is empty.");
               enum rogue_reg_class class = regarray->regs[0]->class;
            for (unsigned u = 0; u < regarray->size; ++u) {
      if (regarray->regs[u]->class != class)
            if (regarray->regs[u]->index != (base_index + u))
         }
      static void validate_dst(rogue_validation_state *state,
                           const rogue_instr_dst *dst,
   uint64_t supported_dst_types,
   {
      state->ctx.ref = &dst->ref;
   state->ctx.src = false;
            if (rogue_ref_is_null(&dst->ref))
            if (!rogue_ref_type_supported(dst->ref.type, supported_dst_types))
            if (rogue_ref_is_reg_or_regarray(&dst->ref) && stride != ~0U) {
      unsigned dst_size = stride + 1;
   if (repeat_mask & (1 << i))
            if (rogue_ref_is_regarray(&dst->ref)) {
      if (rogue_ref_get_regarray_size(&dst->ref) != dst_size) {
      validate_log(state,
                     } else if (dst_size > 1) {
                        }
      static void validate_src(rogue_validation_state *state,
                           const rogue_instr_src *src,
   uint64_t supported_src_types,
   {
      state->ctx.ref = &src->ref;
   state->ctx.src = true;
            if (rogue_ref_is_null(&src->ref))
            if (!rogue_ref_type_supported(src->ref.type, supported_src_types))
            if (rogue_ref_is_reg_or_regarray(&src->ref) && stride != ~0U) {
      unsigned src_size = stride + 1;
   if (repeat_mask & (1 << i))
            if (rogue_ref_is_regarray(&src->ref)) {
      if (rogue_ref_get_regarray_size(&src->ref) != src_size) {
      validate_log(state,
                     } else if (src_size > 1) {
                        }
      static bool validate_alu_op_mod_combo(uint64_t mods)
   {
      rogue_foreach_mod_in_set (mod, mods) {
               /* Check if any excluded op mods have been included. */
   if (info->exclude & mods)
            /* Check if any required op mods have been missed. */
   if (info->require && !(info->require & mods))
                  }
      static void validate_alu_instr(rogue_validation_state *state,
         {
      if (alu->op == ROGUE_ALU_OP_INVALID || alu->op >= ROGUE_ALU_OP_COUNT)
                     /* Check if instruction modifiers are valid. */
   if (!rogue_mods_supported(alu->mod, info->supported_op_mods))
            if (!validate_alu_op_mod_combo(alu->mod))
            /* Instruction repeat checks. */
   if (alu->instr.repeat > 1 && !info->dst_repeat_mask &&
      !info->src_repeat_mask) {
               /* Validate destinations and sources for ungrouped shaders. */
   if (!state->shader->is_grouped) {
      for (unsigned i = 0; i < info->num_dsts; ++i) {
      validate_dst(state,
               &alu->dst[i],
   info->supported_dst_types[i],
   i,
               for (unsigned i = 0; i < info->num_srcs; ++i) {
      validate_src(state,
               &alu->src[i],
   info->supported_src_types[i],
   i,
            }
      static bool validate_backend_op_mod_combo(uint64_t mods)
   {
      rogue_foreach_mod_in_set (mod, mods) {
               /* Check if any excluded op mods have been included. */
   if (info->exclude & mods)
            /* Check if any required op mods have been missed. */
   if (info->require && !(info->require & mods))
                  }
      static void validate_backend_instr(rogue_validation_state *state,
         {
      if (backend->op == ROGUE_BACKEND_OP_INVALID ||
      backend->op >= ROGUE_BACKEND_OP_COUNT)
                  /* Check if instruction modifiers are valid. */
   if (!rogue_mods_supported(backend->mod, info->supported_op_mods))
            if (!validate_backend_op_mod_combo(backend->mod))
            /* Instruction repeat checks. */
   if (backend->instr.repeat > 1 && !info->dst_repeat_mask &&
      !info->src_repeat_mask) {
               /* Validate destinations and sources for ungrouped shaders. */
   if (!state->shader->is_grouped) {
      for (unsigned i = 0; i < info->num_dsts; ++i) {
      validate_dst(state,
               &backend->dst[i],
   info->supported_dst_types[i],
   i,
               for (unsigned i = 0; i < info->num_srcs; ++i) {
      validate_src(state,
               &backend->src[i],
   info->supported_src_types[i],
   i,
            }
      static bool validate_ctrl_op_mod_combo(uint64_t mods)
   {
      rogue_foreach_mod_in_set (mod, mods) {
               /* Check if any excluded op mods have been included. */
   if (info->exclude & mods)
            /* Check if any required op mods have been missed. */
   if (info->require && !(info->require & mods))
                  }
      /* Returns true if instruction can end block. */
   static bool validate_ctrl_instr(rogue_validation_state *state,
         {
      if (ctrl->op == ROGUE_CTRL_OP_INVALID || ctrl->op >= ROGUE_CTRL_OP_COUNT)
            /* TODO: Validate rest, check blocks, etc. */
            if (info->has_target && !ctrl->target_block)
         else if (!info->has_target && ctrl->target_block)
      validate_log(state,
         /* Check if instruction modifiers are valid. */
   if (!rogue_mods_supported(ctrl->mod, info->supported_op_mods))
            if (!validate_ctrl_op_mod_combo(ctrl->mod))
            /* Instruction repeat checks. */
   if (ctrl->instr.repeat > 1 && !info->dst_repeat_mask &&
      !info->src_repeat_mask) {
               /* Validate destinations and sources for ungrouped shaders. */
   if (!state->shader->is_grouped) {
      for (unsigned i = 0; i < info->num_dsts; ++i) {
      validate_dst(state,
               &ctrl->dst[i],
   info->supported_dst_types[i],
   i,
               for (unsigned i = 0; i < info->num_srcs; ++i) {
      validate_src(state,
               &ctrl->src[i],
   info->supported_src_types[i],
   i,
                  /* nop.end counts as a end-of-block instruction. */
   if (rogue_instr_is_nop_end(&ctrl->instr))
            /* Control instructions have no end flag to set. */
   if (ctrl->instr.end)
               }
      static bool validate_bitwise_op_mod_combo(uint64_t mods)
   {
      rogue_foreach_mod_in_set (mod, mods) {
               /* Check if any excluded op mods have been included. */
   if (info->exclude & mods)
            /* Check if any required op mods have been missed. */
   if (info->require && !(info->require & mods))
                  }
      static void validate_bitwise_instr(rogue_validation_state *state,
         {
      if (bitwise->op == ROGUE_BITWISE_OP_INVALID ||
      bitwise->op >= ROGUE_BITWISE_OP_COUNT)
                  /* Check if instruction modifiers are valid. */
   if (!rogue_mods_supported(bitwise->mod, info->supported_op_mods))
            if (!validate_bitwise_op_mod_combo(bitwise->mod))
            /* Instruction repeat checks. */
   if (bitwise->instr.repeat > 1 && !info->dst_repeat_mask &&
      !info->src_repeat_mask) {
               /* Validate destinations and sources for ungrouped shaders. */
   if (!state->shader->is_grouped) {
      for (unsigned i = 0; i < info->num_dsts; ++i) {
      validate_dst(state,
               &bitwise->dst[i],
   info->supported_dst_types[i],
   i,
               for (unsigned i = 0; i < info->num_srcs; ++i) {
      validate_src(state,
               &bitwise->src[i],
   info->supported_src_types[i],
   i,
            }
      /* Returns true if instruction can end block. */
   static bool validate_instr(rogue_validation_state *state,
         {
                        switch (instr->type) {
   case ROGUE_INSTR_TYPE_ALU:
      validate_alu_instr(state, rogue_instr_as_alu(instr));
         case ROGUE_INSTR_TYPE_BACKEND:
      validate_backend_instr(state, rogue_instr_as_backend(instr));
         case ROGUE_INSTR_TYPE_CTRL:
      ends_block = validate_ctrl_instr(state, rogue_instr_as_ctrl(instr));
         case ROGUE_INSTR_TYPE_BITWISE:
      validate_bitwise_instr(state, rogue_instr_as_bitwise(instr));
         default:
      validate_log(state,
                     /* If the last instruction isn't control flow but has the end flag set, it
   * can end a block. */
   if (!ends_block)
                        }
      /* Returns true if instruction can end block. */
   static bool validate_instr_group(rogue_validation_state *state,
         {
      state->ctx.group = group;
   /* TODO: Validate group properties. */
                     /* Validate instructions in group. */
   /* TODO: Check util_last_bit group_phases < bla bla */
   rogue_foreach_phase_in_set (p, group->header.phases) {
               if (!instr)
            /* TODO NEXT: Groups that have control instructions should only have a
   * single instruction. */
                        if (group->header.alu != ROGUE_ALU_CONTROL)
               }
      static void validate_block(rogue_validation_state *state,
         {
      /* TODO: Validate block properties. */
            if (list_is_empty(&block->instrs)) {
      validate_log(state, "Block is empty.");
   state->ctx.block = NULL;
               unsigned block_ends = 0;
   struct list_head *block_end = NULL;
            /* Validate instructions/groups in block. */
   if (!block->shader->is_grouped) {
      rogue_foreach_instr_in_block (instr, block) {
      bool ends_block = validate_instr(state, instr);
   block_ends += ends_block;
         } else {
      rogue_foreach_instr_group_in_block (group, block) {
      bool ends_block = validate_instr_group(state, group);
   block_ends += ends_block;
                  if (!block_ends || block_ends > 1)
      validate_log(state,
      else if (block_end != last)
      validate_log(
                  }
      static void validate_reg_use(rogue_validation_state *state,
               {
      /* No restrictions. */
   if (!supported_io_srcs)
                     rogue_foreach_phase_in_set (p, rogue_instr_supported_phases(instr)) {
      enum rogue_io io_src = rogue_instr_src_io_src(instr, p, use->src_index);
   if (io_src == ROGUE_IO_INVALID)
            if (!rogue_io_supported(io_src, supported_io_srcs))
      validate_log(state,
               "Register class unsupported in S%u.",
   io_src - ROGUE_IO_S0); /* TODO: Either add info here to
      }
      static void validate_reg_state(rogue_validation_state *state,
         {
               for (enum rogue_reg_class class = 0; class < ROGUE_REG_CLASS_COUNT;
      ++class) {
   const rogue_reg_info *info = &rogue_reg_infos[class];
   if (info->num)
                  rogue_foreach_reg (reg, shader, class) {
      /* Ensure that the range restrictions are satisfied. */
                  /* Ensure that only registers of this class are in the regs list. */
   if (reg->class != class)
      validate_log(state,
                     /* Track the registers used in the class. */
                  /* Check register cache entry. */
   rogue_reg **reg_cached =
         if (!reg_cached || !*reg_cached)
      validate_log(state,
                  else if (*reg_cached != reg || (*reg_cached)->index != reg->index ||
            validate_log(state,
                  else if (reg_cached != reg->cached)
      validate_log(state,
                     /* Validate register uses. */
   const rogue_reg_info *reg_info = &rogue_reg_infos[class];
   rogue_foreach_reg_use (use, reg)
               /* Check that the registers used matches the usage list. */
   if (info->num && memcmp(shader->regs_used[class],
                                    /* Check that SSA registers aren't being written to more than once. */
   rogue_foreach_reg (reg, shader, ROGUE_REG_CLASS_SSA)
      if (list_length(&reg->writes) > 1)
      validate_log(state,
            rogue_foreach_regarray (regarray, shader) {
      /* Validate regarray contents. */
            /* Check regarray cache entry. */
   uint64_t key = rogue_regarray_cache_key(regarray->size,
                           rogue_regarray **regarray_cached =
         if (!regarray_cached || !*regarray_cached)
         else if (*regarray_cached != regarray ||
            (*regarray_cached)->size != regarray->size ||
   (*regarray_cached)->parent != regarray->parent ||
      else if (regarray_cached != regarray->cached)
            if (regarray->parent && (regarray->parent->size <= regarray->size ||
               }
      PUBLIC
   bool rogue_validate_shader(rogue_shader *shader, const char *when)
   {
      if (ROGUE_DEBUG(VLD_SKIP))
                                       /* TODO: Ensure there is at least one block (with at least an end
   * instruction!) */
   rogue_foreach_block (block, shader)
                                 }
