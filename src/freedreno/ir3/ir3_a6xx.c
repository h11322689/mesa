   /*
   * Copyright (C) 2017-2018 Rob Clark <robclark@freedesktop.org>
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
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #define GPU 600
      #include "ir3_context.h"
   #include "ir3_image.h"
      /*
   * Handlers for instructions changed/added in a6xx:
   *
   * Starting with a6xx, isam and stbi is used for SSBOs as well; stbi and the
   * atomic instructions (used for both SSBO and image) use a new instruction
   * encoding compared to a4xx/a5xx.
   */
      /* src[] = { buffer_index, offset }. No const_index */
   static void
   emit_intrinsic_load_ssbo(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *offset;
                     ldib = ir3_LDIB(b, ir3_ssbo_to_ibo(ctx, intr->src[0]), 0, offset, 0);
   ldib->dsts[0]->wrmask = MASK(intr->num_components);
   ldib->cat6.iim_val = intr->num_components;
   ldib->cat6.d = 1;
   ldib->cat6.type = intr->def.bit_size == 16 ? TYPE_U16 : TYPE_U32;
   ldib->barrier_class = IR3_BARRIER_BUFFER_R;
   ldib->barrier_conflict = IR3_BARRIER_BUFFER_W;
   ir3_handle_bindless_cat6(ldib, intr->src[0]);
               }
      /* src[] = { value, block_index, offset }. const_index[] = { write_mask } */
   static void
   emit_intrinsic_store_ssbo(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *stib, *val, *offset;
   unsigned wrmask = nir_intrinsic_write_mask(intr);
                     /* src0 is offset, src1 is value:
   */
   val = ir3_create_collect(b, ir3_get_src(ctx, &intr->src[0]), ncomp);
            stib = ir3_STIB(b, ir3_ssbo_to_ibo(ctx, intr->src[1]), 0, offset, 0, val, 0);
   stib->cat6.iim_val = ncomp;
   stib->cat6.d = 1;
   stib->cat6.type = intr->src[0].ssa->bit_size == 16 ? TYPE_U16 : TYPE_U32;
   stib->barrier_class = IR3_BARRIER_BUFFER_W;
   stib->barrier_conflict = IR3_BARRIER_BUFFER_R | IR3_BARRIER_BUFFER_W;
   ir3_handle_bindless_cat6(stib, intr->src[1]);
               }
      static struct ir3_instruction *
   emit_atomic(struct ir3_block *b,
               nir_atomic_op op,
   struct ir3_instruction *ibo,
   {
      switch (op) {
   case nir_atomic_op_iadd:
         case nir_atomic_op_imin:
         case nir_atomic_op_umin:
         case nir_atomic_op_imax:
         case nir_atomic_op_umax:
         case nir_atomic_op_iand:
         case nir_atomic_op_ior:
         case nir_atomic_op_ixor:
         case nir_atomic_op_xchg:
         case nir_atomic_op_cmpxchg:
         default:
            }
      /*
   * SSBO atomic intrinsics
   *
   * All of the SSBO atomic memory operations read a value from memory,
   * compute a new value using one of the operations below, write the new
   * value to memory, and return the original value read.
   *
   * All operations take 3 sources except CompSwap that takes 4. These
   * sources represent:
   *
   * 0: The SSBO buffer index.
   * 1: The offset into the SSBO buffer of the variable that the atomic
   *    operation will operate on.
   * 2: The data parameter to the atomic function (i.e. the value to add
   *    in, etc).
   * 3: For CompSwap only: the second data parameter.
   */
   static struct ir3_instruction *
   emit_intrinsic_atomic_ssbo(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *atomic, *ibo, *src0, *src1, *data, *dummy;
   nir_atomic_op op = nir_intrinsic_atomic_op(intr);
                              /* So this gets a bit creative:
   *
   *    src0    - vecN offset/coords
   *    src1.x  - is actually destination register
   *    src1.y  - is 'data' except for cmpxchg where src2.y is 'compare'
   *    src1.z  - is 'data' for cmpxchg
   *
   * The combining src and dest kinda doesn't work out so well with how
   * scheduling and RA work. So we create a dummy src2 which is tied to the
   * destination in RA (i.e. must be allocated to the same vec2/vec3
   * register) and then immediately extract the first component.
   *
   * Note that nir already multiplies the offset by four
   */
            if (op == nir_atomic_op_cmpxchg) {
      src0 = ir3_get_src(ctx, &intr->src[4])[0];
   struct ir3_instruction *compare = ir3_get_src(ctx, &intr->src[3])[0];
      } else {
      src0 = ir3_get_src(ctx, &intr->src[3])[0];
               atomic = emit_atomic(b, op, ibo, src0, src1);
   atomic->cat6.iim_val = 1;
   atomic->cat6.d = 1;
   atomic->cat6.type = type;
   atomic->barrier_class = IR3_BARRIER_BUFFER_W;
   atomic->barrier_conflict = IR3_BARRIER_BUFFER_R | IR3_BARRIER_BUFFER_W;
            /* even if nothing consume the result, we can't DCE the instruction: */
            atomic->dsts[0]->wrmask = src1->dsts[0]->wrmask;
   ir3_reg_tie(atomic->dsts[0], atomic->srcs[2]);
   ir3_handle_nonuniform(atomic, intr);
   struct ir3_instruction *split;
   ir3_split_dest(b, &split, atomic, 0, 1);
      }
      /* src[] = { deref, coord, sample_index }. const_index[] = {} */
   static void
   emit_intrinsic_load_image(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *ldib;
   struct ir3_instruction *const *coords = ir3_get_src(ctx, &intr->src[1]);
            ldib = ir3_LDIB(b, ir3_image_to_ibo(ctx, intr->src[0]), 0,
         ldib->dsts[0]->wrmask = MASK(intr->num_components);
   ldib->cat6.iim_val = intr->num_components;
   ldib->cat6.d = ncoords;
   ldib->cat6.type = ir3_get_type_for_image_intrinsic(intr);
   ldib->cat6.typed = true;
   ldib->barrier_class = IR3_BARRIER_IMAGE_R;
   ldib->barrier_conflict = IR3_BARRIER_IMAGE_W;
   ir3_handle_bindless_cat6(ldib, intr->src[0]);
               }
      /* src[] = { deref, coord, sample_index, value }. const_index[] = {} */
   static void
   emit_intrinsic_store_image(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *stib;
   struct ir3_instruction *const *value = ir3_get_src(ctx, &intr->src[3]);
   struct ir3_instruction *const *coords = ir3_get_src(ctx, &intr->src[1]);
   unsigned ncoords = ir3_get_image_coords(intr, NULL);
   enum pipe_format format = nir_intrinsic_format(intr);
            /* src0 is offset, src1 is value:
   */
   stib = ir3_STIB(b, ir3_image_to_ibo(ctx, intr->src[0]), 0,
               stib->cat6.iim_val = ncomp;
   stib->cat6.d = ncoords;
   stib->cat6.type = ir3_get_type_for_image_intrinsic(intr);
   stib->cat6.typed = true;
   stib->barrier_class = IR3_BARRIER_IMAGE_W;
   stib->barrier_conflict = IR3_BARRIER_IMAGE_R | IR3_BARRIER_IMAGE_W;
   ir3_handle_bindless_cat6(stib, intr->src[0]);
               }
      /* src[] = { deref, coord, sample_index, value, compare }. const_index[] = {} */
   static struct ir3_instruction *
   emit_intrinsic_atomic_image(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *atomic, *ibo, *src0, *src1, *dummy;
   struct ir3_instruction *const *coords = ir3_get_src(ctx, &intr->src[1]);
   struct ir3_instruction *value = ir3_get_src(ctx, &intr->src[3])[0];
   unsigned ncoords = ir3_get_image_coords(intr, NULL);
                     /* So this gets a bit creative:
   *
   *    src0    - vecN offset/coords
   *    src1.x  - is actually destination register
   *    src1.y  - is 'value' except for cmpxchg where src2.y is 'compare'
   *    src1.z  - is 'value' for cmpxchg
   *
   * The combining src and dest kinda doesn't work out so well with how
   * scheduling and RA work. So we create a dummy src2 which is tied to the
   * destination in RA (i.e. must be allocated to the same vec2/vec3
   * register) and then immediately extract the first component.
   */
   dummy = create_immed(b, 0);
            if (op == nir_atomic_op_cmpxchg) {
      struct ir3_instruction *compare = ir3_get_src(ctx, &intr->src[4])[0];
      } else {
                  atomic = emit_atomic(b, op, ibo, src0, src1);
   atomic->cat6.iim_val = 1;
   atomic->cat6.d = ncoords;
   atomic->cat6.type = ir3_get_type_for_image_intrinsic(intr);
   atomic->cat6.typed = true;
   atomic->barrier_class = IR3_BARRIER_IMAGE_W;
   atomic->barrier_conflict = IR3_BARRIER_IMAGE_R | IR3_BARRIER_IMAGE_W;
            /* even if nothing consume the result, we can't DCE the instruction: */
            atomic->dsts[0]->wrmask = src1->dsts[0]->wrmask;
   ir3_reg_tie(atomic->dsts[0], atomic->srcs[2]);
   ir3_handle_nonuniform(atomic, intr);
   struct ir3_instruction *split;
   ir3_split_dest(b, &split, atomic, 0, 1);
      }
      static void
   emit_intrinsic_image_size(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *ibo = ir3_image_to_ibo(ctx, intr->src[0]);
   struct ir3_instruction *resinfo = ir3_RESINFO(b, ibo, 0);
   resinfo->cat6.iim_val = 1;
   resinfo->cat6.d = intr->num_components;
   resinfo->cat6.type = TYPE_U32;
   resinfo->cat6.typed = false;
   /* resinfo has no writemask and always writes out 3 components: */
   compile_assert(ctx, intr->num_components <= 3);
   resinfo->dsts[0]->wrmask = MASK(3);
   ir3_handle_bindless_cat6(resinfo, intr->src[0]);
               }
      static void
   emit_intrinsic_load_global_ir3(struct ir3_context *ctx,
               {
      struct ir3_block *b = ctx->block;
   unsigned dest_components = nir_intrinsic_dest_components(intr);
            addr = ir3_collect(b, ir3_get_src(ctx, &intr->src[0])[0],
                     bool const_offset_in_bounds = nir_src_is_const(intr->src[1]) &&
                  if (const_offset_in_bounds) {
      load = ir3_LDG(b, addr, 0, create_immed(b, nir_src_as_int(intr->src[1]) * 4),
      } else {
      offset = ir3_get_src(ctx, &intr->src[1])[0];
   if (ctx->compiler->gen >= 7) {
      /* A7XX TODO: Move to NIR for it to be properly optimized? */
      }
   load =
      ir3_LDG_A(b, addr, 0, offset, 0, create_immed(b, 0), 0,
            load->cat6.type = type_uint_size(intr->def.bit_size);
            load->barrier_class = IR3_BARRIER_BUFFER_R;
               }
      static void
   emit_intrinsic_store_global_ir3(struct ir3_context *ctx,
         {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *value, *addr, *offset;
            addr = ir3_collect(b, ir3_get_src(ctx, &intr->src[1])[0],
                              bool const_offset_in_bounds = nir_src_is_const(intr->src[2]) &&
                  if (const_offset_in_bounds) {
      stg = ir3_STG(b, addr, 0,
                  } else {
      offset = ir3_get_src(ctx, &intr->src[2])[0];
   if (ctx->compiler->gen >= 7) {
      /* A7XX TODO: Move to NIR for it to be properly optimized? */
      }
   stg =
      ir3_STG_A(b, addr, 0, offset, 0, create_immed(b, 0), 0,
            stg->cat6.type = type_uint_size(intr->src[0].ssa->bit_size);
                     stg->barrier_class = IR3_BARRIER_BUFFER_W;
      }
      static struct ir3_instruction *
   emit_intrinsic_atomic_global(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *addr, *atomic, *src1;
   struct ir3_instruction *value = ir3_get_src(ctx, &intr->src[1])[0];
   nir_atomic_op op = nir_intrinsic_atomic_op(intr);
            addr = ir3_collect(b, ir3_get_src(ctx, &intr->src[0])[0],
            if (op == nir_atomic_op_cmpxchg) {
      struct ir3_instruction *compare = ir3_get_src(ctx, &intr->src[2])[0];
      } else {
                  switch (op) {
   case nir_atomic_op_iadd:
      atomic = ir3_ATOMIC_G_ADD(b, addr, 0, src1, 0);
      case nir_atomic_op_imin:
      atomic = ir3_ATOMIC_G_MIN(b, addr, 0, src1, 0);
   type = TYPE_S32;
      case nir_atomic_op_umin:
      atomic = ir3_ATOMIC_G_MIN(b, addr, 0, src1, 0);
      case nir_atomic_op_imax:
      atomic = ir3_ATOMIC_G_MAX(b, addr, 0, src1, 0);
   type = TYPE_S32;
      case nir_atomic_op_umax:
      atomic = ir3_ATOMIC_G_MAX(b, addr, 0, src1, 0);
      case nir_atomic_op_iand:
      atomic = ir3_ATOMIC_G_AND(b, addr, 0, src1, 0);
      case nir_atomic_op_ior:
      atomic = ir3_ATOMIC_G_OR(b, addr, 0, src1, 0);
      case nir_atomic_op_ixor:
      atomic = ir3_ATOMIC_G_XOR(b, addr, 0, src1, 0);
      case nir_atomic_op_xchg:
      atomic = ir3_ATOMIC_G_XCHG(b, addr, 0, src1, 0);
      case nir_atomic_op_cmpxchg:
      atomic = ir3_ATOMIC_G_CMPXCHG(b, addr, 0, src1, 0);
      default:
                  atomic->cat6.iim_val = 1;
   atomic->cat6.d = 1;
   atomic->cat6.type = type;
   atomic->barrier_class = IR3_BARRIER_BUFFER_W;
            /* even if nothing consume the result, we can't DCE the instruction: */
               }
      const struct ir3_context_funcs ir3_a6xx_funcs = {
      .emit_intrinsic_load_ssbo = emit_intrinsic_load_ssbo,
   .emit_intrinsic_store_ssbo = emit_intrinsic_store_ssbo,
   .emit_intrinsic_atomic_ssbo = emit_intrinsic_atomic_ssbo,
   .emit_intrinsic_load_image = emit_intrinsic_load_image,
   .emit_intrinsic_store_image = emit_intrinsic_store_image,
   .emit_intrinsic_atomic_image = emit_intrinsic_atomic_image,
   .emit_intrinsic_image_size = emit_intrinsic_image_size,
   .emit_intrinsic_load_global_ir3 = emit_intrinsic_load_global_ir3,
   .emit_intrinsic_store_global_ir3 = emit_intrinsic_store_global_ir3,
      };
