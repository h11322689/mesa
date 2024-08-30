   /*
   * Copyright © 2014-2015 Broadcom
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
      #include "nir.h"
   #include "nir_builder.h"
      struct alu_width_data {
      nir_vectorize_cb cb;
      };
      /** @file nir_lower_alu_width.c
   *
   * Replaces nir_alu_instr operations with more than one channel used in the
   * arguments with individual per-channel operations.
   *
   * Optionally, a callback function which returns the max vectorization width
   * per instruction can be provided.
   *
   * The max vectorization width must be a power of 2.
   */
      static bool
   inst_is_vector_alu(const nir_instr *instr, const void *_state)
   {
      if (instr->type != nir_instr_type_alu)
                     /* There is no ALU instruction which has a scalar destination, scalar
   * src[0], and some other vector source.
   */
   return alu->def.num_components > 1 ||
      }
      /* Checks whether all operands of an ALU instruction are swizzled
   * within the targeted vectorization width.
   *
   * The assumption here is that a vecN instruction can only swizzle
   * within the first N channels of the values it consumes, irrespective
   * of the capabilities of the instruction which produced those values.
   * If we assume values are packed consistently (i.e., they always start
   * at the beginning of a hardware register), we can actually access any
   * aligned group of N channels so long as we stay within the group.
   * This means for a vectorization width of 4 that only swizzles from
   * either [xyzw] or [abcd] etc are allowed.  For a width of 2 these are
   * swizzles from either [xy] or [zw] etc.
   */
   static bool
   alu_is_swizzled_in_bounds(const nir_alu_instr *alu, unsigned width)
   {
      for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      if (nir_op_infos[alu->op].input_sizes[i] == 1)
            unsigned mask = ~(width - 1);
   for (unsigned j = 1; j < alu->def.num_components; j++) {
      if ((alu->src[i].swizzle[0] & mask) != (alu->src[i].swizzle[j] & mask))
                     }
      static void
   nir_alu_ssa_dest_init(nir_alu_instr *alu, unsigned num_components,
         {
         }
      static nir_def *
   lower_reduction(nir_alu_instr *alu, nir_op chan_op, nir_op merge_op,
         {
               nir_def *last = NULL;
   for (int i = 0; i < num_components; i++) {
      int channel = reverse_order ? num_components - 1 - i : i;
   nir_alu_instr *chan = nir_alu_instr_create(builder->shader, chan_op);
   nir_alu_ssa_dest_init(chan, 1, alu->def.bit_size);
   nir_alu_src_copy(&chan->src[0], &alu->src[0]);
   chan->src[0].swizzle[0] = chan->src[0].swizzle[channel];
   if (nir_op_infos[chan_op].num_inputs > 1) {
      assert(nir_op_infos[chan_op].num_inputs == 2);
   nir_alu_src_copy(&chan->src[1], &alu->src[1]);
      }
                     if (i == 0) {
         } else {
      last = nir_build_alu(builder, merge_op,
                     }
      static inline bool
   will_lower_ffma(nir_shader *shader, unsigned bit_size)
   {
      switch (bit_size) {
   case 16:
         case 32:
         case 64:
         }
      }
      static nir_def *
   lower_fdot(nir_alu_instr *alu, nir_builder *builder)
   {
      /* Reversed order can result in lower instruction count because it
   * creates more MAD/FMA in the case of fdot(a, vec4(b, 1.0)).
   * Some games expect xyzw order, so only reverse the order for imprecise fdot.
   */
            /* If we don't want to lower ffma, create several ffma instead of fmul+fadd
   * and fusing later because fusing is not possible for exact fdot instructions.
   */
   if (will_lower_ffma(builder->shader, alu->def.bit_size))
                     nir_def *prev = NULL;
   for (int i = 0; i < num_components; i++) {
      int channel = reverse_order ? num_components - 1 - i : i;
   nir_alu_instr *instr = nir_alu_instr_create(
         nir_alu_ssa_dest_init(instr, 1, alu->def.bit_size);
   for (unsigned j = 0; j < 2; j++) {
      nir_alu_src_copy(&instr->src[j], &alu->src[j]);
      }
   if (i != 0)
                                          }
      static nir_def *
   lower_alu_instr_width(nir_builder *b, nir_instr *instr, void *_data)
   {
      struct alu_width_data *data = _data;
   nir_alu_instr *alu = nir_instr_as_alu(instr);
   unsigned num_src = nir_op_infos[alu->op].num_inputs;
                     unsigned num_components = alu->def.num_components;
            if (data->cb) {
      target_width = data->cb(instr, data->data);
   assert(util_is_power_of_two_or_zero(target_width));
   if (target_width == 0)
            #define LOWER_REDUCTION(name, chan, merge) \
      case name##2:                           \
   case name##3:                           \
   case name##4:                           \
   case name##8:                           \
   case name##16:                          \
            switch (alu->op) {
   case nir_op_vec16:
   case nir_op_vec8:
   case nir_op_vec5:
   case nir_op_vec4:
   case nir_op_vec3:
   case nir_op_vec2:
   case nir_op_cube_amd:
      /* We don't need to scalarize these ops, they're the ones generated to
   * group up outputs into a value that can be SSAed.
   */
         case nir_op_pack_half_2x16: {
      if (!b->shader->options->lower_pack_half_2x16)
            nir_def *src_vec2 = nir_ssa_for_alu_src(b, alu, 0);
   return nir_pack_half_2x16_split(b, nir_channel(b, src_vec2, 0),
               case nir_op_unpack_unorm_4x8:
   case nir_op_unpack_snorm_4x8:
   case nir_op_unpack_unorm_2x16:
   case nir_op_unpack_snorm_2x16:
      /* There is no scalar version of these ops, unless we were to break it
   * down to bitshifts and math (which is definitely not intended).
   */
         case nir_op_unpack_half_2x16_flush_to_zero:
   case nir_op_unpack_half_2x16: {
      if (!b->shader->options->lower_unpack_half_2x16)
            nir_def *packed = nir_ssa_for_alu_src(b, alu, 0);
   if (alu->op == nir_op_unpack_half_2x16_flush_to_zero) {
      return nir_vec2(b,
                  nir_unpack_half_2x16_split_x_flush_to_zero(b,
   } else {
      return nir_vec2(b,
                        case nir_op_pack_uvec2_to_uint: {
      assert(b->shader->options->lower_pack_snorm_2x16 ||
            nir_def *word = nir_extract_u16(b, nir_ssa_for_alu_src(b, alu, 0),
         return nir_ior(b, nir_ishl(b, nir_channel(b, word, 1), nir_imm_int(b, 16)),
               case nir_op_pack_uvec4_to_uint: {
      assert(b->shader->options->lower_pack_snorm_4x8 ||
            nir_def *byte = nir_extract_u8(b, nir_ssa_for_alu_src(b, alu, 0),
         return nir_ior(b, nir_ior(b, nir_ishl(b, nir_channel(b, byte, 3), nir_imm_int(b, 24)), nir_ishl(b, nir_channel(b, byte, 2), nir_imm_int(b, 16))),
                     case nir_op_fdph: {
      nir_def *src0_vec = nir_ssa_for_alu_src(b, alu, 0);
            /* Only use reverse order for imprecise fdph, see explanation in lower_fdot. */
   bool reverse_order = !b->exact;
   if (will_lower_ffma(b->shader, alu->def.bit_size)) {
      nir_def *sum[4];
   for (unsigned i = 0; i < 3; i++) {
      int dest = reverse_order ? 3 - i : i;
   sum[dest] = nir_fmul(b, nir_channel(b, src0_vec, i),
                        } else if (reverse_order) {
      nir_def *sum = nir_channel(b, src1_vec, 3);
   for (int i = 2; i >= 0; i--)
            } else {
      nir_def *sum = nir_fmul(b, nir_channel(b, src0_vec, 0), nir_channel(b, src1_vec, 0));
   sum = nir_ffma(b, nir_channel(b, src0_vec, 1), nir_channel(b, src1_vec, 1), sum);
   sum = nir_ffma(b, nir_channel(b, src0_vec, 2), nir_channel(b, src1_vec, 2), sum);
                  case nir_op_pack_64_2x32: {
      if (!b->shader->options->lower_pack_64_2x32)
            nir_def *src_vec2 = nir_ssa_for_alu_src(b, alu, 0);
   return nir_pack_64_2x32_split(b, nir_channel(b, src_vec2, 0),
      }
   case nir_op_pack_64_4x16: {
      if (!b->shader->options->lower_pack_64_4x16)
            nir_def *src_vec4 = nir_ssa_for_alu_src(b, alu, 0);
   nir_def *xy = nir_pack_32_2x16_split(b, nir_channel(b, src_vec4, 0),
         nir_def *zw = nir_pack_32_2x16_split(b, nir_channel(b, src_vec4, 2),
               }
   case nir_op_pack_32_2x16: {
      if (!b->shader->options->lower_pack_32_2x16)
            nir_def *src_vec2 = nir_ssa_for_alu_src(b, alu, 0);
   return nir_pack_32_2x16_split(b, nir_channel(b, src_vec2, 0),
      }
   case nir_op_unpack_64_2x32:
   case nir_op_unpack_64_4x16:
   case nir_op_unpack_32_2x16:
   case nir_op_unpack_32_4x8:
   case nir_op_unpack_double_2x32_dxil:
            case nir_op_fdot2:
   case nir_op_fdot3:
   case nir_op_fdot4:
   case nir_op_fdot8:
   case nir_op_fdot16:
               LOWER_REDUCTION(nir_op_ball_fequal, nir_op_feq, nir_op_iand);
   LOWER_REDUCTION(nir_op_ball_iequal, nir_op_ieq, nir_op_iand);
   LOWER_REDUCTION(nir_op_bany_fnequal, nir_op_fneu, nir_op_ior);
   LOWER_REDUCTION(nir_op_bany_inequal, nir_op_ine, nir_op_ior);
   LOWER_REDUCTION(nir_op_b8all_fequal, nir_op_feq8, nir_op_iand);
   LOWER_REDUCTION(nir_op_b8all_iequal, nir_op_ieq8, nir_op_iand);
   LOWER_REDUCTION(nir_op_b8any_fnequal, nir_op_fneu8, nir_op_ior);
   LOWER_REDUCTION(nir_op_b8any_inequal, nir_op_ine8, nir_op_ior);
   LOWER_REDUCTION(nir_op_b16all_fequal, nir_op_feq16, nir_op_iand);
   LOWER_REDUCTION(nir_op_b16all_iequal, nir_op_ieq16, nir_op_iand);
   LOWER_REDUCTION(nir_op_b16any_fnequal, nir_op_fneu16, nir_op_ior);
   LOWER_REDUCTION(nir_op_b16any_inequal, nir_op_ine16, nir_op_ior);
   LOWER_REDUCTION(nir_op_b32all_fequal, nir_op_feq32, nir_op_iand);
   LOWER_REDUCTION(nir_op_b32all_iequal, nir_op_ieq32, nir_op_iand);
   LOWER_REDUCTION(nir_op_b32any_fnequal, nir_op_fneu32, nir_op_ior);
   LOWER_REDUCTION(nir_op_b32any_inequal, nir_op_ine32, nir_op_ior);
   LOWER_REDUCTION(nir_op_fall_equal, nir_op_seq, nir_op_fmin);
         default:
                  if (num_components == 1)
            if (num_components <= target_width) {
      /* If the ALU instr is swizzled outside the target width,
   * reduce the target width.
   */
   if (alu_is_swizzled_in_bounds(alu, target_width))
         else
                        for (chan = 0; chan < num_components; chan += target_width) {
      unsigned components = MIN2(target_width, num_components - chan);
            for (i = 0; i < num_src; i++) {
               /* We only handle same-size-as-dest (input_sizes[] == 0) or scalar
   * args (input_sizes[] == 1).
   */
   assert(nir_op_infos[alu->op].input_sizes[i] < 2);
   for (int j = 0; j < components; j++) {
      unsigned src_chan = nir_op_infos[alu->op].input_sizes[i] == 1 ? 0 : chan + j;
                  nir_alu_ssa_dest_init(lower, components, alu->def.bit_size);
            for (i = 0; i < components; i++) {
      vec->src[chan + i].src = nir_src_for_ssa(&lower->def);
                              }
      bool
   nir_lower_alu_width(nir_shader *shader, nir_vectorize_cb cb, const void *_data)
   {
      struct alu_width_data data = {
      .cb = cb,
               return nir_shader_lower_instructions(shader,
                  }
      struct alu_to_scalar_data {
      nir_instr_filter_cb cb;
      };
      static uint8_t
   scalar_cb(const nir_instr *instr, const void *data)
   {
      /* return vectorization-width = 1 for filtered instructions */
   const struct alu_to_scalar_data *filter = data;
      }
      bool
   nir_lower_alu_to_scalar(nir_shader *shader, nir_instr_filter_cb cb, const void *_data)
   {
      struct alu_to_scalar_data data = {
      .cb = cb,
                  }
      static bool
   lower_alu_vec8_16_src(nir_builder *b, nir_instr *instr, void *_data)
   {
      if (instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(instr);
            bool changed = false;
   b->cursor = nir_before_instr(instr);
   for (int i = 0; i < info->num_inputs; i++) {
      if (alu->src[i].src.ssa->num_components < 8 || info->input_sizes[i])
            changed = true;
   nir_def *comps[4];
   for (int c = 0; c < alu->def.num_components; c++) {
                     nir_const_value *const_val = nir_src_as_const_value(alu->src[i].src);
   if (const_val) {
         } else {
            }
   nir_def *src = nir_vec(b, comps, alu->def.num_components);
                  }
      bool
   nir_lower_alu_vec8_16_srcs(nir_shader *shader)
   {
      return nir_shader_instructions_pass(shader, lower_alu_vec8_16_src,
      nir_metadata_block_index | nir_metadata_dominance,
   }
