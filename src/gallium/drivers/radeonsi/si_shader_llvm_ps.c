   /*
   * Copyright 2020 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_pipe.h"
   #include "si_shader_internal.h"
   #include "sid.h"
      static LLVMValueRef si_build_fs_interp(struct si_shader_context *ctx, unsigned attr_index,
               {
      if (i || j) {
      return ac_build_fs_interp(&ctx->ac, LLVMConstInt(ctx->ac.i32, chan, 0),
      }
   return ac_build_fs_interp_mov(&ctx->ac, 0, /* P0 */
            }
      /**
   * Interpolate a fragment shader input.
   *
   * @param ctx                context
   * @param input_index        index of the input in hardware
   * @param semantic_index     semantic index
   * @param num_interp_inputs  number of all interpolated inputs (= BCOLOR offset)
   * @param colors_read_mask   color components read (4 bits for each color, 8 bits in total)
   * @param interp_param       interpolation weights (i,j)
   * @param prim_mask          SI_PARAM_PRIM_MASK
   * @param face               SI_PARAM_FRONT_FACE
   * @param result             the return value (4 components)
   */
   static void interp_fs_color(struct si_shader_context *ctx, unsigned input_index,
                     {
      LLVMValueRef i = NULL, j = NULL;
            /* fs.constant returns the param from the middle vertex, so it's not
   * really useful for flat shading. It's meant to be used for custom
   * interpolation (but the intrinsic can't fetch from the other two
   * vertices).
   *
   * Luckily, it doesn't matter, because we rely on the FLAT_SHADE state
   * to do the right thing. The only reason we use fs.constant is that
   * fs.interp cannot be used on integers, because they can be equal
   * to NaN.
   *
   * When interp is false we will use fs.constant or for newer llvm,
   * amdgcn.interp.mov.
   */
            if (interp) {
      i = LLVMBuildExtractElement(ctx->ac.builder, interp_param, ctx->ac.i32_0, "");
               if (ctx->shader->key.ps.part.prolog.color_two_side) {
               /* If BCOLOR0 is used, BCOLOR1 is at offset "num_inputs + 1",
   * otherwise it's at offset "num_inputs".
   */
   unsigned back_attr_offset = num_interp_inputs;
   if (semantic_index == 1 && colors_read_mask & 0xf)
                     for (chan = 0; chan < 4; chan++) {
                                    } else {
      for (chan = 0; chan < 4; chan++) {
               }
      static void si_alpha_test(struct si_shader_context *ctx, LLVMValueRef alpha)
   {
      if (ctx->shader->key.ps.part.epilog.alpha_func != PIPE_FUNC_NEVER) {
      static LLVMRealPredicate cond_map[PIPE_FUNC_ALWAYS + 1] = {
      [PIPE_FUNC_LESS] = LLVMRealOLT,     [PIPE_FUNC_EQUAL] = LLVMRealOEQ,
   [PIPE_FUNC_LEQUAL] = LLVMRealOLE,   [PIPE_FUNC_GREATER] = LLVMRealOGT,
      };
   LLVMRealPredicate cond = cond_map[ctx->shader->key.ps.part.epilog.alpha_func];
            LLVMValueRef alpha_ref = ac_get_arg(&ctx->ac, ctx->args->alpha_reference);
   if (LLVMTypeOf(alpha) == ctx->ac.f16)
            LLVMValueRef alpha_pass = LLVMBuildFCmp(ctx->ac.builder, cond, alpha, alpha_ref, "");
      } else {
            }
      struct si_ps_exports {
      unsigned num;
      };
      static LLVMValueRef pack_two_16bit(struct ac_llvm_context *ctx, LLVMValueRef args[2])
   {
      LLVMValueRef tmp = ac_build_gather_values(ctx, args, 2);
      }
      static LLVMValueRef get_color_32bit(struct si_shader_context *ctx, unsigned color_type,
         {
      switch (color_type) {
   case SI_TYPE_FLOAT16:
         case SI_TYPE_INT16:
      value = ac_to_integer(&ctx->ac, value);
   value = LLVMBuildSExt(ctx->ac.builder, value, ctx->ac.i32, "");
      case SI_TYPE_UINT16:
      value = ac_to_integer(&ctx->ac, value);
   value = LLVMBuildZExt(ctx->ac.builder, value, ctx->ac.i32, "");
      case SI_TYPE_ANY32:
         }
      }
      /* Initialize arguments for the shader export intrinsic */
   static bool si_llvm_init_ps_export_args(struct si_shader_context *ctx, LLVMValueRef *values,
               {
      const union si_shader_key *key = &ctx->shader->key;
   unsigned col_formats = key->ps.part.epilog.spi_shader_col_format;
   LLVMValueRef f32undef = LLVMGetUndef(ctx->ac.f32);
   unsigned spi_shader_col_format;
   unsigned chan;
                     spi_shader_col_format = (col_formats >> (cbuf * 4)) & 0xf;
   if (spi_shader_col_format == V_028714_SPI_SHADER_ZERO)
            is_int8 = (key->ps.part.epilog.color_is_int8 >> cbuf) & 0x1;
            /* Default is 0xf. Adjusted below depending on the format. */
            /* Specify whether the EXEC mask represents the valid mask */
            /* Specify whether this is the last export */
            /* Specify the target we are exporting */
            if (key->ps.part.epilog.dual_src_blend_swizzle &&
      (compacted_mrt_index == 0 || compacted_mrt_index == 1)) {
   assert(ctx->ac.gfx_level >= GFX11);
               args->compr = false;
   args->out[0] = f32undef;
   args->out[1] = f32undef;
   args->out[2] = f32undef;
            LLVMValueRef (*packf)(struct ac_llvm_context * ctx, LLVMValueRef args[2]) = NULL;
   LLVMValueRef (*packi)(struct ac_llvm_context * ctx, LLVMValueRef args[2], unsigned bits,
            switch (spi_shader_col_format) {
   case V_028714_SPI_SHADER_32_R:
      args->enabled_channels = 1; /* writemask */
   args->out[0] = get_color_32bit(ctx, color_type, values[0]);
         case V_028714_SPI_SHADER_32_GR:
      args->enabled_channels = 0x3; /* writemask */
   args->out[0] = get_color_32bit(ctx, color_type, values[0]);
   args->out[1] = get_color_32bit(ctx, color_type, values[1]);
         case V_028714_SPI_SHADER_32_AR:
      if (ctx->screen->info.gfx_level >= GFX10) {
      args->enabled_channels = 0x3; /* writemask */
   args->out[0] = get_color_32bit(ctx, color_type, values[0]);
      } else {
      args->enabled_channels = 0x9; /* writemask */
   args->out[0] = get_color_32bit(ctx, color_type, values[0]);
      }
         case V_028714_SPI_SHADER_FP16_ABGR:
      if (color_type != SI_TYPE_ANY32)
         else
               case V_028714_SPI_SHADER_UNORM16_ABGR:
      if (color_type != SI_TYPE_ANY32)
         else
               case V_028714_SPI_SHADER_SNORM16_ABGR:
      if (color_type != SI_TYPE_ANY32)
         else
               case V_028714_SPI_SHADER_UINT16_ABGR:
      if (color_type != SI_TYPE_ANY32)
         else
               case V_028714_SPI_SHADER_SINT16_ABGR:
      if (color_type != SI_TYPE_ANY32)
         else
               case V_028714_SPI_SHADER_32_ABGR:
      for (unsigned i = 0; i < 4; i++)
                     /* Pack f16 or norm_i16/u16. */
   if (packf) {
      for (chan = 0; chan < 2; chan++) {
                     packed = packf(&ctx->ac, pack_args);
         }
   /* Pack i16/u16. */
   if (packi) {
      for (chan = 0; chan < 2; chan++) {
      LLVMValueRef pack_args[2] = {ac_to_integer(&ctx->ac, values[2 * chan]),
                  packed = packi(&ctx->ac, pack_args, is_int8 ? 8 : is_int10 ? 10 : 16, chan == 1);
         }
   if (packf || packi) {
      if (ctx->screen->info.gfx_level >= GFX11)
         else
                  }
      static void si_llvm_build_clamp_alpha_test(struct si_shader_context *ctx,
         {
               /* Clamp color */
   if (ctx->shader->key.ps.part.epilog.clamp_color)
      for (i = 0; i < 4; i++)
         /* Alpha to one */
   if (ctx->shader->key.ps.part.epilog.alpha_to_one)
            /* Alpha test */
   if (index == 0 && ctx->shader->key.ps.part.epilog.alpha_func != PIPE_FUNC_ALWAYS)
      }
      static void si_export_mrt_color(struct si_shader_context *ctx, LLVMValueRef *color, unsigned index,
               {
      /* If last_cbuf > 0, FS_COLOR0_WRITES_ALL_CBUFS is true. */
   if (ctx->shader->key.ps.part.epilog.last_cbuf > 0) {
               /* Get the export arguments, also find out what the last one is. */
   for (int c = 0; c <= ctx->shader->key.ps.part.epilog.last_cbuf; c++) {
      if (si_llvm_init_ps_export_args(ctx, color, c, exp->num - first_color_export,
            assert(exp->args[exp->num].enabled_channels);
            } else {
      /* Export */
   if (si_llvm_init_ps_export_args(ctx, color, index, exp->num - first_color_export,
            assert(exp->args[exp->num].enabled_channels);
            }
      /**
   * Return PS outputs in this order:
   *
   * v[0:3] = color0.xyzw
   * v[4:7] = color1.xyzw
   * ...
   * vN+0 = Depth
   * vN+1 = Stencil
   * vN+2 = SampleMask
   * vN+3 = SampleMaskIn (used for OpenGL smoothing)
   *
   * The alpha-ref SGPR is returned via its original location.
   */
   void si_llvm_ps_build_end(struct si_shader_context *ctx)
   {
      struct si_shader *shader = ctx->shader;
   struct si_shader_info *info = &shader->selector->info;
   LLVMBuilderRef builder = ctx->ac.builder;
   unsigned i, j, vgpr;
            LLVMValueRef color[8][4] = {};
   LLVMValueRef depth = NULL, stencil = NULL, samplemask = NULL;
            /* Read the output values. */
   for (i = 0; i < info->num_outputs; i++) {
      unsigned semantic = info->output_semantic[i];
            switch (semantic) {
   case FRAG_RESULT_DEPTH:
      depth = LLVMBuildLoad2(builder, type, addrs[4 * i + 0], "");
      case FRAG_RESULT_STENCIL:
      stencil = LLVMBuildLoad2(builder, type, addrs[4 * i + 0], "");
      case FRAG_RESULT_SAMPLE_MASK:
      samplemask = LLVMBuildLoad2(builder, type, addrs[4 * i + 0], "");
      default:
                        for (j = 0; j < 4; j++) {
      LLVMValueRef ptr = addrs[4 * i + j];
   type = ctx->abi.is_16bit[4 * i + j] ? ctx->ac.f16 : ctx->ac.f32;
   LLVMValueRef result = LLVMBuildLoad2(builder, type, ptr, "");
         } else {
         }
                  /* Fill the return structure. */
            /* Set SGPRs. */
   ret = LLVMBuildInsertValue(
      builder, ret, ac_to_integer(&ctx->ac, LLVMGetParam(ctx->main_fn.value, SI_PARAM_ALPHA_REF)),
         /* Set VGPRs */
   vgpr = SI_SGPR_ALPHA_REF + 1;
   for (i = 0; i < ARRAY_SIZE(color); i++) {
      if (!color[i][0])
            if (LLVMTypeOf(color[i][0]) == ctx->ac.f16) {
      for (j = 0; j < 2; j++) {
      LLVMValueRef tmp = ac_build_gather_values(&ctx->ac, &color[i][j * 2], 2);
   tmp = LLVMBuildBitCast(builder, tmp, ctx->ac.f32, "");
      }
      } else {
      for (j = 0; j < 4; j++)
         }
   if (depth)
         if (stencil)
         if (samplemask)
               }
      static void si_llvm_emit_polygon_stipple(struct si_shader_context *ctx)
   {
      LLVMBuilderRef builder = ctx->ac.builder;
            /* Use the fixed-point gl_FragCoord input.
   * Since the stipple pattern is 32x32 and it repeats, just get 5 bits
   * per coordinate to get the repeating effect.
   */
   address[0] = si_unpack_param(ctx, ctx->args->ac.pos_fixed_pt, 0, 5);
            /* Load the buffer descriptor. */
            /* The stipple pattern is 32x32, each row has 32 bits. */
   offset = LLVMBuildMul(builder, address[1], LLVMConstInt(ctx->ac.i32, 4, 0), "");
   row = si_buffer_load_const(ctx, desc, offset);
   row = ac_to_integer(&ctx->ac, row);
   bit = LLVMBuildLShr(builder, row, address[0], "");
   bit = LLVMBuildTrunc(builder, bit, ctx->ac.i1, "");
      }
      static LLVMValueRef insert_ret_of_arg(struct si_shader_context *ctx, LLVMValueRef ret,
         {
      unsigned base = ctx->args->ac.args[arg_index].file == AC_ARG_VGPR ?
                  if (ctx->args->ac.args[arg_index].size == 1) {
         } else {
      assert(ctx->args->ac.args[arg_index].size == 2);
   LLVMValueRef tmp = LLVMBuildExtractElement(ctx->ac.builder, data, ctx->ac.i32_0, "");
   ret = LLVMBuildInsertValue(ctx->ac.builder, ret, tmp, index, "");
   tmp = LLVMBuildExtractElement(ctx->ac.builder, data, ctx->ac.i32_1, "");
   ret = LLVMBuildInsertValue(ctx->ac.builder, ret, tmp, index + 1, "");
         }
      /**
   * Build the pixel shader prolog function. This handles:
   * - two-side color selection and interpolation
   * - overriding interpolation parameters for the API PS
   * - polygon stippling
   *
   * All preloaded SGPRs and VGPRs are passed through unmodified unless they are
   * overridden by other states. (e.g. per-sample interpolation)
   * Interpolated colors are stored after the preloaded VGPRs.
   */
   void si_llvm_build_ps_prolog(struct si_shader_context *ctx, union si_shader_part_key *key)
   {
      struct si_shader_args *args = ctx->args;
            /* Declare outputs (same as inputs + add colors if needed) */
   LLVMTypeRef return_types[AC_MAX_ARGS];
            for (int i = 0; i < args->ac.num_sgprs_used; i++)
            unsigned num_color_channels = util_bitcount(key->ps_prolog.colors_read);
   unsigned num_output_vgprs = args->ac.num_vgprs_used + num_color_channels;
   for (int i = 0; i < num_output_vgprs; i++)
            /* Create the function. */
   si_llvm_create_func(ctx, "ps_prolog", return_types, num_returns, 0);
            /* Copy inputs to outputs. This should be no-op, as the registers match,
   * but it will prevent the compiler from overwriting them unintentionally.
   */
   LLVMValueRef ret = ctx->return_value;
   for (int i = 0; i < args->ac.arg_count; i++) {
      LLVMValueRef p = LLVMGetParam(func, i);
               /* Polygon stippling. */
   if (key->ps_prolog.states.poly_stipple)
            if (key->ps_prolog.states.bc_optimize_for_persp ||
      key->ps_prolog.states.bc_optimize_for_linear) {
            /* The shader should do: if (PRIM_MASK[31]) CENTROID = CENTER;
   * The hw doesn't compute CENTROID if the whole wave only
   * contains fully-covered quads.
   */
   LLVMValueRef bc_optimize = ac_get_arg(&ctx->ac, args->ac.prim_mask);
   bc_optimize =
                  if (key->ps_prolog.states.bc_optimize_for_persp) {
      center = ac_get_arg(&ctx->ac, args->ac.persp_center);
   centroid = ac_get_arg(&ctx->ac, args->ac.persp_centroid);
   /* Select PERSP_CENTROID. */
   tmp = LLVMBuildSelect(ctx->ac.builder, bc_optimize, center, centroid, "");
      }
   if (key->ps_prolog.states.bc_optimize_for_linear) {
      center = ac_get_arg(&ctx->ac, args->ac.linear_center);
   centroid = ac_get_arg(&ctx->ac, args->ac.linear_centroid);
   /* Select PERSP_CENTROID. */
   tmp = LLVMBuildSelect(ctx->ac.builder, bc_optimize, center, centroid, "");
                  /* Force per-sample interpolation. */
   if (key->ps_prolog.states.force_persp_sample_interp) {
      LLVMValueRef persp_sample = ac_get_arg(&ctx->ac, args->ac.persp_sample);
   /* Overwrite PERSP_CENTER. */
   ret = insert_ret_of_arg(ctx, ret, persp_sample, args->ac.persp_center.arg_index);
   /* Overwrite PERSP_CENTROID. */
      }
   if (key->ps_prolog.states.force_linear_sample_interp) {
      LLVMValueRef linear_sample = ac_get_arg(&ctx->ac, args->ac.linear_sample);
   /* Overwrite LINEAR_CENTER. */
   ret = insert_ret_of_arg(ctx, ret, linear_sample, args->ac.linear_center.arg_index);
   /* Overwrite LINEAR_CENTROID. */
               /* Force center interpolation. */
   if (key->ps_prolog.states.force_persp_center_interp) {
      LLVMValueRef persp_center = ac_get_arg(&ctx->ac, args->ac.persp_center);
   /* Overwrite PERSP_SAMPLE. */
   ret = insert_ret_of_arg(ctx, ret, persp_center, args->ac.persp_sample.arg_index);
   /* Overwrite PERSP_CENTROID. */
      }
   if (key->ps_prolog.states.force_linear_center_interp) {
      LLVMValueRef linear_center = ac_get_arg(&ctx->ac, args->ac.linear_center);
   /* Overwrite LINEAR_SAMPLE. */
   ret = insert_ret_of_arg(ctx, ret, linear_center, args->ac.linear_sample.arg_index);
   /* Overwrite LINEAR_CENTROID. */
               /* Interpolate colors. */
   unsigned color_out_idx = 0;
   unsigned num_input_gprs = args->ac.num_sgprs_used + args->ac.num_vgprs_used;
   for (int i = 0; i < 2; i++) {
               if (!writemask)
            /* If the interpolation qualifier is not CONSTANT (-1). */
   LLVMValueRef interp_ij = NULL;
   if (key->ps_prolog.color_interp_vgpr_index[i] != -1) {
                     /* Get the (i,j) updated by bc_optimize handling. */
   LLVMValueRef interp[2] = {
      LLVMBuildExtractValue(ctx->ac.builder, ret, index, ""),
      };
                        LLVMValueRef face = NULL;
   if (key->ps_prolog.states.color_two_side) {
      face = ac_get_arg(&ctx->ac, args->ac.front_face);
               LLVMValueRef color[4];
   interp_fs_color(ctx, key->ps_prolog.color_attr_index[i], i, key->ps_prolog.num_interp_inputs,
            while (writemask) {
      unsigned chan = u_bit_scan(&writemask);
   ret = LLVMBuildInsertValue(ctx->ac.builder, ret, color[chan],
                  /* Section 15.2.2 (Shader Inputs) of the OpenGL 4.5 (Core Profile) spec
   * says:
   *
   *    "When per-sample shading is active due to the use of a fragment
   *     input qualified by sample or due to the use of the gl_SampleID
   *     or gl_SamplePosition variables, only the bit for the current
   *     sample is set in gl_SampleMaskIn. When state specifies multiple
   *     fragment shader invocations for a given fragment, the sample
   *     mask for any single fragment shader invocation may specify a
   *     subset of the covered samples for the fragment. In this case,
   *     the bit corresponding to each covered sample will be set in
   *     exactly one fragment shader invocation."
   *
   * The samplemask loaded by hardware is always the coverage of the
   * entire pixel/fragment, so mask bits out based on the sample ID.
   */
   if (key->ps_prolog.states.samplemask_log_ps_iter) {
      uint32_t ps_iter_mask =
         LLVMValueRef sampleid = si_unpack_param(ctx, args->ac.ancillary, 8, 4);
            samplemask = ac_to_integer(&ctx->ac, samplemask);
   samplemask =
      LLVMBuildAnd(ctx->ac.builder, samplemask,
                                       /* Tell LLVM to insert WQM instruction sequence when needed. */
   if (key->ps_prolog.wqm) {
                     }
      /**
   * Build the pixel shader epilog function. This handles everything that must be
   * emulated for pixel shader exports. (alpha-test, format conversions, etc)
   */
   void si_llvm_build_ps_epilog(struct si_shader_context *ctx, union si_shader_part_key *key)
   {
      int i;
   struct si_ps_exports exp = {};
            struct si_shader_args *args = ctx->args;
   struct ac_arg color_args[MAX_DRAW_BUFFERS];
   struct ac_arg depth_arg, stencil_arg, samplemask_arg;
            /* Create the function. */
   si_llvm_create_func(ctx, "ps_epilog", NULL, 0, 0);
   /* Disable elimination of unused inputs. */
            /* Prepare color. */
            while (colors_written) {
      int write_i = u_bit_scan(&colors_written);
   unsigned color_type = (key->ps_epilog.color_types >> (write_i * 2)) & 0x3;
            if (color_type != SI_TYPE_ANY32)
            for (i = 0; i < 4; i++)
                        LLVMValueRef mrtz_alpha =
            /* Prepare the mrtz export. */
   if (key->ps_epilog.writes_z ||
      key->ps_epilog.writes_stencil ||
   key->ps_epilog.writes_samplemask ||
   mrtz_alpha) {
            if (key->ps_epilog.writes_z)
         if (key->ps_epilog.writes_stencil)
         if (key->ps_epilog.writes_samplemask)
            ac_export_mrt_z(&ctx->ac, depth, stencil, samplemask, mrtz_alpha, false,
               /* Prepare color exports. */
   const unsigned first_color_export = exp.num;
            while (colors_written) {
      int write_i = u_bit_scan(&colors_written);
                        if (exp.num) {
      exp.args[exp.num - 1].valid_mask = 1;  /* whether the EXEC mask is valid */
            if (key->ps_epilog.states.dual_src_blend_swizzle) {
      assert(ctx->ac.gfx_level >= GFX11);
   assert((key->ps_epilog.colors_written & 0x3) == 0x3);
   ac_build_dual_src_blend_swizzle(&ctx->ac, &exp.args[first_color_export],
               for (unsigned i = 0; i < exp.num; i++)
      } else {
                  /* Compile. */
      }
   