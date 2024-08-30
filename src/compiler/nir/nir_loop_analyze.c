   /*
   * Copyright © 2015 Thomas Helland
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
      #include "nir_loop_analyze.h"
   #include "util/bitset.h"
   #include "nir.h"
   #include "nir_constant_expressions.h"
      typedef enum {
      undefined,
   invariant,
   not_invariant,
      } nir_loop_variable_type;
      typedef struct {
      /* A link for the work list */
                     /* The ssa_def associated with this info */
            /* The type of this ssa_def */
            /* True if variable is in an if branch */
            /* True if variable is in a nested loop */
            /* Could be a basic_induction if following uniforms are inlined */
   nir_src *init_src;
            /**
   * SSA def of the phi-node associated with this induction variable.
   *
   * Every loop induction variable has an associated phi node in the loop
   * header. This may point to the same SSA def as \c def. If, however, \c def
   * is the increment of the induction variable, this will point to the SSA
   * def being incremented.
   */
      } nir_loop_variable;
      typedef struct {
      /* The loop we store information for */
            /* Loop_variable for all ssa_defs in function */
   nir_loop_variable *loop_vars;
            /* A list of the loop_vars to analyze */
                        } loop_info_state;
      static nir_loop_variable *
   get_loop_var(nir_def *value, loop_info_state *state)
   {
               if (!BITSET_TEST(state->loop_vars_init, value->index)) {
      var->in_loop = false;
   var->def = value;
   var->in_if_branch = false;
   var->in_nested_loop = false;
   var->init_src = NULL;
   var->update_src = NULL;
   if (value->parent_instr->type == nir_instr_type_load_const)
         else
                           }
      typedef struct {
      loop_info_state *state;
   bool in_if_branch;
      } init_loop_state;
      static bool
   init_loop_def(nir_def *def, void *void_init_loop_state)
   {
      init_loop_state *loop_init_state = void_init_loop_state;
            if (loop_init_state->in_nested_loop) {
         } else if (loop_init_state->in_if_branch) {
         } else {
      /* Add to the tail of the list. That way we start at the beginning of
   * the defs in the loop instead of the end when walking the list. This
   * means less recursive calls. Only add defs that are not in nested
   * loops or conditional blocks.
   */
                           }
      /** Calculate an estimated cost in number of instructions
   *
   * We do this so that we don't unroll loops which will later get massively
   * inflated due to int64 or fp64 lowering.  The estimates provided here don't
   * have to be massively accurate; they just have to be good enough that loop
   * unrolling doesn't cause things to blow up too much.
   */
   static unsigned
   instr_cost(loop_info_state *state, nir_instr *instr,
         {
      if (instr->type == nir_instr_type_intrinsic ||
      instr->type == nir_instr_type_tex)
         if (instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(instr);
   const nir_op_info *info = &nir_op_infos[alu->op];
            if (nir_op_is_selection(alu->op)) {
      nir_scalar cond_scalar = { alu->src[0].src.ssa, 0 };
   if (nir_is_terminator_condition_with_two_inputs(cond_scalar)) {
                     nir_scalar rhs, lhs;
                  /* If the selects condition is a comparision between a constant and
   * a basic induction variable we know that it will be eliminated once
   * the loop is unrolled so here we assign it a cost of 0.
   */
   if ((nir_src_is_const(sel_alu->src[0].src) &&
      get_loop_var(rhs.def, state)->type == basic_induction) ||
   (nir_src_is_const(sel_alu->src[1].src) &&
   get_loop_var(lhs.def, state)->type == basic_induction)) {
   /* Also if the selects condition is only used by the select then
   * remove that alu instructons cost from the cost total also.
   */
   if (!list_is_singular(&sel_alu->def.uses) ||
      nir_def_used_by_if(&sel_alu->def))
      else
                     if (alu->op == nir_op_flrp) {
      if ((options->lower_flrp16 && alu->def.bit_size == 16) ||
      (options->lower_flrp32 && alu->def.bit_size == 32) ||
   (options->lower_flrp64 && alu->def.bit_size == 64))
            /* Assume everything 16 or 32-bit is cheap.
   *
   * There are no 64-bit ops that don't have a 64-bit thing as their
   * destination or first source.
   */
   if (alu->def.bit_size < 64 &&
      nir_src_bit_size(alu->src[0].src) < 64)
         bool is_fp64 = alu->def.bit_size == 64 &&
         for (unsigned i = 0; i < info->num_inputs; i++) {
      if (nir_src_bit_size(alu->src[i].src) == 64 &&
      nir_alu_type_get_base_type(info->input_types[i]) == nir_type_float)
            if (is_fp64) {
      /* If it's something lowered normally, it's expensive. */
   if (options->lower_doubles_options &
                  /* If it's full software, it's even more expensive */
   if (options->lower_doubles_options & nir_lower_fp64_full_software) {
      cost *= 100;
                  } else {
      if (options->lower_int64_options &
      nir_lower_int64_op_to_options_mask(alu->op)) {
   /* These require a doing the division algorithm. */
   if (alu->op == nir_op_idiv || alu->op == nir_op_udiv ||
      alu->op == nir_op_imod || alu->op == nir_op_umod ||
               /* Other int64 lowering isn't usually all that expensive */
                     }
      static bool
   init_loop_block(nir_block *block, loop_info_state *state,
         {
      init_loop_state init_state = { .in_if_branch = in_if_branch,
                  nir_foreach_instr(instr, block) {
                     }
      static inline bool
   is_var_alu(nir_loop_variable *var)
   {
         }
      static inline bool
   is_var_phi(nir_loop_variable *var)
   {
         }
      static inline bool
   mark_invariant(nir_def *def, loop_info_state *state)
   {
               if (var->type == invariant)
            if (!var->in_loop) {
      var->type = invariant;
               if (var->type == not_invariant)
            if (is_var_alu(var)) {
               for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      if (!mark_invariant(alu->src[i].src.ssa, state)) {
      var->type = not_invariant;
         }
   var->type = invariant;
               /* Phis shouldn't be invariant except if one operand is invariant, and the
   * other is the phi itself. These should be removed by opt_remove_phis.
   * load_consts are already set to invariant and constant during init,
   * and so should return earlier. Remaining op_codes are set undefined.
   */
   var->type = not_invariant;
      }
      static void
   compute_invariance_information(loop_info_state *state)
   {
      /* An expression is invariant in a loop L if:
   *  (base cases)
   *    – it’s a constant
   *    – it’s a variable use, all of whose single defs are outside of L
   *  (inductive cases)
   *    – it’s a pure computation all of whose args are loop invariant
   *    – it’s a variable use whose single reaching def, and the
   *      rhs of that def is loop-invariant
   */
   list_for_each_entry_safe(nir_loop_variable, var, &state->process_list,
                     if (mark_invariant(var->def, state))
         }
      /* If all of the instruction sources point to identical ALU instructions (as
   * per nir_instrs_equal), return one of the ALU instructions.  Otherwise,
   * return NULL.
   */
   static nir_alu_instr *
   phi_instr_as_alu(nir_phi_instr *phi)
   {
      nir_alu_instr *first = NULL;
   nir_foreach_phi_src(src, phi) {
      if (src->src.ssa->parent_instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(src->src.ssa->parent_instr);
   if (first == NULL) {
         } else {
      if (!nir_instrs_equal(&first->instr, &alu->instr))
                     }
      static bool
   alu_src_has_identity_swizzle(nir_alu_instr *alu, unsigned src_idx)
   {
      assert(nir_op_infos[alu->op].input_sizes[src_idx] == 0);
   for (unsigned i = 0; i < alu->def.num_components; i++) {
      if (alu->src[src_idx].swizzle[i] != i)
                  }
      static bool
   is_only_uniform_src(nir_src *src)
   {
               switch (instr->type) {
   case nir_instr_type_alu: {
      /* Return true if all sources return true. */
   nir_alu_instr *alu = nir_instr_as_alu(instr);
   for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      if (!is_only_uniform_src(&alu->src[i].src))
      }
               case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *inst = nir_instr_as_intrinsic(instr);
   /* current uniform inline only support load ubo */
               case nir_instr_type_load_const:
      /* Always return true for constants. */
         default:
            }
      static bool
   compute_induction_information(loop_info_state *state)
   {
               list_for_each_entry_safe(nir_loop_variable, var, &state->process_list,
               /* It can't be an induction variable if it is invariant. Invariants and
   * things in nested loops or conditionals should have been removed from
   * the list by compute_invariance_information().
   */
   assert(!var->in_if_branch && !var->in_nested_loop &&
            /* We are only interested in checking phis for the basic induction
   * variable case as its simple to detect. All basic induction variables
   * have a phi node
   */
   if (!is_var_phi(var))
                     nir_loop_variable *alu_src_var = NULL;
   nir_foreach_phi_src(src, phi) {
               /* If one of the sources is in an if branch or nested loop then don't
   * attempt to go any further.
   */
                  /* Detect inductions variables that are incremented in both branches
   * of an unnested if rather than in a loop block.
   */
   if (is_var_phi(src_var)) {
      nir_phi_instr *src_phi =
         nir_alu_instr *src_phi_alu = phi_instr_as_alu(src_phi);
   if (src_phi_alu) {
      src_var = get_loop_var(&src_phi_alu->def, state);
   if (!src_var->in_if_branch)
                  if (!src_var->in_loop && !var->init_src) {
         } else if (is_var_alu(src_var) && !var->update_src) {
                     /* Check for unsupported alu operations */
   if (alu->op != nir_op_iadd && alu->op != nir_op_fadd &&
      alu->op != nir_op_imul && alu->op != nir_op_fmul &&
                     if (nir_op_infos[alu->op].num_inputs == 2) {
      for (unsigned i = 0; i < 2; i++) {
      /* Is one of the operands const or uniform, and the other the phi.
   * The phi source can't be swizzled in any way.
   */
   if (alu->src[1 - i].src.ssa == &phi->def &&
      alu_src_has_identity_swizzle(alu, 1 - i)) {
   if (is_only_uniform_src(&alu->src[i].src))
                     if (!var->update_src)
      } else {
      var->update_src = NULL;
                  if (var->update_src && var->init_src &&
      is_only_uniform_src(var->init_src)) {
   alu_src_var->init_src = var->init_src;
   alu_src_var->update_src = var->update_src;
                                    } else {
      var->init_src = NULL;
   var->update_src = NULL;
                  nir_loop_info *info = state->loop->info;
   ralloc_free(info->induction_vars);
            /* record induction variables into nir_loop_info */
   if (num_induction_vars) {
      info->induction_vars = ralloc_array(info, nir_loop_induction_variable,
            list_for_each_entry(nir_loop_variable, var, &state->process_list,
            if (var->type == basic_induction) {
      nir_loop_induction_variable *ivar =
         ivar->def = var->def;
   ivar->init_src = var->init_src;
         }
   /* don't overflow */
                  }
      static bool
   find_loop_terminators(loop_info_state *state)
   {
      bool success = false;
   foreach_list_typed_safe(nir_cf_node, node, node, &state->loop->body) {
      if (node->type == nir_cf_node_if) {
               nir_block *break_blk = NULL;
                  nir_block *last_then = nir_if_last_then_block(nif);
   nir_block *last_else = nir_if_last_else_block(nif);
   if (nir_block_ends_in_break(last_then)) {
      break_blk = last_then;
   continue_from_blk = last_else;
      } else if (nir_block_ends_in_break(last_else)) {
      break_blk = last_else;
               /* If there is a break then we should find a terminator. If we can
   * not find a loop terminator, but there is a break-statement then
   * we should return false so that we do not try to find trip-count
   */
   if (!nir_is_trivial_loop_if(nif, break_blk)) {
      state->loop->info->complex_loop = true;
               /* Continue if the if contained no jumps at all */
                  if (nif->condition.ssa->parent_instr->type == nir_instr_type_phi) {
      state->loop->info->complex_loop = true;
                                             terminator->nif = nif;
   terminator->break_block = break_blk;
   terminator->continue_from_block = continue_from_blk;
                                    }
      /* This function looks for an array access within a loop that uses an
   * induction variable for the array index. If found it returns the size of the
   * array, otherwise 0 is returned. If we find an induction var we pass it back
   * to the caller via array_index_out.
   */
   static unsigned
   find_array_access_via_induction(loop_info_state *state,
               {
      for (nir_deref_instr *d = deref; d; d = nir_deref_instr_parent(d)) {
      if (d->deref_type != nir_deref_type_array)
                     if (array_index->type != basic_induction)
            if (array_index_out)
                     if (glsl_type_is_array_or_matrix(parent->type)) {
         } else {
      assert(glsl_type_is_vector(parent->type));
                     }
      static bool
   guess_loop_limit(loop_info_state *state, nir_const_value *limit_val,
         {
               nir_foreach_block_in_cf_node(block, &state->loop->cf_node) {
      nir_foreach_instr(instr, block) {
                              /* Check for arrays variably-indexed by a loop induction variable. */
   if (intrin->intrinsic == nir_intrinsic_load_deref ||
                     nir_loop_variable *array_idx = NULL;
   unsigned array_size =
      find_array_access_via_induction(state,
            if (array_idx && basic_ind.def == array_idx->def &&
      (min_array_size == 0 || min_array_size > array_size)) {
   /* Array indices are scalars */
                                    array_size =
      find_array_access_via_induction(state,
            if (array_idx && basic_ind.def == array_idx->def &&
      (min_array_size == 0 || min_array_size > array_size)) {
   /* Array indices are scalars */
   assert(basic_ind.def->num_components == 1);
                        if (min_array_size) {
      *limit_val = nir_const_value_for_uint(min_array_size,
                        }
      static bool
   try_find_limit_of_alu(nir_scalar limit, nir_const_value *limit_val,
         {
      if (!nir_scalar_is_alu(limit))
            nir_op limit_op = nir_scalar_alu_op(limit);
   if (limit_op == nir_op_imin || limit_op == nir_op_fmin) {
      for (unsigned i = 0; i < 2; i++) {
      nir_scalar src = nir_scalar_chase_alu_src(limit, i);
   if (nir_scalar_is_const(src)) {
      *limit_val = nir_scalar_as_const_value(src);
   terminator->exact_trip_count_unknown = true;
                        }
      static nir_const_value
   eval_const_unop(nir_op op, unsigned bit_size, nir_const_value src0,
         {
      assert(nir_op_infos[op].num_inputs == 1);
   nir_const_value dest;
   nir_const_value *src[1] = { &src0 };
   nir_eval_const_opcode(op, &dest, 1, bit_size, src, execution_mode);
      }
      static nir_const_value
   eval_const_binop(nir_op op, unsigned bit_size,
               {
      assert(nir_op_infos[op].num_inputs == 2);
   nir_const_value dest;
   nir_const_value *src[2] = { &src0, &src1 };
   nir_eval_const_opcode(op, &dest, 1, bit_size, src, execution_mode);
      }
      static int
   find_replacement(const nir_def **originals, const nir_def *key,
         {
      for (int i = 0; i < num_replacements; i++) {
      if (originals[i] == key)
                  }
      /**
   * Try to evaluate an ALU instruction as a constant with a replacement
   *
   * Much like \c nir_opt_constant_folding.c:try_fold_alu, this method attempts
   * to evaluate an ALU instruction as a constant. There are two significant
   * differences.
   *
   * First, this method performs the evaluation recursively. If any source of
   * the ALU instruction is not itself a constant, it is first evaluated.
   *
   * Second, if the SSA value \c original is encountered as a source of the ALU
   * instruction, the value \c replacement is substituted.
   *
   * The intended purpose of this function is to evaluate an arbitrary
   * expression involving a loop induction variable. In this case, \c original
   * would be the phi node associated with the induction variable, and
   * \c replacement is the initial value of the induction variable.
   *
   * \returns true if the ALU instruction can be evaluated as constant (after
   * applying the previously described substitution) or false otherwise.
   */
   static bool
   try_eval_const_alu(nir_const_value *dest, nir_alu_instr *alu,
                     {
               /* In the case that any outputs/inputs have unsized types, then we need to
   * guess the bit-size. In this case, the validator ensures that all
   * bit-sizes match so we can just take the bit-size from first
   * output/input with an unsized type. If all the outputs/inputs are sized
   * then we don't need to guess the bit-size at all because the code we
   * generate for constant opcodes in this case already knows the sizes of
   * the types involved and does not need the provided bit-size for anything
   * (although it still requires to receive a valid bit-size).
   */
   unsigned bit_size = 0;
   if (!nir_alu_type_get_type_size(nir_op_infos[alu->op].output_type))
            for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      if (bit_size == 0 &&
                           if (src_instr->type == nir_instr_type_load_const) {
               for (unsigned j = 0; j < nir_ssa_alu_instr_src_components(alu, i);
      j++) {
         } else {
                     if (r >= 0) {
      for (unsigned j = 0; j < nir_ssa_alu_instr_src_components(alu, i);
      j++) {
                           if (!try_eval_const_alu(src[i], nir_instr_as_alu(src_instr),
                  } else {
                        if (bit_size == 0)
                     for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; ++i)
            nir_eval_const_opcode(alu->op, dest, alu->def.num_components,
               }
      static nir_op
   invert_comparison_if_needed(nir_op alu_op, bool invert)
   {
      if (!invert)
            switch (alu_op) {
      case nir_op_fge:
         case nir_op_ige:
         case nir_op_uge:
         case nir_op_flt:
         case nir_op_ilt:
         case nir_op_ult:
         case nir_op_feq:
         case nir_op_ieq:
         case nir_op_fneu:
         case nir_op_ine:
         default:
         }
      static int32_t
   get_iteration(nir_op cond_op, nir_const_value initial, nir_const_value step,
               {
      nir_const_value span, iter;
            switch (invert_comparison_if_needed(cond_op, invert_cond)) {
   case nir_op_ine:
      /* In order for execution to be here, limit must be the same as initial.
   * Otherwise will_break_on_first_iteration would have returned false.
   * If step is zero, the loop is infinite.  Otherwise the loop will
   * execute once.
   */
         case nir_op_ige:
   case nir_op_ilt:
   case nir_op_ieq:
      span = eval_const_binop(nir_op_isub, bit_size, limit, initial,
         iter = eval_const_binop(nir_op_idiv, bit_size, span, step,
               case nir_op_uge:
   case nir_op_ult:
      span = eval_const_binop(nir_op_isub, bit_size, limit, initial,
         iter = eval_const_binop(nir_op_udiv, bit_size, span, step,
               case nir_op_fneu:
      /* In order for execution to be here, limit must be the same as initial.
   * Otherwise will_break_on_first_iteration would have returned false.
   * If step is zero, the loop is infinite.  Otherwise the loop will
   * execute once.
   *
   * This is a little more tricky for floating point since X-Y might still
   * be X even if Y is not zero.  Instead check that (initial + step) !=
   * initial.
   */
   span = eval_const_binop(nir_op_fadd, bit_size, initial, step,
         iter = eval_const_binop(nir_op_feq, bit_size, initial,
            /* return (initial + step) == initial ? -1 : 1 */
         case nir_op_fge:
   case nir_op_flt:
   case nir_op_feq:
      span = eval_const_binop(nir_op_fsub, bit_size, limit, initial,
         iter = eval_const_binop(nir_op_fdiv, bit_size, span,
         iter = eval_const_unop(nir_op_f2i64, bit_size, iter, execution_mode);
   iter_bit_size = 64;
         default:
                  uint64_t iter_u64 = nir_const_value_as_uint(iter, iter_bit_size);
      }
      static int32_t
   get_iteration_empirical(nir_alu_instr *cond_alu, nir_alu_instr *incr_alu,
                     {
      int iter_count = 0;
   nir_const_value result;
            const nir_def *originals[2] = { basis, NULL };
            while (iter_count <= max_unroll_iterations) {
               success = try_eval_const_alu(&result, cond_alu, originals, replacements,
         if (!success)
            const bool cond_succ = invert_cond ? !result.b : result.b;
   if (cond_succ)
                     success = try_eval_const_alu(&result, incr_alu, originals, replacements,
                                 }
      static bool
   will_break_on_first_iteration(nir_alu_instr *cond_alu, nir_def *basis,
                     {
               const nir_def *originals[2] = { basis, limit_basis };
            ASSERTED bool success = try_eval_const_alu(&result, cond_alu, originals,
                        }
      static bool
   test_iterations(int32_t iter_int, nir_const_value step,
                  nir_const_value limit, nir_op cond_op, unsigned bit_size,
      {
               nir_const_value iter_src;
   nir_op mul_op;
   nir_op add_op;
   switch (induction_base_type) {
   case nir_type_float:
      iter_src = nir_const_value_for_float(iter_int, bit_size);
   mul_op = nir_op_fmul;
   add_op = nir_op_fadd;
      case nir_type_int:
   case nir_type_uint:
      iter_src = nir_const_value_for_int(iter_int, bit_size);
   mul_op = nir_op_imul;
   add_op = nir_op_iadd;
      default:
                  /* Multiple the iteration count we are testing by the number of times we
   * step the induction variable each iteration.
   */
   nir_const_value mul_result =
            /* Add the initial value to the accumulated induction variable total */
   nir_const_value add_result =
            nir_const_value *src[2];
   src[limit_rhs ? 0 : 1] = &add_result;
            /* Evaluate the loop exit condition */
   nir_const_value result;
               }
      static int
   calculate_iterations(nir_def *basis, nir_def *limit_basis,
                        nir_const_value initial, nir_const_value step,
      {
      /* nir_op_isub should have been lowered away by this point */
            /* Make sure the alu type for our induction variable is compatible with the
   * conditional alus input type. If its not something has gone really wrong.
   */
   nir_alu_type induction_base_type =
         if (induction_base_type == nir_type_int || induction_base_type == nir_type_uint) {
      assert(nir_alu_type_get_base_type(nir_op_infos[alu_op].input_types[1]) == nir_type_int ||
      } else {
      assert(nir_alu_type_get_base_type(nir_op_infos[alu_op].input_types[0]) ==
               if (cond.def->num_components != 1 || basis->num_components != 1 ||
      limit_basis->num_components != 1)
         /* do-while loops can increment the starting value before the condition is
   * checked. e.g.
   *
   *    do {
   *        ndx++;
   *     } while (ndx < 3);
   *
   * Here we check if the induction variable is used directly by the loop
   * condition and if so we assume we need to step the initial value.
   */
   unsigned trip_offset = 0;
   nir_alu_instr *cond_alu = nir_instr_as_alu(cond.def->parent_instr);
   if (cond_alu->src[0].src.ssa == &alu->def ||
      cond_alu->src[1].src.ssa == &alu->def) {
                        /* get_iteration works under assumption that iterator will be
   * incremented or decremented until it hits the limit,
   * however if the loop condition is false on the first iteration
   * get_iteration's assumption is broken. Handle such loops first.
   */
   if (will_break_on_first_iteration(cond_alu, basis, limit_basis, initial,
                        /* For loops incremented with addition operation, it's easy to
   * calculate the number of iterations theoretically. Even though it
   * is possible for other operations as well, it is much more error
   * prone, and doesn't cover all possible cases. So, we try to
   * emulate the loop.
   */
   int iter_int;
   switch (alu->op) {
   case nir_op_iadd:
   case nir_op_fadd:
      assert(nir_src_bit_size(alu->src[0].src) ==
            iter_int = get_iteration(alu_op, initial, step, limit, invert_cond,
            case nir_op_fmul:
      /* Detecting non-zero loop counts when the loop increment is floating
   * point multiplication triggers a preexisting problem in
   * glsl-fs-loop-unroll-mul-fp64.shader_test. See
   * https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/3445#note_1779438.
   */
      case nir_op_imul:
   case nir_op_ishl:
   case nir_op_ishr:
   case nir_op_ushr:
      return get_iteration_empirical(cond_alu, alu, basis, initial,
            default:
                  /* If iter_int is negative the loop is ill-formed or is the conditional is
   * unsigned with a huge iteration count so don't bother going any further.
   */
   if (iter_int < 0)
            nir_op actual_alu_op = invert_comparison_if_needed(alu_op, invert_cond);
   if (actual_alu_op == nir_op_ine || actual_alu_op == nir_op_fneu)
            /* An explanation from the GLSL unrolling pass:
   *
   * Make sure that the calculated number of iterations satisfies the exit
   * condition.  This is needed to catch off-by-one errors and some types of
   * ill-formed loops.  For example, we need to detect that the following
   * loop does not have a maximum iteration count.
   *
   *    for (float x = 0.0; x != 0.9; x += 0.2);
   */
   for (int bias = -1; bias <= 1; bias++) {
      const int iter_bias = iter_int + bias;
   if (iter_bias < 1)
            if (test_iterations(iter_bias, step, limit, alu_op, bit_size,
                                    }
      static bool
   get_induction_and_limit_vars(nir_scalar cond,
                           {
      nir_scalar rhs, lhs;
   lhs = nir_scalar_chase_alu_src(cond, 0);
            nir_loop_variable *src0_lv = get_loop_var(lhs.def, state);
            if (src0_lv->type == basic_induction) {
      if (!nir_src_is_const(*src0_lv->init_src))
            *ind = lhs;
   *limit = rhs;
   *limit_rhs = true;
      } else if (src1_lv->type == basic_induction) {
      if (!nir_src_is_const(*src1_lv->init_src))
            *ind = rhs;
   *limit = lhs;
   *limit_rhs = false;
      } else {
            }
      static bool
   try_find_trip_count_vars_in_iand(nir_scalar *cond,
                           {
      const nir_op alu_op = nir_scalar_alu_op(*cond);
                     if (alu_op == nir_op_ieq) {
               if (!nir_scalar_is_alu(iand) || !nir_scalar_is_const(zero)) {
      /* Maybe we had it the wrong way, flip things around */
   nir_scalar tmp = zero;
                  /* If we still didn't find what we need then return */
   if (!nir_scalar_is_const(zero))
               /* If the loop is not breaking on (x && y) == 0 then return */
   if (nir_scalar_as_uint(zero) != 0)
               if (!nir_scalar_is_alu(iand))
            if (nir_scalar_alu_op(iand) != nir_op_iand)
            /* Check if iand src is a terminator condition and try get induction var
   * and trip limit var.
   */
   bool found_induction_var = false;
   for (unsigned i = 0; i < 2; i++) {
      nir_scalar src = nir_scalar_chase_alu_src(iand, i);
   if (nir_is_terminator_condition_with_two_inputs(src) &&
      get_induction_and_limit_vars(src, ind, limit, limit_rhs, state)) {
                  /* If we've found one with a constant limit, stop. */
   if (nir_scalar_is_const(*limit))
                     }
      /* Run through each of the terminators of the loop and try to infer a possible
   * trip-count. We need to check them all, and set the lowest trip-count as the
   * trip-count of our loop. If one of the terminators has an undecidable
   * trip-count we can not safely assume anything about the duration of the
   * loop.
   */
   static void
   find_trip_count(loop_info_state *state, unsigned execution_mode,
         {
      bool trip_count_known = true;
   bool guessed_trip_count = false;
   nir_loop_terminator *limiting_terminator = NULL;
            list_for_each_entry(nir_loop_terminator, terminator,
                           if (!nir_scalar_is_alu(cond)) {
      /* If we get here the loop is dead and will get cleaned up by the
   * nir_opt_dead_cf pass.
   */
   trip_count_known = false;
   terminator->exact_trip_count_unknown = true;
                                 bool limit_rhs;
   nir_scalar basic_ind = { NULL, 0 };
   nir_scalar limit;
   if ((alu_op == nir_op_inot || alu_op == nir_op_ieq) &&
                     /* The loop is exiting on (x && y) == 0 so we need to get the
   * inverse of x or y (i.e. which ever contained the induction var) in
   * order to compute the trip count.
   */
   alu_op = nir_scalar_alu_op(cond);
   invert_cond = !invert_cond;
   trip_count_known = false;
               if (!basic_ind.def) {
      if (nir_is_supported_terminator_condition(cond)) {
      /* Extract and inverse the comparision if it is wrapped in an inot
   */
   if (alu_op == nir_op_inot) {
      cond = nir_scalar_chase_alu_src(cond, 0);
                     get_induction_and_limit_vars(cond, &basic_ind,
                  /* The comparison has to have a basic induction variable for us to be
   * able to find trip counts.
   */
   if (!basic_ind.def) {
      trip_count_known = false;
   terminator->exact_trip_count_unknown = true;
                        /* Attempt to find a constant limit for the loop */
   nir_const_value limit_val;
   if (nir_scalar_is_const(limit)) {
         } else {
               if (!try_find_limit_of_alu(limit, &limit_val, terminator, state)) {
      /* Guess loop limit based on array access */
   if (!guess_loop_limit(state, &limit_val, basic_ind)) {
                                       /* We have determined that we have the following constants:
   * (With the typical int i = 0; i < x; i++; as an example)
   *    - Upper limit.
   *    - Starting value
   *    - Step / iteration size
   * Thats all thats needed to calculate the trip-count
                     /* The basic induction var might be a vector but, because we guarantee
   * earlier that the phi source has a scalar swizzle, we can take the
   * component from basic_ind.
   */
   nir_scalar initial_s = { lv->init_src->ssa, basic_ind.comp };
   nir_scalar alu_s = {
      lv->update_src->src.ssa,
               /* We are not guaranteed by that at one of these sources is a constant.
   * Try to find one.
   */
   if (!nir_scalar_is_const(initial_s) ||
                  nir_const_value initial_val = nir_scalar_as_const_value(initial_s);
            int iterations = calculate_iterations(lv->basis, limit.def,
                                                /* Where we not able to calculate the iteration count */
   if (iterations == -1) {
      trip_count_known = false;
   guessed_trip_count = false;
   terminator->exact_trip_count_unknown = true;
               if (guessed_trip_count) {
      guessed_trip_count = false;
   terminator->exact_trip_count_unknown = true;
   if (state->loop->info->guessed_trip_count == 0 ||
                              /* If this is the first run or we have found a smaller amount of
   * iterations than previously (we have identified a more limiting
   * terminator) set the trip count and limiting terminator.
   */
   if (max_trip_count == -1 || iterations < max_trip_count) {
      max_trip_count = iterations;
                  state->loop->info->exact_trip_count_known = trip_count_known;
   if (max_trip_count > -1)
            }
      static bool
   force_unroll_array_access(loop_info_state *state, nir_deref_instr *deref,
         {
      unsigned array_size = find_array_access_via_induction(state, deref, NULL);
   if (array_size) {
      if ((array_size == state->loop->info->max_trip_count) &&
      nir_deref_mode_must_be(deref, nir_var_shader_in |
                           if (nir_deref_mode_must_be(deref, state->indirect_mask))
            if (contains_sampler && state->force_unroll_sampler_indirect)
                  }
      static bool
   force_unroll_heuristics(loop_info_state *state, nir_block *block)
   {
      nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_tex) {
      nir_tex_instr *tex_instr = nir_instr_as_tex(instr);
   int sampler_idx =
                  if (sampler_idx >= 0) {
      nir_deref_instr *deref =
         if (force_unroll_array_access(state, deref, true))
                  if (instr->type != nir_instr_type_intrinsic)
                     /* Check for arrays variably-indexed by a loop induction variable.
   * Unrolling the loop may convert that access into constant-indexing.
   */
   if (intrin->intrinsic == nir_intrinsic_load_deref ||
      intrin->intrinsic == nir_intrinsic_store_deref ||
   intrin->intrinsic == nir_intrinsic_copy_deref) {
   if (force_unroll_array_access(state,
                        if (intrin->intrinsic == nir_intrinsic_copy_deref &&
      force_unroll_array_access(state,
                              }
      static void
   get_loop_info(loop_info_state *state, nir_function_impl *impl)
   {
      nir_shader *shader = impl->function->shader;
            /* Add all entries in the outermost part of the loop to the processing list
   * Mark the entries in conditionals or in nested loops accordingly
   */
   foreach_list_typed_safe(nir_cf_node, node, node, &state->loop->body) {
               case nir_cf_node_block:
                  case nir_cf_node_if:
      nir_foreach_block_in_cf_node(block, node)
               case nir_cf_node_loop:
      nir_foreach_block_in_cf_node(block, node) {
                     case nir_cf_node_function:
                     /* Try to find all simple terminators of the loop. If we can't find any,
   * or we find possible terminators that have side effects then bail.
   */
   if (!find_loop_terminators(state)) {
      list_for_each_entry_safe(nir_loop_terminator, terminator,
                  list_del(&terminator->loop_terminator_link);
      }
               /* Induction analysis needs invariance information so get that first */
            /* We have invariance information so try to find induction variables */
   if (!compute_induction_information(state))
            /* Run through each of the terminators and try to compute a trip-count */
   find_trip_count(state,
                  nir_foreach_block_in_cf_node(block, &state->loop->cf_node) {
      nir_foreach_instr(instr, block) {
                  if (state->loop->info->force_unroll)
            if (force_unroll_heuristics(state, block)) {
               }
      static loop_info_state *
   initialize_loop_info_state(nir_loop *loop, void *mem_ctx,
         {
      loop_info_state *state = rzalloc(mem_ctx, loop_info_state);
   state->loop_vars = ralloc_array(mem_ctx, nir_loop_variable,
         state->loop_vars_init = rzalloc_array(mem_ctx, BITSET_WORD,
                           if (loop->info)
                                 }
      static void
   process_loops(nir_cf_node *cf_node, nir_variable_mode indirect_mask,
         {
      switch (cf_node->type) {
   case nir_cf_node_block:
         case nir_cf_node_if: {
      nir_if *if_stmt = nir_cf_node_as_if(cf_node);
   foreach_list_typed(nir_cf_node, nested_node, node, &if_stmt->then_list)
         foreach_list_typed(nir_cf_node, nested_node, node, &if_stmt->else_list)
            }
   case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(cf_node);
            foreach_list_typed(nir_cf_node, nested_node, node, &loop->body)
            }
   default:
                  nir_loop *loop = nir_cf_node_as_loop(cf_node);
   nir_function_impl *impl = nir_cf_node_get_function(cf_node);
            loop_info_state *state = initialize_loop_info_state(loop, mem_ctx, impl);
   state->indirect_mask = indirect_mask;
                        }
      void
   nir_loop_analyze_impl(nir_function_impl *impl,
               {
      nir_index_ssa_defs(impl);
   foreach_list_typed(nir_cf_node, node, node, &impl->body)
      }
