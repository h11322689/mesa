   /*
   * Copyright (C) 2018 Alyssa Rosenzweig
   * Copyright (C) 2019-2021 Collabora, Ltd.
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
   */
      #include "pan_shader.h"
   #include "pan_device.h"
   #include "pan_format.h"
      #if PAN_ARCH <= 5
   #include "panfrost/midgard/midgard_compile.h"
   #else
   #include "panfrost/compiler/bifrost_compile.h"
   #endif
      const nir_shader_compiler_options *
   GENX(pan_shader_get_compiler_options)(void)
   {
   #if PAN_ARCH >= 9
         #elif PAN_ARCH >= 6
         #else
         #endif
   }
      #if PAN_ARCH >= 6
   static enum mali_register_file_format
   bifrost_blend_type_from_nir(nir_alu_type nir_type)
   {
      switch (nir_type) {
   case 0: /* Render target not in use */
         case nir_type_float16:
         case nir_type_float32:
         case nir_type_int32:
         case nir_type_uint32:
         case nir_type_int16:
         case nir_type_uint16:
         default:
      unreachable("Unsupported blend shader type for NIR alu type");
         }
      #if PAN_ARCH <= 7
   enum mali_register_file_format
   GENX(pan_fixup_blend_type)(nir_alu_type T_size, enum pipe_format format)
   {
      const struct util_format_description *desc = util_format_description(format);
   unsigned size = nir_alu_type_get_type_size(T_size);
   nir_alu_type T_format = pan_unpacked_type_for_format(desc);
               }
   #endif
   #endif
      /* This is only needed on Midgard. It's the same on both v4 and v5, so only
   * compile once to avoid the GenXML dependency for calls.
   */
   #if PAN_ARCH == 5
   uint8_t
   pan_raw_format_mask_midgard(enum pipe_format *formats)
   {
               for (unsigned i = 0; i < 8; i++) {
      enum pipe_format fmt = formats[i];
            if (wb_fmt < MALI_COLOR_FORMAT_R8)
                  }
   #endif
      void
   GENX(pan_shader_compile)(nir_shader *s, struct panfrost_compile_inputs *inputs,
               {
            #if PAN_ARCH >= 6
         #else
         #endif
         info->stage = s->info.stage;
   info->contains_barrier =
                  switch (info->stage) {
   case MESA_SHADER_VERTEX:
      info->attributes_read = s->info.inputs_read;
   info->attributes_read_count = util_bitcount64(info->attributes_read);
      #if PAN_ARCH <= 5
         bool vertex_id = BITSET_TEST(s->info.system_values_read,
         if (vertex_id)
            bool instance_id =
         if (instance_id)
         #endif
            info->vs.writes_point_size =
      #if PAN_ARCH >= 9
         info->varyings.output_count =
   #endif
            case MESA_SHADER_FRAGMENT:
      if (s->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH))
         if (s->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_STENCIL))
         if (s->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK))
            info->fs.outputs_read = s->info.outputs_read >> FRAG_RESULT_DATA0;
   info->fs.outputs_written = s->info.outputs_written >> FRAG_RESULT_DATA0;
   info->fs.sample_shading = s->info.fs.uses_sample_shading;
            info->fs.can_discard = s->info.fs.uses_discard;
            /* List of reasons we need to execute frag shaders when things
            info->fs.sidefx = s->info.writes_memory || s->info.fs.uses_discard ||
            /* With suitable ZSA/blend, is early-z possible? */
   info->fs.can_early_z = !info->fs.sidefx && !info->fs.writes_depth &&
                  /* Similiarly with suitable state, is FPK possible? */
   info->fs.can_fpk = !info->fs.writes_depth && !info->fs.writes_stencil &&
                  /* Requires the same hardware guarantees, so grouped as one bit
   * in the hardware.
   */
            info->fs.reads_frag_coord =
      (s->info.inputs_read & (1 << VARYING_SLOT_POS)) ||
      info->fs.reads_point_coord =
         info->fs.reads_face =
         #if PAN_ARCH >= 9
         info->varyings.output_count =
   #endif
            default:
      /* Everything else treated as compute */
   info->wls_size = s->info.shared_size;
               info->outputs_written = s->info.outputs_written;
   info->attribute_count += BITSET_LAST_BIT(s->info.images_used);
   info->writes_global = s->info.writes_memory;
            info->sampler_count = info->texture_count =
            unsigned execution_mode = s->info.float_controls_execution_mode;
   info->ftz_fp16 = nir_is_denorm_flush_to_zero(execution_mode, 16);
         #if PAN_ARCH >= 6
      /* This is "redundant" information, but is needed in a draw-time hot path */
   for (unsigned i = 0; i < ARRAY_SIZE(info->bifrost.blend); ++i) {
      info->bifrost.blend[i].format =
         #endif
   }
