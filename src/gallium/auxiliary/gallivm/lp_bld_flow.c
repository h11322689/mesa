   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * LLVM control flow build helpers.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
      #include "util/u_debug.h"
   #include "util/u_memory.h"
      #include "lp_bld_init.h"
   #include "lp_bld_type.h"
   #include "lp_bld_flow.h"
         /**
   * Insert a new block, right where builder is pointing to.
   *
   * This is useful important not only for aesthetic reasons, but also for
   * performance reasons, as frequently run blocks should be laid out next to
   * each other and fall-throughs maximized.
   *
   * See also llvm/lib/Transforms/Scalar/BasicBlockPlacement.cpp.
   *
   * Note: this function has no dependencies on the flow code and could
   * be used elsewhere.
   */
   LLVMBasicBlockRef
   lp_build_insert_new_block(struct gallivm_state *gallivm, const char *name)
   {
      LLVMBasicBlockRef current_block;
   LLVMBasicBlockRef next_block;
            /* get current basic block */
            /* check if there's another block after this one */
   next_block = LLVMGetNextBasicBlock(current_block);
   if (next_block) {
      /* insert the new block before the next block */
      }
   else {
      /* append new block after current block */
   LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
                  }
         /**
   * Begin a "skip" block.  Inside this block we can test a condition and
   * skip to the end of the block if the condition is false.
   */
   void
   lp_build_flow_skip_begin(struct lp_build_skip_context *skip,
         {
      skip->gallivm = gallivm;
   /* create new basic block */
      }
         /**
   * Insert code to test a condition and branch to the end of the current
   * skip block if the condition is true.
   */
   void
   lp_build_flow_skip_cond_break(struct lp_build_skip_context *skip,
         {
                        /* if cond is true, goto skip->block, else goto new_block */
               }
         void
   lp_build_flow_skip_end(struct lp_build_skip_context *skip)
   {
      /* goto block */
   LLVMBuildBr(skip->gallivm->builder, skip->block);
      }
         /**
   * Check if the mask predicate is zero.  If so, jump to the end of the block.
   */
   void
   lp_build_mask_check(struct lp_build_mask_context *mask)
   {
      LLVMBuilderRef builder = mask->skip.gallivm->builder;
   LLVMValueRef value;
                     /*
   * XXX this doesn't quite generate the most efficient code possible, if
   * the masks are vectors which have all bits set to the same value
   * in each element.
   * movmskps/pmovmskb would be more efficient to get the required value
   * into ordinary reg (certainly with 8 floats).
   * Not sure if llvm could figure that out on its own.
            /* cond = (mask == 0) */
   cond = LLVMBuildICmp(builder,
                              /* if cond, goto end of block */
      }
         /**
   * Begin a section of code which is predicated on a mask.
   * \param mask  the mask context, initialized here
   * \param flow  the flow context
   * \param type  the type of the mask
   * \param value  storage for the mask
   */
   void
   lp_build_mask_begin(struct lp_build_mask_context *mask,
                     {
               mask->reg_type = LLVMIntTypeInContext(gallivm->context, type.width * type.length);
   mask->var_type = lp_build_int_vec_type(gallivm, type);
   mask->var = lp_build_alloca(gallivm,
                              }
         LLVMValueRef
   lp_build_mask_value(struct lp_build_mask_context *mask)
   {
         }
         /**
   * Update boolean mask with given value (bitwise AND).
   * Typically used to update the quad's pixel alive/killed mask
   * after depth testing, alpha testing, TGSI_OPCODE_KILL_IF, etc.
   */
   void
   lp_build_mask_update(struct lp_build_mask_context *mask,
         {
      value = LLVMBuildAnd(mask->skip.gallivm->builder,
                  }
      /*
   * Update boolean mask with given value.
   * Used for per-sample shading to force per-sample execution masks.
   */
   void
   lp_build_mask_force(struct lp_build_mask_context *mask,
         {
         }
      /**
   * End section of code which is predicated on a mask.
   */
   LLVMValueRef
   lp_build_mask_end(struct lp_build_mask_context *mask)
   {
      lp_build_flow_skip_end(&mask->skip);
      }
            void
   lp_build_loop_begin(struct lp_build_loop_state *state,
                     {
                        state->counter_type = LLVMTypeOf(start);
   state->counter_var = lp_build_alloca(gallivm, state->counter_type, "loop_counter");
                                          }
         void
   lp_build_loop_end_cond(struct lp_build_loop_state *state,
                     {
      LLVMBuilderRef builder = state->gallivm->builder;
   LLVMValueRef next;
   LLVMValueRef cond;
            if (!step)
                                                                     }
      void
   lp_build_loop_force_set_counter(struct lp_build_loop_state *state,
         {
      LLVMBuilderRef builder = state->gallivm->builder;
      }
      void
   lp_build_loop_force_reload_counter(struct lp_build_loop_state *state)
   {
      LLVMBuilderRef builder = state->gallivm->builder;
      }
      void
   lp_build_loop_end(struct lp_build_loop_state *state,
               {
         }
      /**
   * Creates a c-style for loop,
   * contrasts lp_build_loop as this checks condition on entry
   * e.g. for(i = start; i cmp_op end; i += step)
   * \param state      the for loop state, initialized here
   * \param gallivm    the gallivm state
   * \param start      starting value of iterator
   * \param cmp_op     comparison operator used for comparing current value with end value
   * \param end        value used to compare against iterator
   * \param step       value added to iterator at end of each loop
   */
   void
   lp_build_for_loop_begin(struct lp_build_for_loop_state *state,
                           struct gallivm_state *gallivm,
   {
               assert(LLVMTypeOf(start) == LLVMTypeOf(end));
            state->begin = lp_build_insert_new_block(gallivm, "loop_begin");
   state->step  = step;
   state->counter_type = LLVMTypeOf(start);
   state->counter_var = lp_build_alloca(gallivm, state->counter_type, "loop_counter");
   state->gallivm = gallivm;
   state->cond = cmp_op;
            LLVMBuildStore(builder, start, state->counter_var);
            LLVMPositionBuilderAtEnd(builder, state->begin);
            state->body = lp_build_insert_new_block(gallivm, "loop_body");
      }
      /**
   * End the for loop.
   */
   void
   lp_build_for_loop_end(struct lp_build_for_loop_state *state)
   {
      LLVMValueRef next, cond;
            next = LLVMBuildAdd(builder, state->counter, state->step, "");
   LLVMBuildStore(builder, next, state->counter_var);
                     /*
   * We build the comparison for the begin block here,
   * if we build it earlier the output llvm ir is not human readable
   * as the code produced is not in the standard begin -> body -> end order.
   */
   LLVMPositionBuilderAtEnd(builder, state->begin);
   cond = LLVMBuildICmp(builder, state->cond, state->counter, state->end, "");
               }
         /*
   Example of if/then/else building:
         int x;
   if (cond) {
         }
   else {
               Is built with:
         // x needs an alloca variable
               lp_build_if(ctx, builder, cond);
         lp_build_else(ctx);
               */
            /**
   * Begin an if/else/endif construct.
   */
   void
   lp_build_if(struct lp_build_if_state *ifthen,
               {
               memset(ifthen, 0, sizeof *ifthen);
   ifthen->gallivm = gallivm;
   ifthen->condition = condition;
            /* create endif/merge basic block for the phi functions */
            /* create/insert true_block before merge_block */
   ifthen->true_block =
      LLVMInsertBasicBlockInContext(gallivm->context,
               /* successive code goes into the true block */
      }
         /**
   * Begin else-part of a conditional
   */
   void
   lp_build_else(struct lp_build_if_state *ifthen)
   {
               /* Append an unconditional Br(anch) instruction on the true_block */
            /* create/insert false_block before the merge block */
   ifthen->false_block =
      LLVMInsertBasicBlockInContext(ifthen->gallivm->context,
               /* successive code goes into the else block */
      }
         /**
   * End a conditional.
   */
   void
   lp_build_endif(struct lp_build_if_state *ifthen)
   {
               /* Insert branch to the merge block from current block */
            /*
   * Now patch in the various branch instructions.
            /* Insert the conditional branch instruction at the end of entry_block */
   LLVMPositionBuilderAtEnd(builder, ifthen->entry_block);
   if (ifthen->false_block) {
      /* we have an else clause */
   LLVMBuildCondBr(builder, ifthen->condition,
      }
   else {
      /* no else clause */
   LLVMBuildCondBr(builder, ifthen->condition,
               /* Resume building code at end of the ifthen->merge_block */
      }
         static LLVMBuilderRef
   create_builder_at_entry(struct gallivm_state *gallivm)
   {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
   LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
   LLVMBasicBlockRef first_block = LLVMGetEntryBasicBlock(function);
   LLVMValueRef first_instr = LLVMGetFirstInstruction(first_block);
            if (first_instr) {
         } else {
                     }
         /**
   * Allocate a scalar (or vector) variable.
   *
   * Although not strictly part of control flow, control flow has deep impact in
   * how variables should be allocated.
   *
   * The mem2reg optimization pass is the recommended way to dealing with mutable
   * variables, and SSA. It looks for allocas and if it can handle them, it
   * promotes them, but only looks for alloca instructions in the entry block of
   * the function. Being in the entry block guarantees that the alloca is only
   * executed once, which makes analysis simpler.
   *
   * See also:
   * - http://www.llvm.org/docs/tutorial/OCamlLangImpl7.html#memory
   */
   LLVMValueRef
   lp_build_alloca(struct gallivm_state *gallivm,
               {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMBuilderRef first_builder = create_builder_at_entry(gallivm);
            res = LLVMBuildAlloca(first_builder, type, name);
                        }
         /**
   * Like lp_build_alloca, but do not zero-initialize the variable.
   */
   LLVMValueRef
   lp_build_alloca_undef(struct gallivm_state *gallivm,
               {
      LLVMBuilderRef first_builder = create_builder_at_entry(gallivm);
                                 }
         /**
   * Allocate an array of scalars/vectors.
   *
   * mem2reg pass is not capable of promoting structs or arrays to registers, but
   * we still put it in the first block anyway as failure to put allocas in the
   * first block may prevent the X86 backend from successfully align the stack as
   * required.
   *
   * Also the scalarrepl pass is supposedly more powerful and can promote
   * arrays in many cases.
   *
   * See also:
   * - http://www.llvm.org/docs/tutorial/OCamlLangImpl7.html#memory
   */
   LLVMValueRef
   lp_build_array_alloca(struct gallivm_state *gallivm,
                     {
      LLVMBuilderRef first_builder = create_builder_at_entry(gallivm);
                                 }
