   /*
   * Copyright (C) 2020 Collabora, Ltd.
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
   *
   * Authors (Collabora):
   *      Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   */
      /**
   * Implements framebuffer format conversions in software for Midgard/Bifrost
   * blend shaders. This pass is designed for a single render target; Midgard
   * duplicates blend shaders for MRT to simplify everything. A particular
   * framebuffer format may be categorized as 1) typed load available, 2) typed
   * unpack available, or 3) software unpack only, and likewise for stores. The
   * first two types are handled in the compiler backend directly, so this module
   * is responsible for identifying type 3 formats (hardware dependent) and
   * inserting appropriate ALU code to perform the conversion from the packed
   * type to a designated unpacked type, and vice versa.
   *
   * The unpacked type depends on the format:
   *
   *      - For 32-bit float formats or >8-bit UNORM, 32-bit floats.
   *      - For other floats, 16-bit floats.
   *      - For 32-bit ints, 32-bit ints.
   *      - For 8-bit ints, 8-bit ints.
   *      - For other ints, 16-bit ints.
   *
   * The rationale is to optimize blending and logic op instructions by using the
   * smallest precision necessary to store the pixel losslessly.
   */
      #include "pan_lower_framebuffer.h"
   #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_format_convert.h"
   #include "util/format/u_format.h"
      /* Determines the unpacked type best suiting a given format, so the rest of the
   * pipeline may be adjusted accordingly */
      nir_alu_type
   pan_unpacked_type_for_format(const struct util_format_description *desc)
   {
               if (c == -1)
            bool large = (desc->channel[c].size > 16);
   bool large_norm = (desc->channel[c].size > 8);
   bool bit8 = (desc->channel[c].size == 8);
            if (desc->channel[c].normalized)
            switch (desc->channel[c].type) {
   case UTIL_FORMAT_TYPE_UNSIGNED:
         case UTIL_FORMAT_TYPE_SIGNED:
         case UTIL_FORMAT_TYPE_FLOAT:
         default:
            }
      static bool
   pan_is_format_native(const struct util_format_description *desc,
         {
      if (is_store || broken_ld_special)
            if (util_format_is_pure_integer(desc->format) ||
      util_format_is_float(desc->format))
         /* Some formats are missing as typed but have unpacks */
   if (desc->format == PIPE_FORMAT_R11G11B10_FLOAT)
            if (desc->is_array) {
      int c = util_format_get_first_non_void_channel(desc->format);
   assert(c >= 0);
   if (desc->channel[c].size > 8)
                  }
      /* Software packs/unpacks, by format class. Packs take in the pixel value typed
   * as `pan_unpacked_type_for_format` of the format and return an i32vec4
   * suitable for storing (with components replicated to fill). Unpacks do the
   * reverse but cannot rely on replication. */
      static nir_def *
   pan_replicate(nir_builder *b, nir_def *v, unsigned num_components)
   {
               for (unsigned i = 0; i < 4; ++i)
               }
      /* Pure x16 formats are x16 unpacked, so it's similar, but we need to pack
   * upper/lower halves of course */
      static nir_def *
   pan_pack_pure_16(nir_builder *b, nir_def *v, unsigned num_components)
   {
               nir_def *lo = nir_pack_32_2x16(b, nir_channels(b, v4, 0x3 << 0));
               }
      static nir_def *
   pan_unpack_pure_16(nir_builder *b, nir_def *pack, unsigned num_components)
   {
                        for (unsigned i = 0; i < num_components; i += 2) {
               unpacked[i + 0] = nir_channel(b, halves, 0);
                  }
      static nir_def *
   pan_pack_reorder(nir_builder *b, const struct util_format_description *desc,
         {
               for (unsigned i = 0; i < v->num_components; i++) {
      if (desc->swizzle[i] <= PIPE_SWIZZLE_W)
                  }
      static nir_def *
   pan_unpack_reorder(nir_builder *b, const struct util_format_description *desc,
         {
               for (unsigned i = 0; i < v->num_components; i++) {
      if (desc->swizzle[i] <= PIPE_SWIZZLE_W)
                  }
      static nir_def *
   pan_pack_pure_8(nir_builder *b, nir_def *v, unsigned num_components)
   {
      return nir_replicate(
      }
      static nir_def *
   pan_unpack_pure_8(nir_builder *b, nir_def *pack, unsigned num_components)
   {
      nir_def *unpacked = nir_unpack_32_4x8(b, nir_channel(b, pack, 0));
      }
      static nir_def *
   pan_fsat(nir_builder *b, nir_def *v, bool is_signed)
   {
      if (is_signed)
         else
      }
      static float
   norm_scale(bool snorm, unsigned bits)
   {
      if (snorm)
         else
      }
      /* For <= 8-bits per channel, [U,S]NORM formats are packed like [U,S]NORM 8,
   * with zeroes spacing out each component as needed */
      static nir_def *
   pan_pack_norm(nir_builder *b, nir_def *v, unsigned x, unsigned y, unsigned z,
         {
      /* If a channel has N bits, 1.0 is encoded as 2^N - 1 for UNORMs and
   * 2^(N-1) - 1 for SNORMs */
   nir_def *scales =
      is_signed ? nir_imm_vec4_16(b, (1 << (x - 1)) - 1, (1 << (y - 1)) - 1,
                     /* If a channel has N bits, we pad out to the byte by (8 - N) bits */
   nir_def *shifts = nir_imm_ivec4(b, 8 - x, 8 - y, 8 - z, 8 - w);
            nir_def *f = nir_fmul(b, clamped, scales);
   nir_def *u8 = nir_f2u8(b, nir_fround_even(b, f));
   nir_def *s = nir_ishl(b, u8, shifts);
               }
      static nir_def *
   pan_pack_unorm(nir_builder *b, nir_def *v, unsigned x, unsigned y, unsigned z,
         {
         }
      /* RGB10_A2 is packed in the tilebuffer as the bottom 3 bytes being the top
   * 8-bits of RGB and the top byte being RGBA as 2-bits packed. As imirkin
   * pointed out, this means free conversion to RGBX8 */
      static nir_def *
   pan_pack_unorm_1010102(nir_builder *b, nir_def *v)
   {
      nir_def *scale = nir_imm_vec4(b, 1023.0, 1023.0, 1023.0, 3.0);
   nir_def *s =
            nir_def *top8 = nir_ushr(b, s, nir_imm_ivec4(b, 0x2, 0x2, 0x2, 0x2));
                     nir_def *top =
      nir_ior(b,
         nir_ior(b, nir_ishl_imm(b, nir_channel(b, bottom2, 0), 24 + 0),
               nir_def *p = nir_ior(b, top, top8_rgb);
      }
      /* On the other hand, the pure int RGB10_A2 is identical to the spec */
      static nir_def *
   pan_pack_int_1010102(nir_builder *b, nir_def *v, bool is_signed)
   {
               /* Clamp the values */
   if (is_signed) {
      v = nir_imin(b, v, nir_imm_ivec4(b, 511, 511, 511, 1));
      } else {
                  v = nir_ishl(b, v, nir_imm_ivec4(b, 0, 10, 20, 30));
   v = nir_ior(b, nir_ior(b, nir_channel(b, v, 0), nir_channel(b, v, 1)),
               }
      static nir_def *
   pan_unpack_int_1010102(nir_builder *b, nir_def *packed, bool is_signed)
   {
               /* Left shift all components so the sign bit is on the MSB, and
   * can be extended by ishr(). The ishl()+[u,i]shr() combination
   * sets all unused bits to 0 without requiring a mask.
   */
            if (is_signed)
         else
               }
      /* NIR means we can *finally* catch a break */
      static nir_def *
   pan_pack_r11g11b10(nir_builder *b, nir_def *v)
   {
         }
      static nir_def *
   pan_unpack_r11g11b10(nir_builder *b, nir_def *v)
   {
      nir_def *f32 = nir_format_unpack_11f11f10f(b, nir_channel(b, v, 0));
            /* Extend to vec4 with alpha */
   nir_def *components[4] = {nir_channel(b, f16, 0), nir_channel(b, f16, 1),
               }
      /* Wrapper around sRGB conversion */
      static nir_def *
   pan_linear_to_srgb(nir_builder *b, nir_def *linear)
   {
               /* TODO: fp16 native conversion */
   nir_def *srgb =
            nir_def *comp[4] = {
      nir_channel(b, srgb, 0),
   nir_channel(b, srgb, 1),
   nir_channel(b, srgb, 2),
                  }
      static nir_def *
   pan_unpack_pure(nir_builder *b, nir_def *packed, unsigned size, unsigned nr)
   {
      switch (size) {
   case 32:
         case 16:
         case 8:
         default:
            }
      /* Generic dispatches for un/pack regardless of format */
      static nir_def *
   pan_unpack(nir_builder *b, const struct util_format_description *desc,
         {
      if (desc->is_array) {
      int c = util_format_get_first_non_void_channel(desc->format);
   assert(c >= 0);
   struct util_format_channel_description d = desc->channel[c];
            /* Normalized formats are unpacked as integers. We need to
   * convert to float for the final result.
   */
   if (d.normalized) {
      bool snorm = desc->is_snorm;
                                    } else {
                     switch (desc->format) {
   case PIPE_FORMAT_R10G10B10A2_UINT:
   case PIPE_FORMAT_B10G10R10A2_UINT:
         case PIPE_FORMAT_R10G10B10A2_SINT:
   case PIPE_FORMAT_B10G10R10A2_SINT:
         case PIPE_FORMAT_R11G11B10_FLOAT:
         default:
                  fprintf(stderr, "%s\n", desc->name);
      }
      static nir_def *pan_pack(nir_builder *b,
               {
      if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
            if (desc->is_array) {
      int c = util_format_get_first_non_void_channel(desc->format);
   assert(c >= 0);
            /* Pure formats are packed as-is */
            /* Normalized formats get normalized first */
   if (d.normalized) {
      bool snorm = desc->is_snorm;
   float multiplier = norm_scale(snorm, d.size);
                              /* Pack the raw format */
   switch (d.size) {
   case 32:
         case 16:
         case 8:
         default:
                     switch (desc->format) {
   case PIPE_FORMAT_B4G4R4A4_UNORM:
   case PIPE_FORMAT_B4G4R4X4_UNORM:
   case PIPE_FORMAT_A4R4_UNORM:
   case PIPE_FORMAT_R4A4_UNORM:
   case PIPE_FORMAT_A4B4G4R4_UNORM:
   case PIPE_FORMAT_R4G4B4A4_UNORM:
         case PIPE_FORMAT_B5G5R5A1_UNORM:
   case PIPE_FORMAT_R5G5B5A1_UNORM:
         case PIPE_FORMAT_R5G6B5_UNORM:
   case PIPE_FORMAT_B5G6R5_UNORM:
         case PIPE_FORMAT_R10G10B10A2_UNORM:
   case PIPE_FORMAT_B10G10R10A2_UNORM:
         case PIPE_FORMAT_R10G10B10A2_UINT:
   case PIPE_FORMAT_B10G10R10A2_UINT:
         case PIPE_FORMAT_R10G10B10A2_SINT:
   case PIPE_FORMAT_B10G10R10A2_SINT:
         case PIPE_FORMAT_R11G11B10_FLOAT:
         default:
                  fprintf(stderr, "%s\n", desc->name);
      }
      static void
   pan_lower_fb_store(nir_builder *b, nir_intrinsic_instr *intr,
               {
      /* For stores, add conversion before */
   nir_def *unpacked = intr->src[0].ssa;
            /* Re-order the components */
   if (reorder_comps)
                     /* We have to split writeout in 128 bit chunks */
            for (unsigned s = 0; s < iterations; ++s) {
      nir_store_raw_output_pan(b, packed,
               }
      static nir_def *
   pan_sample_id(nir_builder *b, int sample)
   {
         }
      static void
   pan_lower_fb_load(nir_builder *b, nir_intrinsic_instr *intr,
               {
      nir_def *packed =
      nir_load_raw_output_pan(b, 4, 32, pan_sample_id(b, sample),
         /* Convert the raw value */
            /* Convert to the size of the load intrinsic.
   *
   * We can assume that the type will match with the framebuffer format:
   *
   * Page 170 of the PDF of the OpenGL ES 3.0.6 spec says:
   *
   * If [UNORM or SNORM, convert to fixed-point]; otherwise no type
   * conversion is applied. If the values written by the fragment shader
   * do not match the format(s) of the corresponding color buffer(s),
   * the result is undefined.
                     nir_alu_type src_type =
            unpacked = nir_convert_to_bit_size(b, unpacked, src_type, bits);
            /* Reorder the components */
   if (reorder_comps)
               }
      struct inputs {
      const enum pipe_format *rt_fmts;
   uint8_t raw_fmt_mask;
   bool is_blend;
   bool broken_ld_special;
      };
      static bool
   lower(nir_builder *b, nir_instr *instr, void *data)
   {
      struct inputs *inputs = data;
   if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   bool is_load = intr->intrinsic == nir_intrinsic_load_output;
            if (!(is_load || (is_store && inputs->is_blend)))
            nir_io_semantics sem = nir_intrinsic_io_semantics(intr);
   if (sem.location < FRAG_RESULT_DATA0)
            unsigned rt = sem.location - FRAG_RESULT_DATA0;
   if (inputs->rt_fmts[rt] == PIPE_FORMAT_NONE)
            const struct util_format_description *desc =
            /* Don't lower */
   if (pan_is_format_native(desc, inputs->broken_ld_special, is_store))
            /* EXT_shader_framebuffer_fetch requires per-sample loads. MSAA blend
   * shaders are not yet handled, so for now always load sample 0.
   */
   int sample = inputs->is_blend ? 0 : -1;
            if (is_store) {
      b->cursor = nir_before_instr(instr);
      } else {
      b->cursor = nir_after_instr(instr);
               nir_instr_remove(instr);
      }
      bool
   pan_lower_framebuffer(nir_shader *shader, const enum pipe_format *rt_fmts,
               {
               return nir_shader_instructions_pass(
      shader, lower, nir_metadata_block_index | nir_metadata_dominance,
   &(struct inputs){
      .rt_fmts = rt_fmts,
   .raw_fmt_mask = raw_fmt_mask,
   .nr_samples = blend_shader_nr_samples,
   .is_blend = blend_shader_nr_samples > 0,
      }
