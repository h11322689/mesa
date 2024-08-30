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
      #include "ir_builder.h"
   #include "program/prog_instruction.h"
      using namespace ir_builder;
      namespace ir_builder {
      void
   ir_factory::emit(ir_instruction *ir)
   {
         }
      ir_variable *
   ir_factory::make_temp(const glsl_type *type, const char *name)
   {
               var = new(mem_ctx) ir_variable(type, name, ir_var_temporary);
               }
      ir_assignment *
   assign(deref lhs, operand rhs)
   {
         }
      ir_assignment *
   assign(deref lhs, operand rhs, int writemask)
   {
               ir_assignment *assign = new(mem_ctx) ir_assignment(lhs.val,
                     }
      ir_return *
   ret(operand retval)
   {
      void *mem_ctx = ralloc_parent(retval.val);
      }
      ir_swizzle *
   swizzle(operand a, int swizzle, int components)
   {
               return new(mem_ctx) ir_swizzle(a.val,
                              }
      ir_swizzle *
   swizzle_for_size(operand a, unsigned components)
   {
               if (a.val->type->vector_elements < components)
            unsigned s[4] = { 0, 1, 2, 3 };
   for (int i = components; i < 4; i++)
               }
      ir_swizzle *
   swizzle_xxxx(operand a)
   {
         }
      ir_swizzle *
   swizzle_yyyy(operand a)
   {
         }
      ir_swizzle *
   swizzle_zzzz(operand a)
   {
         }
      ir_swizzle *
   swizzle_wwww(operand a)
   {
         }
      ir_swizzle *
   swizzle_x(operand a)
   {
         }
      ir_swizzle *
   swizzle_y(operand a)
   {
         }
      ir_swizzle *
   swizzle_z(operand a)
   {
         }
      ir_swizzle *
   swizzle_w(operand a)
   {
         }
      ir_swizzle *
   swizzle_xy(operand a)
   {
         }
      ir_swizzle *
   swizzle_xyz(operand a)
   {
         }
      ir_swizzle *
   swizzle_xyzw(operand a)
   {
         }
      ir_expression *
   expr(ir_expression_operation op, operand a)
   {
                  }
      ir_expression *
   expr(ir_expression_operation op, operand a, operand b)
   {
                  }
      ir_expression *
   expr(ir_expression_operation op, operand a, operand b, operand c)
   {
                  }
      ir_expression *add(operand a, operand b)
   {
         }
      ir_expression *sub(operand a, operand b)
   {
         }
      ir_expression *min2(operand a, operand b)
   {
         }
      ir_expression *max2(operand a, operand b)
   {
         }
      ir_expression *mul(operand a, operand b)
   {
         }
      ir_expression *imul_high(operand a, operand b)
   {
         }
      ir_expression *div(operand a, operand b)
   {
         }
      ir_expression *carry(operand a, operand b)
   {
         }
      ir_expression *borrow(operand a, operand b)
   {
         }
      ir_expression *trunc(operand a)
   {
         }
      ir_expression *round_even(operand a)
   {
         }
      ir_expression *fract(operand a)
   {
         }
      /* dot for vectors, mul for scalars */
   ir_expression *dot(operand a, operand b)
   {
               if (a.val->type->vector_elements == 1)
               }
      ir_expression*
   clamp(operand a, operand b, operand c)
   {
         }
      ir_expression *
   saturate(operand a)
   {
         }
      ir_expression *
   abs(operand a)
   {
         }
      ir_expression *
   neg(operand a)
   {
         }
      ir_expression *
   sin(operand a)
   {
         }
      ir_expression *
   cos(operand a)
   {
         }
      ir_expression *
   exp(operand a)
   {
         }
      ir_expression *
   rcp(operand a)
   {
         }
      ir_expression *
   rsq(operand a)
   {
         }
      ir_expression *
   sqrt(operand a)
   {
         }
      ir_expression *
   log(operand a)
   {
         }
      ir_expression *
   sign(operand a)
   {
         }
      ir_expression *
   subr_to_int(operand a)
   {
         }
      ir_expression*
   equal(operand a, operand b)
   {
         }
      ir_expression*
   nequal(operand a, operand b)
   {
         }
      ir_expression*
   less(operand a, operand b)
   {
         }
      ir_expression*
   greater(operand a, operand b)
   {
         }
      ir_expression*
   lequal(operand a, operand b)
   {
         }
      ir_expression*
   gequal(operand a, operand b)
   {
         }
      ir_expression*
   logic_not(operand a)
   {
         }
      ir_expression*
   logic_and(operand a, operand b)
   {
         }
      ir_expression*
   logic_or(operand a, operand b)
   {
         }
      ir_expression*
   bit_not(operand a)
   {
         }
      ir_expression*
   bit_and(operand a, operand b)
   {
         }
      ir_expression*
   bit_or(operand a, operand b)
   {
         }
      ir_expression*
   bit_xor(operand a, operand b)
   {
         }
      ir_expression*
   lshift(operand a, operand b)
   {
         }
      ir_expression*
   rshift(operand a, operand b)
   {
         }
      ir_expression*
   f2i(operand a)
   {
         }
      ir_expression*
   bitcast_f2i(operand a)
   {
         }
      ir_expression*
   i2f(operand a)
   {
         }
      ir_expression*
   bitcast_i2f(operand a)
   {
         }
      ir_expression*
   i2u(operand a)
   {
         }
      ir_expression*
   u2i(operand a)
   {
         }
      ir_expression*
   f2u(operand a)
   {
         }
      ir_expression*
   bitcast_f2u(operand a)
   {
         }
      ir_expression*
   u2f(operand a)
   {
         }
      ir_expression*
   bitcast_u2f(operand a)
   {
         }
      ir_expression*
   i2b(operand a)
   {
         }
      ir_expression*
   b2i(operand a)
   {
         }
      ir_expression *
   f2b(operand a)
   {
         }
      ir_expression *
   b2f(operand a)
   {
         }
      ir_expression*
   bitcast_d2i64(operand a)
   {
         }
      ir_expression*
   bitcast_d2u64(operand a)
   {
         }
      ir_expression*
   bitcast_i642d(operand a)
   {
         }
      ir_expression*
   bitcast_u642d(operand a)
   {
         }
      ir_expression *
   interpolate_at_centroid(operand a)
   {
         }
      ir_expression *
   interpolate_at_offset(operand a, operand b)
   {
         }
      ir_expression *
   interpolate_at_sample(operand a, operand b)
   {
         }
      ir_expression *
   f2d(operand a)
   {
         }
      ir_expression *
   i2d(operand a)
   {
         }
      ir_expression *
   u2d(operand a)
   {
         }
      ir_expression *
   fma(operand a, operand b, operand c)
   {
         }
      ir_expression *
   lrp(operand x, operand y, operand a)
   {
         }
      ir_expression *
   csel(operand a, operand b, operand c)
   {
         }
      ir_expression *
   bitfield_extract(operand a, operand b, operand c)
   {
         }
      ir_expression *
   bitfield_insert(operand a, operand b, operand c, operand d)
   {
      void *mem_ctx = ralloc_parent(a.val);
   return new(mem_ctx) ir_expression(ir_quadop_bitfield_insert,
      }
      ir_if*
   if_tree(operand condition,
         {
                        ir_if *result = new(mem_ctx) ir_if(condition.val);
   result->then_instructions.push_tail(then_branch);
      }
      ir_if*
   if_tree(operand condition,
         ir_instruction *then_branch,
   {
      assert(then_branch != NULL);
                     ir_if *result = new(mem_ctx) ir_if(condition.val);
   result->then_instructions.push_tail(then_branch);
   result->else_instructions.push_tail(else_branch);
      }
      } /* namespace ir_builder */
