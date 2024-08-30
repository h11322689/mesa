   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
   * Copyright 2007-2008 VMware, Inc.
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
      #include "util/u_memory.h"
   #include "lp_bld_type.h"
   #include "lp_bld_init.h"
   #include "lp_bld_flow.h"
   #include "lp_bld_ir_common.h"
   #include "lp_bld_logic.h"
      /*
   * Return the context for the current function.
   * (always 'main', if shader doesn't do any function calls)
   */
   static inline struct function_ctx *
   func_ctx(struct lp_exec_mask *mask)
   {
      assert(mask->function_stack_size > 0);
   assert(mask->function_stack_size <= LP_MAX_NUM_FUNCS);
      }
      /*
   * Returns true if we're in a loop.
   * It's global, meaning that it returns true even if there's
   * no loop inside the current function, but we were inside
   * a loop inside another function, from which this one was called.
   */
   static inline bool
   mask_has_loop(struct lp_exec_mask *mask)
   {
      int i;
   for (i = mask->function_stack_size - 1; i >= 0; --i) {
      const struct function_ctx *ctx = &mask->function_stack[i];
   if (ctx->loop_stack_size > 0)
      }
      }
      /*
   * Returns true if we're inside a switch statement.
   * It's global, meaning that it returns true even if there's
   * no switch in the current function, but we were inside
   * a switch inside another function, from which this one was called.
   */
   static inline bool
   mask_has_switch(struct lp_exec_mask *mask)
   {
      int i;
   for (i = mask->function_stack_size - 1; i >= 0; --i) {
      const struct function_ctx *ctx = &mask->function_stack[i];
   if (ctx->switch_stack_size > 0)
      }
      }
      /*
   * Returns true if we're inside a conditional.
   * It's global, meaning that it returns true even if there's
   * no conditional in the current function, but we were inside
   * a conditional inside another function, from which this one was called.
   */
   static inline bool
   mask_has_cond(struct lp_exec_mask *mask)
   {
      int i;
   for (i = mask->function_stack_size - 1; i >= 0; --i) {
      const struct function_ctx *ctx = &mask->function_stack[i];
   if (ctx->cond_stack_size > 0)
      }
      }
      void lp_exec_mask_update(struct lp_exec_mask *mask)
   {
      LLVMBuilderRef builder = mask->bld->gallivm->builder;
   bool has_loop_mask = mask_has_loop(mask);
   bool has_cond_mask = mask_has_cond(mask);
   bool has_switch_mask = mask_has_switch(mask);
   bool has_ret_mask = mask->function_stack_size > 1 ||
            if (has_loop_mask) {
      /*for loops we need to update the entire mask at runtime */
   LLVMValueRef tmp;
   assert(mask->break_mask);
   tmp = LLVMBuildAnd(builder,
                     mask->exec_mask = LLVMBuildAnd(builder,
                  } else
            if (has_switch_mask) {
      mask->exec_mask = LLVMBuildAnd(builder,
                           if (has_ret_mask) {
      mask->exec_mask = LLVMBuildAnd(builder,
                           mask->has_mask = (has_cond_mask ||
                  }
      /*
   * Initialize a function context at the specified index.
   */
   void
   lp_exec_mask_function_init(struct lp_exec_mask *mask, int function_idx)
   {
      LLVMTypeRef int_type = LLVMInt32TypeInContext(mask->bld->gallivm->context);
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
            ctx->cond_stack_size = 0;
   ctx->loop_stack_size = 0;
   ctx->bgnloop_stack_size = 0;
            if (function_idx == 0) {
                  ctx->loop_limiter = lp_build_alloca(mask->bld->gallivm,
         LLVMBuildStore(
      builder,
   LLVMConstInt(int_type, LP_MAX_TGSI_LOOP_ITERATIONS, false),
   }
      void lp_exec_mask_init(struct lp_exec_mask *mask, struct lp_build_context *bld)
   {
      mask->bld = bld;
   mask->has_mask = false;
   mask->ret_in_main = false;
   /* For the main function */
            mask->int_vec_type = lp_build_int_vec_type(bld->gallivm, mask->bld->type);
   mask->exec_mask = mask->ret_mask = mask->break_mask = mask->cont_mask =
                  mask->function_stack = CALLOC(LP_MAX_NUM_FUNCS,
            }
      void
   lp_exec_mask_fini(struct lp_exec_mask *mask)
   {
         }
      /* stores val into an address pointed to by dst_ptr.
   * mask->exec_mask is used to figure out which bits of val
   * should be stored into the address
   * (0 means don't store this bit, 1 means do store).
   */
   void lp_exec_mask_store(struct lp_exec_mask *mask,
                     {
      LLVMBuilderRef builder = mask->bld->gallivm->builder;
            assert(lp_check_value(bld_store->type, val));
   assert(LLVMGetTypeKind(LLVMTypeOf(dst_ptr)) == LLVMPointerTypeKind);
   assert(LLVM_VERSION_MAJOR >= 15
                  if (exec_mask) {
               dst = LLVMBuildLoad2(builder, LLVMTypeOf(val), dst_ptr, "");
   if (bld_store->type.width < 32)
         res = lp_build_select(bld_store, exec_mask, val, dst);
      } else
      }
      void lp_exec_bgnloop_post_phi(struct lp_exec_mask *mask)
   {
      LLVMBuilderRef builder = mask->bld->gallivm->builder;
            if (ctx->loop_stack_size != ctx->bgnloop_stack_size) {
      mask->break_mask = LLVMBuildLoad2(builder, mask->int_vec_type, ctx->break_var, "");
   lp_exec_mask_update(mask);
         }
      void lp_exec_bgnloop(struct lp_exec_mask *mask, bool load)
   {
      LLVMBuilderRef builder = mask->bld->gallivm->builder;
            if (ctx->loop_stack_size >= LP_MAX_TGSI_NESTING) {
      ++ctx->loop_stack_size;
               ctx->break_type_stack[ctx->loop_stack_size + ctx->switch_stack_size] =
                  ctx->loop_stack[ctx->loop_stack_size].loop_block = ctx->loop_block;
   ctx->loop_stack[ctx->loop_stack_size].cont_mask = mask->cont_mask;
   ctx->loop_stack[ctx->loop_stack_size].break_mask = mask->break_mask;
   ctx->loop_stack[ctx->loop_stack_size].break_var = ctx->break_var;
            ctx->break_var = lp_build_alloca(mask->bld->gallivm, mask->int_vec_type, "");
                     LLVMBuildBr(builder, ctx->loop_block);
            if (load) {
            }
      void lp_exec_endloop(struct gallivm_state *gallivm,
         {
      LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);
   LLVMBasicBlockRef endloop;
   LLVMTypeRef int_type = LLVMInt32TypeInContext(mask->bld->gallivm->context);
   LLVMTypeRef reg_type = LLVMIntTypeInContext(gallivm->context,
                                 assert(ctx->loop_stack_size);
   if (ctx->loop_stack_size > LP_MAX_TGSI_NESTING) {
      --ctx->loop_stack_size;
   --ctx->bgnloop_stack_size;
               /*
   * Restore the cont_mask, but don't pop
   */
   mask->cont_mask = ctx->loop_stack[ctx->loop_stack_size - 1].cont_mask;
            /*
   * Unlike the continue mask, the break_mask must be preserved across loop
   * iterations
   */
            /* Decrement the loop limiter */
            limiter = LLVMBuildSub(
      builder,
   limiter,
   LLVMConstInt(int_type, 1, false),
                  /* i1cond = (mask != 0) */
   i1cond = LLVMBuildICmp(
      builder,
   LLVMIntNE,
   LLVMBuildBitCast(builder, mask->exec_mask, reg_type, ""),
         /* i2cond = (looplimiter > 0) */
   i2cond = LLVMBuildICmp(
      builder,
   LLVMIntSGT,
   limiter,
         /* if( i1cond && i2cond ) */
                     LLVMBuildCondBr(builder,
                     assert(ctx->loop_stack_size);
   --ctx->loop_stack_size;
   --ctx->bgnloop_stack_size;
   mask->cont_mask = ctx->loop_stack[ctx->loop_stack_size].cont_mask;
   mask->break_mask = ctx->loop_stack[ctx->loop_stack_size].break_mask;
   ctx->loop_block = ctx->loop_stack[ctx->loop_stack_size].loop_block;
   ctx->break_var = ctx->loop_stack[ctx->loop_stack_size].break_var;
   ctx->break_type = ctx->break_type_stack[ctx->loop_stack_size +
               }
      void lp_exec_mask_cond_push(struct lp_exec_mask *mask,
         {
      LLVMBuilderRef builder = mask->bld->gallivm->builder;
            if (ctx->cond_stack_size >= LP_MAX_TGSI_NESTING) {
      ctx->cond_stack_size++;
      }
   if (ctx->cond_stack_size == 0 && mask->function_stack_size == 1) {
         }
   ctx->cond_stack[ctx->cond_stack_size++] = mask->cond_mask;
   assert(LLVMTypeOf(val) == mask->int_vec_type);
   mask->cond_mask = LLVMBuildAnd(builder,
                        }
      void lp_exec_mask_cond_invert(struct lp_exec_mask *mask)
   {
      LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);
   LLVMValueRef prev_mask;
            assert(ctx->cond_stack_size);
   if (ctx->cond_stack_size >= LP_MAX_TGSI_NESTING)
         prev_mask = ctx->cond_stack[ctx->cond_stack_size - 1];
   if (ctx->cond_stack_size == 1 && mask->function_stack_size == 1) {
                           mask->cond_mask = LLVMBuildAnd(builder,
                  }
      void lp_exec_mask_cond_pop(struct lp_exec_mask *mask)
   {
      struct function_ctx *ctx = func_ctx(mask);
   assert(ctx->cond_stack_size);
   --ctx->cond_stack_size;
   if (ctx->cond_stack_size >= LP_MAX_TGSI_NESTING)
         mask->cond_mask = ctx->cond_stack[ctx->cond_stack_size];
      }
         void lp_exec_continue(struct lp_exec_mask *mask)
   {
      LLVMBuilderRef builder = mask->bld->gallivm->builder;
   LLVMValueRef exec_mask = LLVMBuildNot(builder,
                  mask->cont_mask = LLVMBuildAnd(builder,
                     }
      void lp_exec_break(struct lp_exec_mask *mask, int *pc,
         {
      LLVMBuilderRef builder = mask->bld->gallivm->builder;
            if (ctx->break_type == LP_EXEC_MASK_BREAK_TYPE_LOOP) {
      LLVMValueRef exec_mask = LLVMBuildNot(builder,
                  mask->break_mask = LLVMBuildAnd(builder,
            }
   else {
      if (ctx->switch_in_default) {
      /*
   * stop default execution but only if this is an unconditional switch.
   * (The condition here is not perfect since dead code after break is
   * allowed but should be sufficient since false negatives are just
   * unoptimized - so we don't have to pre-evaluate that).
   */
   if(break_always && ctx->switch_pc) {
      if (pc)
                        if (break_always) {
         }
   else {
      LLVMValueRef exec_mask = LLVMBuildNot(builder,
               mask->switch_mask = LLVMBuildAnd(builder,
                           }
