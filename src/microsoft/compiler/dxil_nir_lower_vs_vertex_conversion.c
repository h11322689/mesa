   /*
   * Copyright © Microsoft Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "dxil_nir.h"
      #include "nir_builder.h"
   #include "nir_builtin_builder.h"
      static enum pipe_format
   get_input_target_format(nir_variable *var, const void *options)
   {
      enum pipe_format *target_formats = (enum pipe_format *)options;
      }
      static bool
   lower_vs_vertex_conversion_filter(const nir_instr *instr, const void *options)
   {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *inst = nir_instr_as_intrinsic(instr);
   if (inst->intrinsic != nir_intrinsic_load_deref)
            nir_variable *var = nir_intrinsic_get_var(inst, 0);
   return (var->data.mode == nir_var_shader_in) &&
      }
      typedef  nir_def *
   (*shift_right_func)(nir_builder *build, nir_def *src0, nir_def *src1);
      /* decoding the signed vs unsigned scaled format is handled
   * by applying the signed or unsigned shift right function
   * accordingly */
   static nir_def *
   from_10_10_10_2_scaled(nir_builder *b, nir_def *src,
         {
      nir_def *rshift = nir_imm_ivec4(b, 22, 22, 22, 30);
      }
      static nir_def *
   from_10_10_10_2_snorm(nir_builder *b, nir_def *src, nir_def *lshift)
   {
      nir_def *split = from_10_10_10_2_scaled(b, src, lshift, nir_ishr);
   nir_def *scale_rgb = nir_imm_vec4(b,
                              }
      static nir_def *
   from_10_10_10_2_unorm(nir_builder *b, nir_def *src, nir_def *lshift)
   {
      nir_def *split = from_10_10_10_2_scaled(b, src, lshift, nir_ushr);
   nir_def *scale_rgb = nir_imm_vec4(b,
                              }
      inline static nir_def *
   lshift_rgba(nir_builder *b)
   {
         }
      inline static nir_def *
   lshift_bgra(nir_builder *b)
   {
         }
      static nir_def *
   lower_vs_vertex_conversion_impl(nir_builder *b, nir_instr *instr, void *options)
   {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   nir_variable *var = nir_intrinsic_get_var(intr, 0);
            if (!util_format_has_alpha(fmt)) {
      /* these formats need the alpha channel replaced with 1: */
   assert(fmt == PIPE_FORMAT_R8G8B8_SINT ||
         fmt == PIPE_FORMAT_R8G8B8_UINT ||
   fmt == PIPE_FORMAT_R16G16B16_SINT ||
   if (intr->def.num_components == 3)
            } else {
               switch (fmt) {
   case PIPE_FORMAT_R10G10B10A2_SNORM:
         case PIPE_FORMAT_B10G10R10A2_SNORM:
         case PIPE_FORMAT_B10G10R10A2_UNORM:
         case PIPE_FORMAT_R10G10B10A2_SSCALED:
         case PIPE_FORMAT_B10G10R10A2_SSCALED:
         case PIPE_FORMAT_R10G10B10A2_USCALED:
         case PIPE_FORMAT_B10G10R10A2_USCALED:
         case PIPE_FORMAT_R8G8B8A8_USCALED:
   case PIPE_FORMAT_R16G16B16A16_USCALED:
         case PIPE_FORMAT_R8G8B8A8_SSCALED:
   case PIPE_FORMAT_R16G16B16A16_SSCALED:
            default:
               }
      /* Lower emulated vertex attribute input
   * The vertex attributes are passed as R32_UINT that needs to be converted
   * to one of the RGB10A2 formats that need to be emulated.
   *
   * @param target_formats contains the per attribute format to convert to
   * or PIPE_FORMAT_NONE if no conversion is needed
   */
   bool
   dxil_nir_lower_vs_vertex_conversion(nir_shader *s,
         {
               bool result =
         nir_shader_lower_instructions(s,
                  }
