   /*
   * Copyright © 2013-2015 Intel Corporation
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
      #include "brw_vec4_surface_builder.h"
      using namespace brw;
      namespace {
      namespace array_utils {
      /**
   * Copy one every \p src_stride logical components of the argument into
   * one every \p dst_stride logical components of the result.
   */
   static src_reg
   emit_stride(const vec4_builder &bld, const src_reg &src, unsigned size,
         {
      if (src_stride == 1 && dst_stride == 1) {
         } else {
                     for (unsigned i = 0; i < size; ++i)
      bld.MOV(writemask(offset(dst, 8, i * dst_stride / 4),
                                    /**
   * Convert a VEC4 into an array of registers with the layout expected by
   * the recipient shared unit.  If \p has_simd4x2 is true the argument is
   * left unmodified in SIMD4x2 form, otherwise it will be rearranged into
   * a SIMD8 vector.
   */
   static src_reg
   emit_insert(const vec4_builder &bld, const src_reg &src,
         {
                     } else {
      /* Pad unused components with zeroes. */
                  bld.MOV(writemask(tmp, mask), src);
                              }
      namespace brw {
      namespace surface_access {
      namespace {
               /**
   * Generate a send opcode for a surface message and return the
   * result.
   */
   src_reg
   emit_send(const vec4_builder &bld, enum opcode op,
            const src_reg &header,
   const src_reg &addr, unsigned addr_sz,
   const src_reg &src, unsigned src_sz,
   const src_reg &surface,
      {
      /* Calculate the total number of components of the payload. */
                  /* Construct the payload. */
                  if (header_sz)
                  for (unsigned i = 0; i < addr_sz; i++)
                  for (unsigned i = 0; i < src_sz; i++)
                  /* Reduce the dynamically uniform surface index to a single
   * scalar.
                  /* Emit the message send instruction. */
   const dst_reg dst = bld.vgrf(BRW_REGISTER_TYPE_UD, ret_sz);
   vec4_instruction *inst =
         inst->mlen = sz;
   inst->size_written = ret_sz * REG_SIZE;
                                 /**
   * Emit an untyped surface read opcode.  \p dims determines the number
   * of components of the address and \p size the number of components of
   * the returned value.
   */
   src_reg
   emit_untyped_read(const vec4_builder &bld,
                     {
      return emit_send(bld, VEC4_OPCODE_UNTYPED_SURFACE_READ, src_reg(),
                           /**
   * Emit an untyped surface write opcode.  \p dims determines the number
   * of components of the address and \p size the number of components of
   * the argument.
   */
   void
   emit_untyped_write(const vec4_builder &bld, const src_reg &surface,
                     {
      const bool has_simd4x2 = bld.shader->devinfo->verx10 == 75;
   emit_send(bld, VEC4_OPCODE_UNTYPED_SURFACE_WRITE, src_reg(),
            emit_insert(bld, addr, dims, has_simd4x2),
   has_simd4x2 ? 1 : dims,
   emit_insert(bld, src, size, has_simd4x2),
            /**
   * Emit an untyped surface atomic opcode.  \p dims determines the number
   * of components of the address and \p rsize the number of components of
   * the returned value (either zero or one).
   */
   src_reg
   emit_untyped_atomic(const vec4_builder &bld,
                     const src_reg &surface, const src_reg &addr,
   {
               /* Zip the components of both sources, they are represented as the X
   * and Y components of the same vector.
   */
                  if (size >= 1) {
      bld.MOV(writemask(srcs, WRITEMASK_X),
               if (size >= 2) {
      bld.MOV(writemask(srcs, WRITEMASK_Y),
               return emit_send(bld, VEC4_OPCODE_UNTYPED_ATOMIC, src_reg(),
                  emit_insert(bld, addr, dims, has_simd4x2),
   has_simd4x2 ? 1 : dims,
         }
