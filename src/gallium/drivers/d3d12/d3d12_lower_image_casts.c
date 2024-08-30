   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_format_convert.h"
      #include "pipe/p_state.h"
   #include "util/format/u_format.h"
      #include "d3d12_compiler.h"
   #include "d3d12_nir_passes.h"
      static nir_def *
   convert_value(nir_builder *b, nir_def *value,
      const struct util_format_description *from_desc,
      {
      if (from_desc->format == to_desc->format)
            assert(value->num_components == 4);
   /* No support for 16 or 64 bit data in the shader for image loads/stores */
   assert(value->bit_size == 32);
   /* Overall format size needs to be the same */
   assert(from_desc->block.bits == to_desc->block.bits);
                     /* First, construct a "tightly packed" vector of the input values. For unorm/snorm, convert
   * from the float we're given into the original bits (only happens while storing). For packed
   * formats that don't fall on a nice bit size, convert/pack them into 32bit values. Otherwise,
   * just produce a vecNx4 where N is the expected bit size.
   */
   nir_def *src_as_vec;
   if (from_desc->format == PIPE_FORMAT_R10G10B10A2_UINT ||
      from_desc->format == PIPE_FORMAT_R10G10B10A2_UNORM) {
   if (from_desc->format == PIPE_FORMAT_R10G10B10A2_UNORM)
         nir_def *channels[4];
   for (unsigned i = 0; i < 4; ++i)
            src_as_vec = channels[0];
   src_as_vec = nir_mask_shift_or(b, src_as_vec, channels[1], (1 << 10) - 1, 10);
   src_as_vec = nir_mask_shift_or(b, src_as_vec, channels[2], (1 << 10) - 1, 20);
      } else if (from_desc->format == PIPE_FORMAT_R11G11B10_FLOAT) {
         } else if (from_desc->is_unorm) {
      if (from_desc->channel[0].size == 8)
         else {
      nir_def *packed_channels[2];
   packed_channels[0] = nir_pack_unorm_2x16(b,
         packed_channels[1] = nir_pack_unorm_2x16(b, nir_channels(b, value, 0x3 << 2));
         } else if (from_desc->is_snorm) {
      if (from_desc->channel[0].size == 8)
         else {
      nir_def *packed_channels[2];
   packed_channels[0] = nir_pack_snorm_2x16(b,
         packed_channels[1] = nir_pack_snorm_2x16(b, nir_channels(b, value, 0x3 << 2));
         } else if (util_format_is_float(from_desc->format)) {
         } else if (util_format_is_pure_sint(from_desc->format)) {
         } else {
                  /* Now that we have the tightly packed bits, we can use nir_extract_bits to get it into a
   * vector of differently-sized components. For producing packed formats, get a 32-bit
   * value and manually extract the bits. For unorm/snorm, get one or two 32-bit values,
   * and extract it using helpers. Otherwise, get a format-sized dest vector and use a
   * cast to expand it back to 32-bit.
   * 
   * Pay extra attention for changing semantics for alpha as 1.
   */
   if (to_desc->format == PIPE_FORMAT_R10G10B10A2_UINT ||
      to_desc->format == PIPE_FORMAT_R10G10B10A2_UNORM) {
   nir_def *u32 = nir_extract_bits(b, &src_as_vec, 1, 0, 1, 32);
   nir_def *channels[4] = {
      nir_iand_imm(b, u32,                      (1 << 10) - 1),
   nir_iand_imm(b, nir_ushr_imm(b, u32, 10), (1 << 10) - 1),
   nir_iand_imm(b, nir_ushr_imm(b, u32, 20), (1 << 10) - 1),
      };
   nir_def *vec = nir_vec(b, channels, 4);
   if (to_desc->format == PIPE_FORMAT_R10G10B10A2_UNORM)
            } else if (to_desc->format == PIPE_FORMAT_R11G11B10_FLOAT) {
      nir_def *u32 = nir_extract_bits(b, &src_as_vec, 1, 0, 1, 32);
   nir_def *vec3 = nir_format_unpack_11f11f10f(b, u32);
   return nir_vec4(b, nir_channel(b, vec3, 0),
                  } else if (to_desc->is_unorm || to_desc->is_snorm) {
      nir_def *dest_packed = nir_extract_bits(b, &src_as_vec, 1, 0,
         if (to_desc->is_unorm) {
      if (to_desc->channel[0].size == 8) {
      nir_def *unpacked = nir_unpack_unorm_4x8(b, nir_channel(b, dest_packed, 0));
   if (to_desc->nr_channels < 4)
            }
   nir_def *vec2s[2] = {
      nir_unpack_unorm_2x16(b, nir_channel(b, dest_packed, 0)),
   to_desc->nr_channels > 2 ?
      nir_unpack_unorm_2x16(b, nir_channel(b, dest_packed, 1)) :
   };
   if (to_desc->nr_channels == 1)
         return nir_vec4(b, nir_channel(b, vec2s[0], 0),
                  } else {
      if (to_desc->channel[0].size == 8) {
      nir_def *unpacked = nir_unpack_snorm_4x8(b, nir_channel(b, dest_packed, 0));
   if (to_desc->nr_channels < 4)
            }
   nir_def *vec2s[2] = {
      nir_unpack_snorm_2x16(b, nir_channel(b, dest_packed, 0)),
   to_desc->nr_channels > 2 ?
      nir_unpack_snorm_2x16(b, nir_channel(b, dest_packed, 1)) :
   };
   if (to_desc->nr_channels == 1)
         return nir_vec4(b, nir_channel(b, vec2s[0], 0),
                     } else {
      nir_def *dest_packed = nir_extract_bits(b, &src_as_vec, 1, 0,
         nir_def *final_channels[4];
   for (unsigned i = 0; i < 4; ++i) {
      if (i >= dest_packed->num_components)
      final_channels[i] = util_format_is_float(to_desc->format) ?
   nir_imm_floatN_t(b, i == 3 ? 1.0f : 0.0f, to_desc->channel[0].size) :
      else
      }
   nir_def *final_vec = nir_vec(b, final_channels, 4);
   if (util_format_is_float(to_desc->format))
         else if (util_format_is_pure_sint(to_desc->format))
         else
         }
      static bool
   lower_image_cast_instr(nir_builder *b, nir_intrinsic_instr *intr, void *_data)
   {
      if (intr->intrinsic != nir_intrinsic_image_deref_load &&
      intr->intrinsic != nir_intrinsic_image_deref_store)
         const struct d3d12_image_format_conversion_info_arr* info = _data;
   nir_variable *image = nir_intrinsic_get_var(intr, 0);
            if (image->data.driver_location >= info->n_images)
            enum pipe_format emulation_format = info->image_format_conversion[image->data.driver_location].emulated_format;
   if (emulation_format == PIPE_FORMAT_NONE)
            enum pipe_format real_format = info->image_format_conversion[image->data.driver_location].view_format;
            nir_def *value;
   const struct util_format_description *from_desc, *to_desc;
   if (intr->intrinsic == nir_intrinsic_image_deref_load) {
      b->cursor = nir_after_instr(&intr->instr);
   value = &intr->def;
   from_desc = util_format_description(emulation_format);
      } else {
      b->cursor = nir_before_instr(&intr->instr);
   value = intr->src[3].ssa;
   from_desc = util_format_description(real_format);
                        nir_alu_type alu_type = util_format_is_pure_uint(emulation_format) ?
      nir_type_uint : (util_format_is_pure_sint(emulation_format) ?
         if (intr->intrinsic == nir_intrinsic_image_deref_load) {
      nir_def_rewrite_uses_after(value, new_value, new_value->parent_instr);
      } else {
      nir_src_rewrite(&intr->src[3], new_value);
      }
   nir_intrinsic_set_format(intr, emulation_format);
      }
      /* Given a shader that does image loads/stores expecting to load from the format embedded in the intrinsic,
   * if the corresponding entry in formats is not PIPE_FORMAT_NONE, replace the image format and convert
   * the data being loaded/stored to/from the app's expected format.
   */
   bool
   d3d12_lower_image_casts(nir_shader *s, struct d3d12_image_format_conversion_info_arr *info)
   {
      bool progress = nir_shader_intrinsics_pass(s, lower_image_cast_instr,
                  if (progress) {
      nir_foreach_image_variable(var, s) {
      if (var->data.driver_location < info->n_images && info->image_format_conversion[var->data.driver_location].emulated_format != PIPE_FORMAT_NONE) {
                           }
