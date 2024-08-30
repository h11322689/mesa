   /*
   * Copyright © 2018 Intel Corporation
   * Copyright © 2018 Broadcom
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
      #include "v3d_compiler.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_format_convert.h"
      /** @file v3d_nir_lower_image_load_store.c
   *
   * Performs any necessary lowering of GL_ARB_shader_image_load_store
   * operations.
   *
   * On V3D 4.x, we just need to do format conversion for stores such that the
   * GPU can effectively memcpy the arguments (in increments of 32-bit words)
   * into the texel.  Loads are the same as texturing, where we may need to
   * unpack from 16-bit ints or floats.
   *
   * On V3D 3.x, to implement image load store we would need to do manual tiling
   * calculations and load/store using the TMU general memory access path.
   */
      bool
   v3d_gl_format_is_return_32(enum pipe_format format)
   {
         /* We can get a NONE format in Vulkan because we support the
      * shaderStorageImageReadWithoutFormat feature. We consider these to
   * always use 32-bit precision.
      if (format == PIPE_FORMAT_NONE)
            const struct util_format_description *desc =
                  }
      /* Packs a 32-bit vector of colors in the range [0, (1 << bits[i]) - 1] to a
   * 32-bit SSA value, with as many channels as necessary to store all the bits
   */
   static nir_def *
   pack_bits(nir_builder *b, nir_def *color, const unsigned *bits,
         {
         nir_def *results[4];
   int offset = 0;
   for (int i = 0; i < num_components; i++) {
                                    if (mask) {
                        if (offset % 32 == 0) {
         } else {
            results[offset / 32] =
                        }
      static bool
   v3d_nir_lower_image_store(nir_builder *b, nir_intrinsic_instr *instr)
   {
         enum pipe_format format = nir_intrinsic_format(instr);
   assert(format != PIPE_FORMAT_NONE);
   const struct util_format_description *desc =
         const struct util_format_channel_description *r_chan = &desc->channel[0];
                     nir_def *color = nir_trim_vector(b,
                        if (format == PIPE_FORMAT_R11G11B10_FLOAT) {
         } else if (format == PIPE_FORMAT_R9G9B9E5_FLOAT) {
         } else if (r_chan->size == 32) {
            /* For 32-bit formats, we just have to move the vector
   * across (possibly reducing the number of channels).
      } else {
            static const unsigned bits_8[4] = {8, 8, 8, 8};
                        switch (r_chan->size) {
   case 8:
               case 10:
               case 16:
                                    bool pack_mask = false;
   if (r_chan->pure_integer &&
      r_chan->type == UTIL_FORMAT_TYPE_SIGNED) {
         /* We don't need to do any conversion or clamping in this case */
      } else if (r_chan->pure_integer &&
                     } else if (r_chan->normalized &&
                     } else if (r_chan->normalized &&
               } else {
                                          nir_src_rewrite(&instr->src[3], formatted);
            }
      static bool
   v3d_nir_lower_image_load(nir_builder *b, nir_intrinsic_instr *instr)
   {
         static const unsigned bits16[] = {16, 16, 16, 16};
            if (v3d_gl_format_is_return_32(format))
                     nir_def *result = &instr->def;
   if (util_format_is_pure_uint(format)) {
         } else if (util_format_is_pure_sint(format)) {
         } else {
            nir_def *rg = nir_channel(b, result, 0);
   nir_def *ba = nir_channel(b, result, 1);
   result = nir_vec4(b,
                           nir_def_rewrite_uses_after(&instr->def, result,
            }
      static bool
   v3d_nir_lower_image_load_store_cb(nir_builder *b,
               {
         switch (intr->intrinsic) {
   case nir_intrinsic_image_load:
         case nir_intrinsic_image_store:
         default:
                  }
      bool
   v3d_nir_lower_image_load_store(nir_shader *s)
   {
         return nir_shader_intrinsics_pass(s,
               }
