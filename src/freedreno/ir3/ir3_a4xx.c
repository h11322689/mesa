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
      /* 500 gets us LDIB but doesn't change any other a4xx instructions */
   #define GPU 500
      #include "ir3_context.h"
   #include "ir3_image.h"
      /* SSBO data is available at this CB address, addressed like regular consts
   * containing the following data in each vec4:
   *
   * [ base address, pitch, array_pitch, cpp ]
   *
   * These mirror the values uploaded to A4XX_SSBO_0 state. For A5XX, these are
   * uploaded manually by the driver.
   */
   #define A4XX_SSBO_CB_BASE(i) (0x700 + ((i) << 2))
      /*
   * Handlers for instructions changed/added in a4xx:
   */
      /* Convert byte offset to address of appropriate width for GPU */
   static struct ir3_instruction *
   byte_offset_to_address(struct ir3_context *ctx,
         nir_src *ssbo,
   {
               if (ctx->compiler->gen == 4) {
      uint32_t index = nir_src_as_uint(*ssbo);
   unsigned cb = A4XX_SSBO_CB_BASE(index);
               if (ctx->compiler->is_64bit) {
         } else {
            }
      /* src[] = { buffer_index, offset }. No const_index */
   static void
   emit_intrinsic_load_ssbo(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
      struct ir3_block *b = ctx->block;
                     byte_offset = ir3_get_src(ctx, &intr->src[1])[0];
            /* src0 is uvec2(offset*4, 0), src1 is offset.. nir already *= 4: */
   src0 = byte_offset_to_address(ctx, &intr->src[0], byte_offset);
            ldgb = ir3_LDGB(b, ssbo, 0, src0, 0, src1, 0);
   ldgb->dsts[0]->wrmask = MASK(intr->num_components);
   ldgb->cat6.iim_val = intr->num_components;
   ldgb->cat6.d = 4;
   ldgb->cat6.type = TYPE_U32;
   ldgb->barrier_class = IR3_BARRIER_BUFFER_R;
               }
      /* src[] = { value, block_index, offset }. const_index[] = { write_mask } */
   static void
   emit_intrinsic_store_ssbo(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *stgb, *src0, *src1, *src2, *byte_offset, *offset;
   unsigned wrmask = nir_intrinsic_write_mask(intr);
                              byte_offset = ir3_get_src(ctx, &intr->src[2])[0];
            /* src0 is value, src1 is offset, src2 is uvec2(offset*4, 0)..
   * nir already *= 4:
   */
   src0 = ir3_create_collect(b, ir3_get_src(ctx, &intr->src[0]), ncomp);
   src1 = offset;
            stgb = ir3_STGB(b, ssbo, 0, src0, 0, src1, 0, src2, 0);
   stgb->cat6.iim_val = ncomp;
   stgb->cat6.d = 4;
   stgb->cat6.type = TYPE_U32;
   stgb->barrier_class = IR3_BARRIER_BUFFER_W;
               }
      static struct ir3_instruction *
   emit_atomic(struct ir3_block *b,
               nir_atomic_op op,
   struct ir3_instruction *bo,
   struct ir3_instruction *data,
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
   * 1: The byte offset into the SSBO buffer of the variable that the atomic
   *    operation will operate on.
   * 2: The data parameter to the atomic function (i.e. the value to add
   *    in, etc).
   * 3: CompSwap: the second data parameter.
   *    Non-CompSwap: The dword offset into the SSBO buffer variable.
   * 4: CompSwap: The dword offset into the SSBO buffer variable.
   *
   * We use custom ssbo_*_ir3 intrinsics generated by ir3_nir_lower_io_offsets()
   * so we can have the dword offset generated in NIR.
   */
   static struct ir3_instruction *
   emit_intrinsic_atomic_ssbo(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   nir_atomic_op op = nir_intrinsic_atomic_op(intr);
                     struct ir3_instruction *data = ir3_get_src(ctx, &intr->src[2])[0];
   /* 64b byte offset */
   struct ir3_instruction *byte_offset =
         /* dword offset for everything but cmpxchg */
            if (op == nir_atomic_op_cmpxchg) {
      /* for cmpxchg, src0 is [ui]vec2(data, compare): */
   data = ir3_collect(b, src3, data);
               struct ir3_instruction *atomic =
            atomic->cat6.iim_val = 1;
   atomic->cat6.d = 4;
   atomic->cat6.type = type;
   atomic->barrier_class = IR3_BARRIER_BUFFER_W;
            /* even if nothing consume the result, we can't DCE the instruction: */
               }
      static struct ir3_instruction *
   get_image_offset(struct ir3_context *ctx, const nir_intrinsic_instr *instr,
         {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *offset;
   unsigned index = nir_src_as_uint(instr->src[0]);
            /* to calculate the byte offset (yes, uggg) we need (up to) three
   * const values to know the bytes per pixel, and y and z stride:
   */
   unsigned cb;
   if (ctx->compiler->gen > 4) {
      const struct ir3_const_state *const_state = ir3_const_state(ctx->so);
            cb = regid(const_state->offsets.image_dims, 0) +
      } else {
      index += ctx->s->info.num_ssbos;
               /* offset = coords.x * bytes_per_pixel: */
   if (ctx->compiler->gen == 4)
         else
         if (ncoords > 1) {
      /* offset += coords.y * y_pitch: */
   offset =
      }
   if (ncoords > 2) {
      /* offset += coords.z * z_pitch: */
   offset =
               /* a4xx: must add in the base address: */
   if (ctx->compiler->gen == 4)
            if (!byteoff) {
      /* Some cases, like atomics, seem to use dword offset instead
   * of byte offsets.. blob just puts an extra shr.b in there
   * in those cases:
   */
               if (ctx->compiler->is_64bit)
         else
      }
      /* src[] = { deref, coord, sample_index }. const_index[] = {} */
   static void
   emit_intrinsic_load_image(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *const *coords = ir3_get_src(ctx, &intr->src[1]);
   struct ir3_instruction *ibo = ir3_image_to_ibo(ctx, intr->src[0]);
   struct ir3_instruction *offset = get_image_offset(ctx, intr, coords, true);
   unsigned ncoords = ir3_get_image_coords(intr, NULL);
   unsigned ncomp =
            struct ir3_instruction *ldib;
   /* At least A420 does not have LDIB. Use LDGB and perform conversion
   * ourselves.
   *
   * TODO: Actually do the conversion. ES 3.1 only requires this for
   * single-component 32-bit types anyways.
   */
   if (ctx->compiler->gen > 4) {
      ldib = ir3_LDIB(
      } else {
      ldib = ir3_LDGB(
         switch (nir_intrinsic_format(intr)) {
   case PIPE_FORMAT_R32_UINT:
   case PIPE_FORMAT_R32_SINT:
   case PIPE_FORMAT_R32_FLOAT:
         default:
      /* For some reason even more 32-bit components don't work. */
   assert(0);
         }
   ldib->dsts[0]->wrmask = MASK(intr->num_components);
   ldib->cat6.iim_val = ncomp;
   ldib->cat6.d = ncoords;
   ldib->cat6.type = ir3_get_type_for_image_intrinsic(intr);
   ldib->cat6.typed = true;
   ldib->barrier_class = IR3_BARRIER_IMAGE_R;
               }
      /* src[] = { index, coord, sample_index, value }. const_index[] = {} */
   static void
   emit_intrinsic_store_image(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *stib, *offset;
   struct ir3_instruction *const *value = ir3_get_src(ctx, &intr->src[3]);
   struct ir3_instruction *const *coords = ir3_get_src(ctx, &intr->src[1]);
   struct ir3_instruction *ibo = ir3_image_to_ibo(ctx, intr->src[0]);
   unsigned ncoords = ir3_get_image_coords(intr, NULL);
   unsigned ncomp =
            /* src0 is value
   * src1 is coords
   * src2 is 64b byte offset
                     /* NOTE: stib seems to take byte offset, but stgb.typed can be used
   * too and takes a dword offset.. not quite sure yet why blob uses
   * one over the other in various cases.
            stib = ir3_STIB(b, ibo, 0, ir3_create_collect(b, value, ncomp), 0,
         stib->cat6.iim_val = ncomp;
   stib->cat6.d = ncoords;
   stib->cat6.type = ir3_get_type_for_image_intrinsic(intr);
   stib->cat6.typed = true;
   stib->barrier_class = IR3_BARRIER_IMAGE_W;
               }
      /* src[] = { deref, coord, sample_index, value, compare }. const_index[] = {} */
   static struct ir3_instruction *
   emit_intrinsic_atomic_image(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *atomic, *src0, *src1, *src2;
   struct ir3_instruction *const *coords = ir3_get_src(ctx, &intr->src[1]);
   struct ir3_instruction *image = ir3_image_to_ibo(ctx, intr->src[0]);
   unsigned ncoords = ir3_get_image_coords(intr, NULL);
            /* src0 is value (or uvec2(value, compare))
   * src1 is coords
   * src2 is 64b byte offset
   */
   src0 = ir3_get_src(ctx, &intr->src[3])[0];
   src1 = ir3_create_collect(b, coords, ncoords);
            if (op == nir_atomic_op_cmpxchg)
            atomic = emit_atomic(b, op, image, src0, src1, src2);
   atomic->cat6.iim_val = 1;
   atomic->cat6.d = ncoords;
   atomic->cat6.type = ir3_get_type_for_image_intrinsic(intr);
   atomic->cat6.typed = ctx->compiler->gen == 5;
   atomic->barrier_class = IR3_BARRIER_IMAGE_W;
            /* even if nothing consume the result, we can't DCE the instruction: */
               }
      static struct ir3_instruction *
   emit_intrinsic_atomic_global(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
         }
      const struct ir3_context_funcs ir3_a4xx_funcs = {
      .emit_intrinsic_load_ssbo = emit_intrinsic_load_ssbo,
   .emit_intrinsic_store_ssbo = emit_intrinsic_store_ssbo,
   .emit_intrinsic_atomic_ssbo = emit_intrinsic_atomic_ssbo,
   .emit_intrinsic_load_image = emit_intrinsic_load_image,
   .emit_intrinsic_store_image = emit_intrinsic_store_image,
   .emit_intrinsic_atomic_image = emit_intrinsic_atomic_image,
   .emit_intrinsic_image_size = emit_intrinsic_image_size_tex,
   .emit_intrinsic_load_global_ir3 = NULL,
   .emit_intrinsic_store_global_ir3 = NULL,
      };
