   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "agx_nir_lower_vbo.h"
   #include "asahi/compiler/agx_internal_formats.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_format_convert.h"
   #include "util/u_math.h"
      static bool
   is_rgb10_a2(const struct util_format_description *desc)
   {
      return desc->channel[0].shift == 0 && desc->channel[0].size == 10 &&
         desc->channel[1].shift == 10 && desc->channel[1].size == 10 &&
      }
      static enum pipe_format
   agx_vbo_internal_format(enum pipe_format format)
   {
               /* RGB10A2 formats are native for UNORM and unpacked otherwise */
   if (is_rgb10_a2(desc)) {
      if (desc->is_unorm)
         else
               /* R11G11B10F is native and special */
   if (format == PIPE_FORMAT_R11G11B10_FLOAT)
            /* No other non-array formats handled */
   if (!desc->is_array)
            /* Otherwise look at one (any) channel */
   int idx = util_format_get_first_non_void_channel(format);
   if (idx < 0)
            /* We only handle RGB formats (we could do SRGB if we wanted though?) */
   if ((desc->colorspace != UTIL_FORMAT_COLORSPACE_RGB) ||
      (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN))
         /* We have native 8-bit and 16-bit normalized formats */
            if (chan.normalized) {
      if (chan.size == 8)
         else if (chan.size == 16)
               /* Otherwise map to the corresponding integer format */
   switch (chan.size) {
   case 32:
         case 16:
         case 8:
         default:
            }
      bool
   agx_vbo_supports_format(enum pipe_format format)
   {
         }
      static nir_def *
   apply_swizzle_channel(nir_builder *b, nir_def *vec, unsigned swizzle,
         {
      switch (swizzle) {
   case PIPE_SWIZZLE_X:
         case PIPE_SWIZZLE_Y:
         case PIPE_SWIZZLE_Z:
         case PIPE_SWIZZLE_W:
         case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
      return is_int ? nir_imm_intN_t(b, 1, vec->bit_size)
      default:
            }
      static bool
   pass(struct nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_load_input)
            struct agx_vbufs *vbufs = data;
            nir_src *offset_src = nir_get_io_offset_src(intr);
   assert(nir_src_is_const(*offset_src) && "no attribute indirects");
            struct agx_attribute attrib = vbufs->attributes[index];
   uint32_t stride = attrib.stride;
            const struct util_format_description *desc =
         int chan = util_format_get_first_non_void_channel(attrib.format);
            bool is_float = desc->channel[chan].type == UTIL_FORMAT_TYPE_FLOAT;
   bool is_unsigned = desc->channel[chan].type == UTIL_FORMAT_TYPE_UNSIGNED;
   bool is_signed = desc->channel[chan].type == UTIL_FORMAT_TYPE_SIGNED;
   bool is_fixed = desc->channel[chan].type == UTIL_FORMAT_TYPE_FIXED;
                     enum pipe_format interchange_format = agx_vbo_internal_format(attrib.format);
            unsigned interchange_align = util_format_get_blocksize(interchange_format);
            /* In the hardware, uint formats zero-extend and float formats convert.
   * However, non-uint formats using a uint interchange format shouldn't be
   * zero extended.
   */
   unsigned interchange_register_size =
      util_format_is_pure_uint(interchange_format) &&
                     /* Non-UNORM R10G10B10A2 loaded as a scalar and unpacked */
   if (interchange_format == PIPE_FORMAT_R32_UINT && !desc->is_array)
            /* Calculate the element to fetch the vertex for. Divide the instance ID by
   * the divisor for per-instance data. Divisor=0 specifies per-vertex data.
   */
   nir_def *el = (attrib.divisor == 0)
                           assert((stride % interchange_align) == 0 && "must be aligned");
            unsigned stride_el = stride / interchange_align;
   unsigned offset_el = offset / interchange_align;
            /* Try to use the small shift on the load itself when possible. This can save
   * an instruction. Shifts are only available for regular interchange formats,
   * i.e. the set of formats that support masking.
   */
   if (offset_el == 0 && (stride_el == 2 || stride_el == 4) &&
      agx_internal_format_supports_mask(
            shift = util_logbase2(stride_el);
               nir_def *stride_offset_el =
            /* Load the raw vector */
   nir_def *memory = nir_load_constant_agx(
      b, interchange_comps, interchange_register_size, base, stride_offset_el,
                  /* Unpack but do not convert non-native non-array formats */
   if (is_rgb10_a2(desc) && interchange_format == PIPE_FORMAT_R32_UINT) {
               if (is_signed)
         else
               if (desc->channel[chan].normalized) {
      /* 8/16-bit normalized formats are native, others converted here */
   if (is_rgb10_a2(desc) && is_signed) {
      unsigned bits[] = {10, 10, 10, 2};
      } else if (desc->channel[chan].size == 32) {
                     if (is_signed)
         else
         } else if (desc->channel[chan].pure_integer) {
      /* Zero-extension is native, may need to sign extend */
   if (is_signed)
      } else {
      if (is_unsigned)
         else if (is_signed || is_fixed)
         else
            /* 16.16 fixed-point weirdo GL formats need to be scaled */
   if (is_fixed) {
      assert(desc->is_array && desc->channel[chan].size == 32);
   assert(dest_size == 32 && "overflow if smaller");
                  /* We now have a properly formatted vector of the components in memory. Apply
   * the format swizzle forwards to trim/pad/reorder as needed.
   */
            for (unsigned i = 0; i < intr->num_components; ++i) {
      unsigned c = nir_intrinsic_component(intr) + i;
               nir_def *logical = nir_vec(b, channels, intr->num_components);
   nir_def_rewrite_uses(&intr->def, logical);
      }
      bool
   agx_nir_lower_vbo(nir_shader *shader, struct agx_vbufs *vbufs)
   {
      assert(shader->info.stage == MESA_SHADER_VERTEX);
   return nir_shader_instructions_pass(
      }
