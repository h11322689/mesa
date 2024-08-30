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
      #include "pan_blend.h"
   #include "util/blend.h"
      #ifdef PAN_ARCH
   #include "pan_shader.h"
   #endif
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_conversion_builder.h"
   #include "compiler/nir/nir_lower_blend.h"
   #include "panfrost/util/pan_lower_framebuffer.h"
   #include "util/format/u_format.h"
   #include "pan_texture.h"
      #ifndef PAN_ARCH
      /* Fixed function blending */
      static bool
   factor_is_supported(enum pipe_blendfactor factor)
   {
               return factor != PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE &&
            }
      /* OpenGL allows encoding (src*dest + dest*src) which is incompatiblle with
   * Midgard style blending since there are two multiplies. However, it may be
   * factored as 2*src*dest = dest*(2*src), which can be encoded on Bifrost as 0
   * + dest * (2*src) wih the new source_2 value of C. Detect this case. */
      static bool
   is_2srcdest(enum pipe_blend_func blend_func, enum pipe_blendfactor src_factor,
         {
      return (blend_func == PIPE_BLEND_ADD) &&
         ((src_factor == PIPE_BLENDFACTOR_DST_COLOR) ||
   ((src_factor == PIPE_BLENDFACTOR_DST_ALPHA) && is_alpha)) &&
      }
      static bool
   can_fixed_function_equation(enum pipe_blend_func blend_func,
                     {
      if (is_2srcdest(blend_func, src_factor, dest_factor, is_alpha))
            if (blend_func != PIPE_BLEND_ADD && blend_func != PIPE_BLEND_SUBTRACT &&
      blend_func != PIPE_BLEND_REVERSE_SUBTRACT)
         if (!factor_is_supported(src_factor) || !factor_is_supported(dest_factor))
            /* Fixed function requires src/dest factors to match (up to invert) or be
   * zero/one.
   */
   enum pipe_blendfactor src = util_blendfactor_without_invert(src_factor);
            return (src == dest) || (src == PIPE_BLENDFACTOR_ONE) ||
      }
      static unsigned
   blend_factor_constant_mask(enum pipe_blendfactor factor)
   {
               if (factor == PIPE_BLENDFACTOR_CONST_COLOR)
         else if (factor == PIPE_BLENDFACTOR_CONST_ALPHA)
         else
      }
      unsigned
   pan_blend_constant_mask(const struct pan_blend_equation eq)
   {
      return blend_factor_constant_mask(eq.rgb_src_factor) |
         blend_factor_constant_mask(eq.rgb_dst_factor) |
      }
      /* Only "homogenous" (scalar or vector with all components equal) constants are
   * valid for fixed-function, so check for this condition */
      bool
   pan_blend_is_homogenous_constant(unsigned mask, const float *constants)
   {
               u_foreach_bit(i, mask) {
      if (constants[i] != constant)
                  }
      /* Determines if an equation can run in fixed function */
      bool
   pan_blend_can_fixed_function(const struct pan_blend_equation equation,
         {
      return !equation.blend_enable ||
         (can_fixed_function_equation(
      equation.rgb_func, equation.rgb_src_factor,
      can_fixed_function_equation(
      }
      static enum mali_blend_operand_c
   to_c_factor(enum pipe_blendfactor factor)
   {
      switch (util_blendfactor_without_invert(factor)) {
   case PIPE_BLENDFACTOR_ONE:
      /* Extra invert to flip back in caller */
         case PIPE_BLENDFACTOR_SRC_ALPHA:
            case PIPE_BLENDFACTOR_DST_ALPHA:
            case PIPE_BLENDFACTOR_SRC_COLOR:
            case PIPE_BLENDFACTOR_DST_COLOR:
            case PIPE_BLENDFACTOR_CONST_COLOR:
   case PIPE_BLENDFACTOR_CONST_ALPHA:
            default:
            }
      static void
   to_panfrost_function(enum pipe_blend_func blend_func,
                     {
      assert(can_fixed_function_equation(blend_func, src_factor, dest_factor,
            /* We handle ZERO/ONE specially since it's the hardware has 0 and can invert
   * to 1 but Gallium has 0 as the uninverted version.
   */
   bool src_inverted =
      util_blendfactor_is_inverted(src_factor) ^
         bool dest_inverted =
      util_blendfactor_is_inverted(dest_factor) ^
         if (src_factor == PIPE_BLENDFACTOR_ZERO) {
      function->a = MALI_BLEND_OPERAND_A_ZERO;
   function->b = MALI_BLEND_OPERAND_B_DEST;
   if (blend_func == PIPE_BLEND_SUBTRACT)
         function->invert_c = dest_inverted;
      } else if (src_factor == PIPE_BLENDFACTOR_ONE) {
      function->a = MALI_BLEND_OPERAND_A_SRC;
   function->b = MALI_BLEND_OPERAND_B_DEST;
   if (blend_func == PIPE_BLEND_SUBTRACT)
         else if (blend_func == PIPE_BLEND_REVERSE_SUBTRACT)
         function->invert_c = dest_inverted;
      } else if (dest_factor == PIPE_BLENDFACTOR_ZERO) {
      function->a = MALI_BLEND_OPERAND_A_ZERO;
   function->b = MALI_BLEND_OPERAND_B_SRC;
   if (blend_func == PIPE_BLEND_REVERSE_SUBTRACT)
         function->invert_c = src_inverted;
      } else if (dest_factor == PIPE_BLENDFACTOR_ONE) {
      function->a = MALI_BLEND_OPERAND_A_DEST;
   function->b = MALI_BLEND_OPERAND_B_SRC;
   if (blend_func == PIPE_BLEND_SUBTRACT)
         else if (blend_func == PIPE_BLEND_REVERSE_SUBTRACT)
         function->invert_c = src_inverted;
      } else if (src_factor == dest_factor) {
      function->a = MALI_BLEND_OPERAND_A_ZERO;
   function->invert_c = src_inverted;
            switch (blend_func) {
   case PIPE_BLEND_ADD:
      function->b = MALI_BLEND_OPERAND_B_SRC_PLUS_DEST;
      case PIPE_BLEND_REVERSE_SUBTRACT:
      function->negate_b = true;
      case PIPE_BLEND_SUBTRACT:
      function->b = MALI_BLEND_OPERAND_B_SRC_MINUS_DEST;
      default:
            } else if (is_2srcdest(blend_func, src_factor, dest_factor, is_alpha)) {
      /* src*dest + dest*src = 2*src*dest = 0 + dest*(2*src) */
   function->a = MALI_BLEND_OPERAND_A_ZERO;
   function->b = MALI_BLEND_OPERAND_B_DEST;
      } else {
      assert(util_blendfactor_without_invert(src_factor) ==
                  function->a = MALI_BLEND_OPERAND_A_DEST;
   function->invert_c = src_inverted;
            switch (blend_func) {
   case PIPE_BLEND_ADD:
      function->b = MALI_BLEND_OPERAND_B_SRC_MINUS_DEST;
      case PIPE_BLEND_REVERSE_SUBTRACT:
      function->b = MALI_BLEND_OPERAND_B_SRC_PLUS_DEST;
   function->negate_b = true;
      case PIPE_BLEND_SUBTRACT:
      function->b = MALI_BLEND_OPERAND_B_SRC_PLUS_DEST;
   function->negate_a = true;
      default:
               }
      bool
   pan_blend_is_opaque(const struct pan_blend_equation equation)
   {
      /* If a channel is masked out, we can't use opaque mode even if
   * blending is disabled, since we need a tilebuffer read in there */
   if (equation.color_mask != 0xF)
            /* With nothing masked out, disabled bledning is opaque */
   if (!equation.blend_enable)
            /* Also detect open-coded opaque blending */
   return equation.rgb_src_factor == PIPE_BLENDFACTOR_ONE &&
         equation.rgb_dst_factor == PIPE_BLENDFACTOR_ZERO &&
   (equation.rgb_func == PIPE_BLEND_ADD ||
   equation.rgb_func == PIPE_BLEND_SUBTRACT) &&
   equation.alpha_src_factor == PIPE_BLENDFACTOR_ONE &&
   equation.alpha_dst_factor == PIPE_BLENDFACTOR_ZERO &&
      }
      /* Check if a factor represents a constant value of val, assuming src_alpha is
   * the given constant.
   */
      static inline bool
   is_factor_01(enum pipe_blendfactor factor, unsigned val, unsigned srca)
   {
      assert(val == 0 || val == 1);
            switch (factor) {
   case PIPE_BLENDFACTOR_ZERO:
            case PIPE_BLENDFACTOR_ONE:
            case PIPE_BLENDFACTOR_SRC_ALPHA:
            case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
            default:
            }
      /* Returns if src alpha = 0 implies the blended colour equals the destination
   * colour. Suppose source alpha = 0 and consider cases.
   *
   * Additive blending: Equivalent to D = S * f_s + D * f_d for all D and all S
   * with S_a = 0, for each component. For the alpha component (if it unmasked),
   * we have S_a = 0 so this reduces to D = D * f_d <===> f_d = 1. For RGB
   * components (if unmasked), we need f_s = 0 and f_d = 1.
   *
   * Subtractive blending: Fails in general (D = S * f_S - D * f_D). We
   * would need f_S = 0 and f_D = -1, which is not valid in the APIs.
   *
   * Reverse subtractive blending (D = D * f_D - S * f_S), we need f_D = 1
   * and f_S = 0 up to masking. This is the same as additive blending.
   *
   * Min/max: Fails in general on the RGB components.
   */
      bool
   pan_blend_alpha_zero_nop(const struct pan_blend_equation eq)
   {
      if (eq.rgb_func != PIPE_BLEND_ADD &&
      eq.rgb_func != PIPE_BLEND_REVERSE_SUBTRACT)
         if (eq.color_mask & 0x8) {
      if (!is_factor_01(eq.alpha_dst_factor, 1, 0))
               if (eq.color_mask & 0x7) {
      if (!is_factor_01(eq.rgb_dst_factor, 1, 0))
            if (!is_factor_01(eq.rgb_src_factor, 0, 0))
                  }
      /* Returns if src alpha = 1 implies the blended colour equals the source
   * colour. Suppose source alpha = 1 and consider cases.
   *
   * Additive blending: S = S * f_s + D * f_d. We need f_s = 1 and f_d = 0.
   *
   * Subtractive blending: S = S * f_s - D * f_d. Same as additive blending.
   *
   * Reverse subtractive blending: S = D * f_d - S * f_s. Fails in general since
   * it would require f_s = -1, which is not valid in the APIs.
   *
   * Min/max: Fails in general on the RGB components.
   *
   * Note if any component is masked, we can't use a store.
   */
      bool
   pan_blend_alpha_one_store(const struct pan_blend_equation eq)
   {
      if (eq.rgb_func != PIPE_BLEND_ADD && eq.rgb_func != PIPE_BLEND_SUBTRACT)
            if (eq.color_mask != 0xf)
            return is_factor_01(eq.rgb_src_factor, 1, 1) &&
         is_factor_01(eq.alpha_src_factor, 1, 1) &&
      }
      static bool
   is_dest_factor(enum pipe_blendfactor factor, bool alpha)
   {
               return factor == PIPE_BLENDFACTOR_DST_ALPHA ||
            }
      /* Determines if a blend equation reads back the destination. This can occur by
   * explicitly referencing the destination in the blend equation, or by using a
   * partial writemask. */
      bool
   pan_blend_reads_dest(const struct pan_blend_equation equation)
   {
      if (equation.color_mask && equation.color_mask != 0xF)
            if (!equation.blend_enable)
            return is_dest_factor(equation.rgb_src_factor, false) ||
         is_dest_factor(equation.alpha_src_factor, true) ||
      }
      /* Create the descriptor for a fixed blend mode given the corresponding API
   * state. Assumes the equation can be represented as fixed-function. */
      void
   pan_blend_to_fixed_function_equation(const struct pan_blend_equation equation,
         {
      /* If no blending is enabled, default back on `replace` mode */
   if (!equation.blend_enable) {
      out->color_mask = equation.color_mask;
   out->rgb.a = MALI_BLEND_OPERAND_A_SRC;
   out->rgb.b = MALI_BLEND_OPERAND_B_SRC;
   out->rgb.c = MALI_BLEND_OPERAND_C_ZERO;
   out->alpha.a = MALI_BLEND_OPERAND_A_SRC;
   out->alpha.b = MALI_BLEND_OPERAND_B_SRC;
   out->alpha.c = MALI_BLEND_OPERAND_C_ZERO;
               /* Compile the fixed-function blend */
   to_panfrost_function(equation.rgb_func, equation.rgb_src_factor,
         to_panfrost_function(equation.alpha_func, equation.alpha_src_factor,
               }
      uint32_t
   pan_pack_blend(const struct pan_blend_equation equation)
   {
                        pan_pack(&out, BLEND_EQUATION, cfg) {
                     }
      static uint32_t
   pan_blend_shader_key_hash(const void *key)
   {
         }
      static bool
   pan_blend_shader_key_equal(const void *a, const void *b)
   {
         }
      void
   pan_blend_shaders_init(struct panfrost_device *dev)
   {
      dev->blend_shaders.shaders = _mesa_hash_table_create(
            }
      void
   pan_blend_shaders_cleanup(struct panfrost_device *dev)
   {
         }
      #else /* ifndef PAN_ARCH */
      static const char *
   logicop_str(enum pipe_logicop logicop)
   {
      switch (logicop) {
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
         case PIPE_LOGICOP_COPY:
         case PIPE_LOGICOP_OR_REVERSE:
         case PIPE_LOGICOP_OR:
         case PIPE_LOGICOP_SET:
         default:
            }
      static void
   get_equation_str(const struct pan_blend_rt_state *rt_state, char *str,
         {
      const char *funcs[] = {
         };
   const char *factors[] = {
      "",           "one",           "src_color",   "src_alpha",   "dst_alpha",
   "dst_color",  "src_alpha_sat", "const_color", "const_alpha", "src1_color",
      };
            if (!rt_state->equation.blend_enable) {
      ret = snprintf(str, len, "replace(%s%s%s%s)",
                  (rt_state->equation.color_mask & 1) ? "R" : "",
      assert(ret > 0);
               if (rt_state->equation.color_mask & 7) {
      assert(rt_state->equation.rgb_func < ARRAY_SIZE(funcs));
   ret = snprintf(
      str, len, "%s%s%s(func=%s,src_factor=%s%s,dst_factor=%s%s)%s",
   (rt_state->equation.color_mask & 1) ? "R" : "",
   (rt_state->equation.color_mask & 2) ? "G" : "",
   (rt_state->equation.color_mask & 4) ? "B" : "",
   funcs[rt_state->equation.rgb_func],
   util_blendfactor_is_inverted(rt_state->equation.rgb_src_factor) ? "-"
         factors[util_blendfactor_without_invert(
         util_blendfactor_is_inverted(rt_state->equation.rgb_dst_factor) ? "-"
         factors[util_blendfactor_without_invert(
            assert(ret > 0);
   str += ret;
               if (rt_state->equation.color_mask & 8) {
      assert(rt_state->equation.alpha_func < ARRAY_SIZE(funcs));
   ret = snprintf(
      str, len, "A(func=%s,src_factor=%s%s,dst_factor=%s%s)",
   funcs[rt_state->equation.alpha_func],
   util_blendfactor_is_inverted(rt_state->equation.alpha_src_factor) ? "-"
         factors[util_blendfactor_without_invert(
         util_blendfactor_is_inverted(rt_state->equation.alpha_dst_factor) ? "-"
         factors[util_blendfactor_without_invert(
      assert(ret > 0);
   str += ret;
         }
      static bool
   pan_inline_blend_constants(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_load_blend_const_color_rgba)
            float *floats = data;
   const nir_const_value constants[4] = {
      nir_const_value_for_float(floats[0], 32),
   nir_const_value_for_float(floats[1], 32),
   nir_const_value_for_float(floats[2], 32),
         b->cursor = nir_after_instr(&intr->instr);
   nir_def *constant = nir_build_imm(b, 4, 32, constants);
   nir_def_rewrite_uses(&intr->def, constant);
   nir_instr_remove(&intr->instr);
      }
      nir_shader *
   GENX(pan_blend_create_shader)(const struct panfrost_device *dev,
                     {
      const struct pan_blend_rt_state *rt_state = &state->rts[rt];
                     nir_builder b = nir_builder_init_simple_shader(
      MESA_SHADER_FRAGMENT, GENX(pan_shader_get_compiler_options)(),
   "pan_blend(rt=%d,fmt=%s,nr_samples=%d,%s=%s)", rt,
   util_format_name(rt_state->format), rt_state->nr_samples,
   state->logicop_enable ? "logicop" : "equation",
         const struct util_format_description *format_desc =
                  /* Bifrost/Valhall support 16-bit and 32-bit register formats for
   * LD_TILE/ST_TILE/BLEND, but do not support 8-bit. Rather than making
   * the fragment output 8-bit and inserting extra conversions in the
   * compiler, promote the output to 16-bit. The larger size is still
   * compatible with correct conversion semantics.
   */
   if (PAN_ARCH >= 6 && nir_alu_type_get_type_size(nir_type) == 8)
            nir_lower_blend_options options = {
      .logicop_enable = state->logicop_enable,
               options.rt[rt].colormask = rt_state->equation.color_mask;
            if (!rt_state->equation.blend_enable) {
      static const nir_lower_blend_channel replace = {
      .func = PIPE_BLEND_ADD,
   .src_factor = PIPE_BLENDFACTOR_ONE,
               options.rt[rt].rgb = replace;
      } else {
      options.rt[rt].rgb.func = rt_state->equation.rgb_func;
   options.rt[rt].rgb.src_factor = rt_state->equation.rgb_src_factor;
   options.rt[rt].rgb.dst_factor = rt_state->equation.rgb_dst_factor;
   options.rt[rt].alpha.func = rt_state->equation.alpha_func;
   options.rt[rt].alpha.src_factor = rt_state->equation.alpha_src_factor;
               nir_def *pixel = nir_load_barycentric_pixel(&b, 32, .interp_mode = 1);
            for (unsigned i = 0; i < 2; ++i) {
      nir_alu_type src_type =
            /* HACK: workaround buggy TGSI shaders (u_blitter) */
   src_type = nir_alu_type_get_base_type(nir_type) |
            nir_def *src = nir_load_interpolated_input(
      &b, 4, nir_alu_type_get_type_size(src_type), pixel, zero,
               /* On Midgard, the blend shader is responsible for format conversion.
   * As the OpenGL spec requires integer conversions to saturate, we must
   * saturate ourselves here. On Bifrost and later, the conversion
   * hardware handles this automatically.
   */
   nir_alu_type T = nir_alu_type_get_base_type(nir_type);
   bool should_saturate = (PAN_ARCH <= 5) && (T != nir_type_float);
   src = nir_convert_with_rounding(&b, src, T, nir_type,
            nir_store_output(&b, src, zero, .write_mask = BITFIELD_MASK(4),
                  .src_type = nir_type,
                     NIR_PASS_V(b.shader, nir_lower_blend, &options);
   nir_shader_intrinsics_pass(b.shader, pan_inline_blend_constants,
                     }
      #if PAN_ARCH >= 6
   uint64_t
   GENX(pan_blend_get_internal_desc)(const struct panfrost_device *dev,
               {
      const struct util_format_description *desc = util_format_description(fmt);
            pan_pack(&res, INTERNAL_BLEND, cfg) {
      cfg.mode = MALI_BLEND_MODE_OPAQUE;
   cfg.fixed_function.num_comps = desc->nr_channels;
                     if (force_size)
            switch (T) {
   case nir_type_float16:
      cfg.fixed_function.conversion.register_format =
            case nir_type_float32:
      cfg.fixed_function.conversion.register_format =
            case nir_type_int8:
   case nir_type_int16:
      cfg.fixed_function.conversion.register_format =
            case nir_type_int32:
      cfg.fixed_function.conversion.register_format =
            case nir_type_uint8:
   case nir_type_uint16:
      cfg.fixed_function.conversion.register_format =
            case nir_type_uint32:
      cfg.fixed_function.conversion.register_format =
            default:
                  cfg.fixed_function.conversion.memory_format =
                  }
      struct rt_conversion_inputs {
      const struct panfrost_device *dev;
      };
      static bool
   inline_rt_conversion(nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      if (intr->intrinsic != nir_intrinsic_load_rt_conversion_pan)
            struct rt_conversion_inputs *inputs = data;
   unsigned rt = nir_intrinsic_base(intr);
   unsigned size = nir_alu_type_get_type_size(nir_intrinsic_src_type(intr));
   uint64_t conversion = GENX(pan_blend_get_internal_desc)(
            b->cursor = nir_after_instr(&intr->instr);
   nir_def_rewrite_uses(&intr->def, nir_imm_int(b, conversion >> 32));
      }
      bool
   GENX(pan_inline_rt_conversion)(nir_shader *s, const struct panfrost_device *dev,
         {
      return nir_shader_intrinsics_pass(
      s, inline_rt_conversion,
   nir_metadata_block_index | nir_metadata_dominance,
   }
   #endif
      struct pan_blend_shader_variant *
   GENX(pan_blend_get_shader_locked)(const struct panfrost_device *dev,
                     {
      struct pan_blend_shader_key key = {
      .format = state->rts[rt].format,
   .src0_type = src0_type,
   .src1_type = src1_type,
   .rt = rt,
   .has_constants = pan_blend_constant_mask(state->rts[rt].equation) != 0,
   .logicop_enable = state->logicop_enable,
   .logicop_func = state->logicop_func,
   .nr_samples = state->rts[rt].nr_samples,
               /* Blend shaders should only be used for blending on Bifrost onwards */
   assert(dev->arch <= 5 || state->logicop_enable ||
                  struct hash_entry *he =
                  if (!shader) {
      shader = rzalloc(dev->blend_shaders.shaders, struct pan_blend_shader);
   shader->key = key;
   list_inithead(&shader->variants);
               list_for_each_entry(struct pan_blend_shader_variant, iter, &shader->variants,
            if (!key.has_constants ||
      !memcmp(iter->constants, state->constants, sizeof(iter->constants))) {
                           if (shader->nvariants < PAN_BLEND_SHADER_MAX_VARIANTS) {
      variant = rzalloc(shader, struct pan_blend_shader_variant);
   util_dynarray_init(&variant->binary, variant);
   list_add(&variant->node, &shader->variants);
      } else {
      variant = list_last_entry(&shader->variants,
         list_del(&variant->node);
   list_add(&variant->node, &shader->variants);
                        nir_shader *nir =
            /* Compile the NIR shader */
   struct panfrost_compile_inputs inputs = {
      .gpu_id = dev->gpu_id,
   .is_blend = true,
               enum pipe_format rt_formats[8] = {0};
         #if PAN_ARCH >= 6
      inputs.blend.bifrost_blend_desc =
      #endif
         struct pan_shader_info info;
         #if PAN_ARCH >= 6
         #else
      NIR_PASS_V(nir, pan_lower_framebuffer, rt_formats,
            #endif
                        #if PAN_ARCH <= 5
         #endif
                     }
   #endif /* ifndef PAN_ARCH */
