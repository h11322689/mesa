   /*
   * Copyright © 2015 Broadcom
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
      /**
   * Implements most of the fixed function fragment pipeline in shader code.
   *
   * VC4 doesn't have any hardware support for blending, alpha test, logic ops,
   * or color mask.  Instead, you read the current contents of the destination
   * from the tile buffer after having waited for the scoreboard (which is
   * handled by vc4_qpu_emit.c), then do math using your output color and that
   * destination value, and update the output color appropriately.
   *
   * Once this pass is done, the color write will either have one component (for
   * single sample) with packed argb8888, or 4 components with the per-sample
   * argb8888 result.
   */
      /**
   * Lowers fixed-function blending to a load of the destination color and a
   * series of ALU operations before the store of the output.
   */
   #include "util/format/u_format.h"
   #include "vc4_qir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_format_convert.h"
   #include "vc4_context.h"
      static bool
   blend_depends_on_dst_color(struct vc4_compile *c)
   {
         return (c->fs_key->blend.blend_enable ||
         }
      /** Emits a load of the previous fragment color from the tile buffer. */
   static nir_def *
   vc4_nir_get_dst_color(nir_builder *b, int sample)
   {
         return nir_load_input(b, 1, 32, nir_imm_int(b, 0),
   }
      static nir_def *
   vc4_blend_channel_f(nir_builder *b,
                     nir_def **src,
   {
         switch(factor) {
   case PIPE_BLENDFACTOR_ONE:
         case PIPE_BLENDFACTOR_SRC_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA:
         case PIPE_BLENDFACTOR_DST_ALPHA:
         case PIPE_BLENDFACTOR_DST_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
            if (channel != 3) {
            return nir_fmin(b,
            } else {
      case PIPE_BLENDFACTOR_CONST_COLOR:
            return nir_load_system_value(b,
            case PIPE_BLENDFACTOR_CONST_ALPHA:
         case PIPE_BLENDFACTOR_ZERO:
         case PIPE_BLENDFACTOR_INV_SRC_COLOR:
         case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_COLOR:
         case PIPE_BLENDFACTOR_INV_CONST_COLOR:
            return nir_fsub_imm(b, 1.0,
                  case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
                  default:
   case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
            /* Unsupported. */
      }
      static nir_def *
   vc4_nir_set_packed_chan(nir_builder *b, nir_def *src0, nir_def *src1,
         {
         unsigned chan_mask = 0xff << (chan * 8);
   return nir_ior(b,
         }
      static nir_def *
   vc4_blend_channel_i(nir_builder *b,
                     nir_def *src,
   nir_def *dst,
   nir_def *src_a,
   {
         switch (factor) {
   case PIPE_BLENDFACTOR_ONE:
         case PIPE_BLENDFACTOR_SRC_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA:
         case PIPE_BLENDFACTOR_DST_ALPHA:
         case PIPE_BLENDFACTOR_DST_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
            return vc4_nir_set_packed_chan(b,
                        case PIPE_BLENDFACTOR_CONST_COLOR:
         case PIPE_BLENDFACTOR_CONST_ALPHA:
         case PIPE_BLENDFACTOR_ZERO:
         case PIPE_BLENDFACTOR_INV_SRC_COLOR:
         case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_COLOR:
         case PIPE_BLENDFACTOR_INV_CONST_COLOR:
               case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
                  default:
   case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
            /* Unsupported. */
      }
      static nir_def *
   vc4_blend_func_f(nir_builder *b, nir_def *src, nir_def *dst,
         {
         switch (func) {
   case PIPE_BLEND_ADD:
         case PIPE_BLEND_SUBTRACT:
         case PIPE_BLEND_REVERSE_SUBTRACT:
         case PIPE_BLEND_MIN:
         case PIPE_BLEND_MAX:
            default:
                        }
      static nir_def *
   vc4_blend_func_i(nir_builder *b, nir_def *src, nir_def *dst,
         {
         switch (func) {
   case PIPE_BLEND_ADD:
         case PIPE_BLEND_SUBTRACT:
         case PIPE_BLEND_REVERSE_SUBTRACT:
         case PIPE_BLEND_MIN:
         case PIPE_BLEND_MAX:
            default:
                        }
      static void
   vc4_do_blending_f(struct vc4_compile *c, nir_builder *b, nir_def **result,
         {
                  if (!blend->blend_enable) {
            for (int i = 0; i < 4; i++)
               /* Clamp the src color to [0, 1].  Dest is already clamped. */
   for (int i = 0; i < 4; i++)
            nir_def *src_blend[4], *dst_blend[4];
   for (int i = 0; i < 4; i++) {
            int src_factor = ((i != 3) ? blend->rgb_src_factor :
         int dst_factor = ((i != 3) ? blend->rgb_dst_factor :
         src_blend[i] = nir_fmul(b, src_color[i],
                     dst_blend[i] = nir_fmul(b, dst_color[i],
                     for (int i = 0; i < 4; i++) {
            result[i] = vc4_blend_func_f(b, src_blend[i], dst_blend[i],
      }
      static nir_def *
   vc4_nir_splat(nir_builder *b, nir_def *src)
   {
         nir_def *or1 = nir_ior(b, src, nir_ishl_imm(b, src, 8));
   }
      static nir_def *
   vc4_do_blending_i(struct vc4_compile *c, nir_builder *b,
               {
                  if (!blend->blend_enable)
            enum pipe_format color_format = c->fs_key->color_format;
   const uint8_t *format_swiz = vc4_get_format_swizzle(color_format);
   nir_def *src_a = nir_pack_unorm_4x8(b, src_float_a);
   nir_def *dst_a;
   int alpha_chan;
   for (alpha_chan = 0; alpha_chan < 4; alpha_chan++) {
               }
   if (alpha_chan != 4) {
            dst_a = vc4_nir_splat(b, nir_iand_imm(b, nir_ushr_imm(b, dst_color,
      } else {
                  nir_def *src_factor = vc4_blend_channel_i(b,
                           nir_def *dst_factor = vc4_blend_channel_i(b,
                              if (alpha_chan != 4 &&
         blend->alpha_src_factor != blend->rgb_src_factor) {
      nir_def *src_alpha_factor =
            vc4_blend_channel_i(b,
                  src_factor = vc4_nir_set_packed_chan(b, src_factor,
      }
   if (alpha_chan != 4 &&
         blend->alpha_dst_factor != blend->rgb_dst_factor) {
      nir_def *dst_alpha_factor =
            vc4_blend_channel_i(b,
                  dst_factor = vc4_nir_set_packed_chan(b, dst_factor,
      }
   nir_def *src_blend = nir_umul_unorm_4x8_vc4(b, src_color, src_factor);
            nir_def *result =
         if (alpha_chan != 4 && blend->alpha_func != blend->rgb_func) {
            nir_def *result_a = vc4_blend_func_i(b,
                        }
   }
      static nir_def *
   vc4_logicop(nir_builder *b, int logicop_func,
         {
         switch (logicop_func) {
   case PIPE_LOGICOP_CLEAR:
         case PIPE_LOGICOP_NOR:
         case PIPE_LOGICOP_AND_INVERTED:
         case PIPE_LOGICOP_COPY_INVERTED:
         case PIPE_LOGICOP_AND_REVERSE:
         case PIPE_LOGICOP_INVERT:
         case PIPE_LOGICOP_XOR:
         case PIPE_LOGICOP_NAND:
         case PIPE_LOGICOP_AND:
         case PIPE_LOGICOP_EQUIV:
         case PIPE_LOGICOP_NOOP:
         case PIPE_LOGICOP_OR_INVERTED:
         case PIPE_LOGICOP_OR_REVERSE:
         case PIPE_LOGICOP_OR:
         case PIPE_LOGICOP_SET:
         default:
               case PIPE_LOGICOP_COPY:
         }
      static nir_def *
   vc4_nir_swizzle_and_pack(struct vc4_compile *c, nir_builder *b,
         {
         enum pipe_format color_format = c->fs_key->color_format;
            nir_def *swizzled[4];
   for (int i = 0; i < 4; i++) {
                        return nir_pack_unorm_4x8(b,
                  }
      static nir_def *
   vc4_nir_blend_pipeline(struct vc4_compile *c, nir_builder *b, nir_def *src,
         {
         enum pipe_format color_format = c->fs_key->color_format;
   const uint8_t *format_swiz = vc4_get_format_swizzle(color_format);
            /* Pull out the float src/dst color components. */
   nir_def *packed_dst_color = vc4_nir_get_dst_color(b, sample);
   nir_def *dst_vec4 = nir_unpack_unorm_4x8(b, packed_dst_color);
   nir_def *src_color[4], *unpacked_dst_color[4];
   for (unsigned i = 0; i < 4; i++) {
                        if (c->fs_key->sample_alpha_to_one && c->fs_key->msaa)
            nir_def *packed_color;
   if (srgb) {
            /* Unswizzle the destination color. */
   nir_def *dst_color[4];
   for (unsigned i = 0; i < 4; i++) {
                                                                                    } else {
                           packed_color =
                     packed_color = vc4_logicop(b, c->fs_key->logicop_func,
            /* If the bit isn't set in the color mask, then just return the
      * original dst color, instead.
      uint32_t colormask = 0xffffffff;
   for (int i = 0; i < 4; i++) {
            if (format_swiz[i] < 4 &&
      !(c->fs_key->blend.colormask & (1 << format_swiz[i]))) {
            return nir_ior(b,
         }
      static void
   vc4_nir_store_sample_mask(struct vc4_compile *c, nir_builder *b,
         {
         nir_variable *sample_mask = nir_variable_create(c->s, nir_var_shader_out,
               sample_mask->data.driver_location = c->s->num_outputs++;
            nir_store_output(b, val, nir_imm_int(b, 0),
   }
      static void
   vc4_nir_lower_blend_instr(struct vc4_compile *c, nir_builder *b,
         {
                  if (c->fs_key->sample_alpha_to_coverage) {
                     /* XXX: We should do a nice dither based on the fragment
   * coordinate, instead.
   */
   nir_def *num_bits = nir_f2i32(b, nir_fmul_imm(b, a, VC4_MAX_SAMPLES));
   nir_def *bitmask = nir_iadd_imm(b,
                                 /* The TLB color read returns each sample in turn, so if our blending
      * depends on the destination color, we're going to have to run the
   * blending function separately for each destination sample value, and
   * then output the per-sample color using TLB_COLOR_MS.
      nir_def *blend_output;
   if (c->fs_key->msaa && blend_depends_on_dst_color(c)) {
                     nir_def *samples[4];
   for (int i = 0; i < VC4_MAX_SAMPLES; i++)
         blend_output = nir_vec4(b,
      } else {
                  nir_src_rewrite(&intr->src[0], blend_output);
   if (intr->num_components != blend_output->num_components) {
            unsigned component_mask = BITFIELD_MASK(blend_output->num_components);
      }
      static bool
   vc4_nir_lower_blend_block(nir_block *block, struct vc4_compile *c)
   {
         nir_foreach_instr_safe(instr, block) {
            if (instr->type != nir_instr_type_intrinsic)
                              nir_variable *output_var = NULL;
   nir_foreach_shader_out_variable(var, c->s) {
            if (var->data.driver_location ==
      nir_intrinsic_base(intr)) {
                        if (output_var->data.location != FRAG_RESULT_COLOR &&
                           }
   }
      void
   vc4_nir_lower_blend(nir_shader *s, struct vc4_compile *c)
   {
         nir_foreach_function_impl(impl, s) {
                                 nir_metadata_preserve(impl,
               /* If we didn't do alpha-to-coverage on the output color, we still
      * need to pass glSampleMask() through.
      if (c->fs_key->sample_coverage && !c->fs_key->sample_alpha_to_coverage) {
                        }
