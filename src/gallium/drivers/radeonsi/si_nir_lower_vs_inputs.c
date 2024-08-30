   /*
   * Copyright 2023 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "nir_builder.h"
      #include "ac_nir.h"
   #include "si_shader_internal.h"
   #include "si_state.h"
   #include "si_pipe.h"
      struct lower_vs_inputs_state {
      struct si_shader *shader;
            nir_def *instance_divisor_constbuf;
      };
      /* See fast_idiv_by_const.h. */
   /* If num != UINT_MAX, this more efficient version can be used. */
   /* Set: increment = util_fast_udiv_info::increment; */
   static nir_def *
   fast_udiv_nuw(nir_builder *b, nir_def *num, nir_def *divisor)
   {
      nir_def *multiplier = nir_channel(b, divisor, 0);
   nir_def *pre_shift = nir_channel(b, divisor, 1);
   nir_def *post_shift = nir_channel(b, divisor, 2);
            num = nir_ushr(b, num, pre_shift);
   num = nir_iadd_nuw(b, num, increment);
   num = nir_umul_high(b, num, multiplier);
      }
      static nir_def *
   get_vertex_index_for_mono_shader(nir_builder *b, int input_index,
         {
               bool divisor_is_one =
         bool divisor_is_fetched =
            if (divisor_is_one || divisor_is_fetched) {
               /* This is used to determine vs vgpr count in si_get_vs_vgpr_comp_cnt(). */
            nir_def *index = NULL;
   if (divisor_is_one) {
         } else {
      nir_def *offset = nir_imm_int(b, input_index * 16);
                  /* The faster NUW version doesn't work when InstanceID == UINT_MAX.
   * Such InstanceID might not be achievable in a reasonable time though.
   */
               nir_def *start_instance = nir_load_base_instance(b);
      } else {
      nir_def *vertex_id = nir_load_vertex_id_zero_base(b);
                  }
      static nir_def *
   get_vertex_index_for_part_shader(nir_builder *b, int input_index,
         {
         }
      static void
   get_vertex_index_for_all_inputs(nir_shader *nir, struct lower_vs_inputs_state *s)
   {
               nir_builder builder = nir_builder_at(nir_before_impl(impl));
            const struct si_shader_selector *sel = s->shader->selector;
            if (key->ge.part.vs.prolog.instance_divisor_is_fetched) {
      s->instance_divisor_constbuf =
               for (int i = 0; i < sel->info.num_inputs; i++) {
      s->vertex_index[i] = s->shader->is_monolithic ?
      get_vertex_index_for_mono_shader(b, i, s) :
      }
      static void
   load_vs_input_from_blit_sgpr(nir_builder *b, unsigned input_index,
               {
      nir_def *vertex_id = nir_load_vertex_id_zero_base(b);
   nir_def *sel_x1 = nir_ule_imm(b, vertex_id, 1);
   /* Use nir_ine, because we have 3 vertices and only
   * the middle one should use y2.
   */
            if (input_index == 0) {
      /* Position: */
   nir_def *x1y1 = ac_nir_load_arg_at_offset(b, &s->args->ac, s->args->vs_blit_inputs, 0);
            x1y1 = nir_i2i32(b, nir_unpack_32_2x16(b, x1y1));
            nir_def *x1 = nir_channel(b, x1y1, 0);
   nir_def *y1 = nir_channel(b, x1y1, 1);
   nir_def *x2 = nir_channel(b, x2y2, 0);
            out[0] = nir_i2f32(b, nir_bcsel(b, sel_x1, x1, x2));
   out[1] = nir_i2f32(b, nir_bcsel(b, sel_y1, y1, y2));
   out[2] = ac_nir_load_arg_at_offset(b, &s->args->ac, s->args->vs_blit_inputs, 2);
      } else {
               /* Color or texture coordinates: */
            unsigned vs_blit_property = s->shader->selector->info.base.vs.blit_sgprs_amd;
   if (vs_blit_property == SI_VS_BLIT_SGPRS_POS_COLOR + has_attribute_ring_address) {
      for (int i = 0; i < 4; i++)
      } else {
               nir_def *x1 = ac_nir_load_arg_at_offset(b, &s->args->ac, s->args->vs_blit_inputs, 3);
   nir_def *y1 = ac_nir_load_arg_at_offset(b, &s->args->ac, s->args->vs_blit_inputs, 4);
                  out[0] = nir_bcsel(b, sel_x1, x1, x2);
   out[1] = nir_bcsel(b, sel_y1, y1, y2);
   out[2] = ac_nir_load_arg_at_offset(b, &s->args->ac, s->args->vs_blit_inputs, 7);
            }
      /**
   * Convert an 11- or 10-bit unsigned floating point number to an f32.
   *
   * The input exponent is expected to be biased analogous to IEEE-754, i.e. by
   * 2^(exp_bits-1) - 1 (as defined in OpenGL and other graphics APIs).
   */
   static nir_def *
   ufN_to_float(nir_builder *b, nir_def *src, unsigned exp_bits, unsigned mant_bits)
   {
                        /* Converting normal numbers is just a shift + correcting the exponent bias */
   unsigned normal_shift = 23 - mant_bits;
            nir_def *shifted = nir_ishl_imm(b, src, normal_shift);
            /* Converting nan/inf numbers is the same, but with a different exponent update */
            /* Converting denormals is the complex case: determine the leading zeros of the
   * mantissa to obtain the correct shift for the mantissa and exponent correction.
   */
   nir_def *ctlz = nir_uclz(b, mantissa);
   /* Shift such that the leading 1 ends up as the LSB of the exponent field. */
            unsigned denormal_exp = bias_shift + (32 - mant_bits) - 1;
   nir_def *tmp = nir_isub_imm(b, denormal_exp, ctlz);
            /* Select the final result. */
   nir_def *cond = nir_uge_imm(b, src, ((1ULL << exp_bits) - 1) << mant_bits);
            cond = nir_uge_imm(b, src, 1ULL << mant_bits);
            cond = nir_ine_imm(b, src, 0);
               }
      /**
   * Generate a fully general open coded buffer format fetch with all required
   * fixups suitable for vertex fetch, using non-format buffer loads.
   *
   * Some combinations of argument values have special interpretations:
   * - size = 8 bytes, format = fixed indicates PIPE_FORMAT_R11G11B10_FLOAT
   * - size = 8 bytes, format != {float,fixed} indicates a 2_10_10_10 data format
   */
   static void
   opencoded_load_format(nir_builder *b, nir_def *rsrc, nir_def *vindex,
               {
      unsigned log_size = fix_fetch.u.log_size;
   unsigned num_channels = fix_fetch.u.num_channels_m1 + 1;
   unsigned format = fix_fetch.u.format;
            unsigned load_log_size = log_size;
   unsigned load_num_channels = num_channels;
   if (log_size == 3) {
      load_log_size = 2;
   if (format == AC_FETCH_FORMAT_FLOAT) {
         } else {
                     int log_recombine = 0;
   if ((gfx_level == GFX6 || gfx_level >= GFX10) && !known_aligned) {
      /* Avoid alignment restrictions by loading one byte at a time. */
   load_num_channels <<= load_log_size;
   log_recombine = load_log_size;
      } else if (load_num_channels == 2 || load_num_channels == 4) {
      log_recombine = -util_logbase2(load_num_channels);
   load_num_channels = 1;
               nir_def *loads[32]; /* up to 32 bytes */
   for (unsigned i = 0; i < load_num_channels; ++i) {
      nir_def *soffset = nir_imm_int(b, i << load_log_size);
   unsigned num_channels = 1 << (MAX2(load_log_size, 2) - 2);
   unsigned bit_size = 8 << MIN2(load_log_size, 2);
                        if (log_recombine > 0) {
      /* Recombine bytes if necessary (GFX6 only) */
            for (unsigned src = 0, dst = 0; src < load_num_channels; ++dst) {
      nir_def *accum = NULL;
   for (unsigned i = 0; i < (1 << log_recombine); ++i, ++src) {
      nir_def *tmp = nir_u2uN(b, loads[src], dst_bitsize);
   if (i == 0) {
         } else {
      tmp = nir_ishl_imm(b, tmp, 8 * i);
         }
         } else if (log_recombine < 0) {
      /* Split vectors of dwords */
   if (load_log_size > 2) {
      assert(load_num_channels == 1);
   nir_def *loaded = loads[0];
   unsigned log_split = load_log_size - 2;
   log_recombine += log_split;
   load_num_channels = 1 << log_split;
   load_log_size = 2;
   for (unsigned i = 0; i < load_num_channels; ++i)
               /* Further split dwords and shorts if required */
   if (log_recombine < 0) {
      for (unsigned src = load_num_channels, dst = load_num_channels << -log_recombine;
      src > 0; --src) {
   unsigned dst_bits = 1 << (3 + load_log_size + log_recombine);
   nir_def *loaded = loads[src - 1];
   for (unsigned i = 1 << -log_recombine; i > 0; --i, --dst) {
      nir_def *tmp = nir_ushr_imm(b, loaded, dst_bits * (i - 1));
                        if (log_size == 3) {
      switch (format) {
   case AC_FETCH_FORMAT_FLOAT: {
      for (unsigned i = 0; i < num_channels; ++i)
            }
   case AC_FETCH_FORMAT_FIXED: {
      /* 10_11_11_FLOAT */
   nir_def *data = loads[0];
   nir_def *red = nir_iand_imm(b, data, 2047);
                  loads[0] = ufN_to_float(b, red, 5, 6);
                  num_channels = 3;
   log_size = 2;
   format = AC_FETCH_FORMAT_FLOAT;
      }
   case AC_FETCH_FORMAT_UINT:
   case AC_FETCH_FORMAT_UNORM:
   case AC_FETCH_FORMAT_USCALED: {
                     loads[0] = nir_ubfe_imm(b, data, 0, 10);
   loads[1] = nir_ubfe_imm(b, data, 10, 10);
                  num_channels = 4;
      }
   case AC_FETCH_FORMAT_SINT:
   case AC_FETCH_FORMAT_SNORM:
   case AC_FETCH_FORMAT_SSCALED: {
                     loads[0] = nir_ibfe_imm(b, data, 0, 10);
   loads[1] = nir_ibfe_imm(b, data, 10, 10);
                  num_channels = 4;
      }
   default:
      unreachable("invalid fetch format");
                  switch (format) {
   case AC_FETCH_FORMAT_FLOAT:
      if (log_size != 2) {
      for (unsigned chan = 0; chan < num_channels; ++chan)
      }
      case AC_FETCH_FORMAT_UINT:
      if (log_size != 2) {
      for (unsigned chan = 0; chan < num_channels; ++chan)
      }
      case AC_FETCH_FORMAT_SINT:
      if (log_size != 2) {
      for (unsigned chan = 0; chan < num_channels; ++chan)
      }
      case AC_FETCH_FORMAT_USCALED:
      for (unsigned chan = 0; chan < num_channels; ++chan)
            case AC_FETCH_FORMAT_SSCALED:
      for (unsigned chan = 0; chan < num_channels; ++chan)
            case AC_FETCH_FORMAT_FIXED:
      for (unsigned chan = 0; chan < num_channels; ++chan) {
      nir_def *tmp = nir_i2f32(b, loads[chan]);
      }
      case AC_FETCH_FORMAT_UNORM:
      for (unsigned chan = 0; chan < num_channels; ++chan) {
      /* 2_10_10_10 data formats */
   unsigned bits = log_size == 3 ? (chan == 3 ? 2 : 10) : (8 << log_size);
   nir_def *tmp = nir_u2f32(b, loads[chan]);
      }
      case AC_FETCH_FORMAT_SNORM:
      for (unsigned chan = 0; chan < num_channels; ++chan) {
      /* 2_10_10_10 data formats */
   unsigned bits = log_size == 3 ? (chan == 3 ? 2 : 10) : (8 << log_size);
   nir_def *tmp = nir_i2f32(b, loads[chan]);
   tmp = nir_fmul_imm(b, tmp, 1.0 / (double)BITFIELD64_MASK(bits - 1));
   /* Clamp to [-1, 1] */
   tmp = nir_fmax(b, tmp, nir_imm_float(b, -1));
      }
      default:
      unreachable("invalid fetch format");
               while (num_channels < 4) {
      unsigned pad_value = num_channels == 3 ? 1 : 0;
   loads[num_channels] =
      format == AC_FETCH_FORMAT_UINT || format == AC_FETCH_FORMAT_SINT ?
                  if (reverse) {
      nir_def *tmp = loads[0];
   loads[0] = loads[2];
                  }
      static void
   load_vs_input_from_vertex_buffer(nir_builder *b, unsigned input_index,
               {
      const struct si_shader_selector *sel = s->shader->selector;
            nir_def *vb_desc;
   if (input_index < sel->info.num_vbos_in_user_sgprs) {
         } else {
      unsigned index = input_index - sel->info.num_vbos_in_user_sgprs;
   nir_def *addr = ac_nir_load_arg(b, &s->args->ac, s->args->ac.vertex_buffers);
                        /* Use the open-coded implementation for all loads of doubles and
   * of dword-sized data that needs fixups. We need to insert conversion
   * code anyway.
   */
   bool opencode = key->ge.mono.vs_fetch_opencode & (1 << input_index);
   union si_vs_fix_fetch fix_fetch = key->ge.mono.vs_fix_fetch[input_index];
   if (opencode ||
      (fix_fetch.u.log_size == 3 && fix_fetch.u.format == AC_FETCH_FORMAT_FLOAT) ||
   fix_fetch.u.log_size == 2) {
   opencoded_load_format(b, vb_desc, vertex_index, fix_fetch, !opencode,
            if (bit_size == 16) {
      if (fix_fetch.u.format == AC_FETCH_FORMAT_UINT ||
      fix_fetch.u.format == AC_FETCH_FORMAT_SINT) {
   for (unsigned i = 0; i < 4; i++)
      } else {
      for (unsigned i = 0; i < 4; i++)
         }
               unsigned required_channels = util_last_bit(sel->info.input[input_index].usage_mask);
   if (required_channels == 0) {
      for (unsigned i = 0; i < 4; ++i)
                     /* Do multiple loads for special formats. */
   nir_def *fetches[4];
   unsigned num_fetches;
   unsigned fetch_stride;
            if (fix_fetch.u.log_size <= 1 && fix_fetch.u.num_channels_m1 == 2) {
      num_fetches = MIN2(required_channels, 3);
   fetch_stride = 1 << fix_fetch.u.log_size;
      } else {
      num_fetches = 1;
   fetch_stride = 0;
               for (unsigned i = 0; i < num_fetches; ++i) {
      nir_def *zero = nir_imm_int(b, 0);
   fetches[i] = nir_load_buffer_amd(b, channels_per_fetch, bit_size, vb_desc,
                           if (num_fetches == 1 && channels_per_fetch > 1) {
      nir_def *fetch = fetches[0];
   for (unsigned i = 0; i < channels_per_fetch; ++i)
            num_fetches = channels_per_fetch;
               for (unsigned i = num_fetches; i < 4; ++i)
            if (fix_fetch.u.log_size <= 1 && fix_fetch.u.num_channels_m1 == 2 && required_channels == 4) {
      if (fix_fetch.u.format == AC_FETCH_FORMAT_UINT || fix_fetch.u.format == AC_FETCH_FORMAT_SINT)
         else
      } else if (fix_fetch.u.log_size == 3 &&
            (fix_fetch.u.format == AC_FETCH_FORMAT_SNORM ||
                  /* For 2_10_10_10, the hardware returns an unsigned value;
   * convert it to a signed one.
   */
            /* First, recover the sign-extended signed integer value. */
   if (fix_fetch.u.format == AC_FETCH_FORMAT_SSCALED)
            /* For the integer-like cases, do a natural sign extension.
   *
   * For the SNORM case, the values are 0.0, 0.333, 0.666, 1.0
   * and happen to contain 0, 1, 2, 3 as the two LSBs of the
   * exponent.
   */
   tmp = nir_ishl_imm(b, tmp, fix_fetch.u.format == AC_FETCH_FORMAT_SNORM ? 7 : 30);
            /* Convert back to the right type. */
   if (fix_fetch.u.format == AC_FETCH_FORMAT_SNORM) {
      tmp = nir_i2fN(b, tmp, bit_size);
   /* Clamp to [-1, 1] */
   tmp = nir_fmax(b, tmp, nir_imm_float(b, -1));
      } else if (fix_fetch.u.format == AC_FETCH_FORMAT_SSCALED) {
                                 }
      static bool
   lower_vs_input_instr(nir_builder *b, nir_intrinsic_instr *intrin, void *state)
   {
      if (intrin->intrinsic != nir_intrinsic_load_input)
                              unsigned input_index = nir_intrinsic_base(intrin);
   unsigned component = nir_intrinsic_component(intrin);
            nir_def *comp[4];
   if (s->shader->selector->info.base.vs.blit_sgprs_amd)
         else
                     nir_def_rewrite_uses(&intrin->def, replacement);
   nir_instr_remove(&intrin->instr);
               }
      bool
   si_nir_lower_vs_inputs(nir_shader *nir, struct si_shader *shader, struct si_shader_args *args)
   {
               /* no inputs to lower */
   if (!sel->info.num_inputs)
            struct lower_vs_inputs_state state = {
      .shader = shader,
               if (!sel->info.base.vs.blit_sgprs_amd)
            return nir_shader_intrinsics_pass(nir, lower_vs_input_instr,
            }
