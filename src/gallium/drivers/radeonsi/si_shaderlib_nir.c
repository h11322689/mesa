   /*
   * Copyright 2018 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #define AC_SURFACE_INCLUDE_NIR
   #include "ac_surface.h"
   #include "si_pipe.h"
      #include "nir_format_convert.h"
      static void *create_shader_state(struct si_context *sctx, nir_shader *nir)
   {
               struct pipe_shader_state state = {0};
   state.type = PIPE_SHADER_IR_NIR;
            switch (nir->info.stage) {
   case MESA_SHADER_VERTEX:
         case MESA_SHADER_TESS_CTRL:
         case MESA_SHADER_TESS_EVAL:
         case MESA_SHADER_FRAGMENT:
         case MESA_SHADER_COMPUTE: {
      struct pipe_compute_state cs_state = {0};
   cs_state.ir_type = PIPE_SHADER_IR_NIR;
   cs_state.prog = nir;
      }
   default:
      unreachable("invalid shader stage");
         }
      static nir_def *get_global_ids(nir_builder *b, unsigned num_components)
   {
               nir_def *local_ids = nir_channels(b, nir_load_local_invocation_id(b), mask);
   nir_def *block_ids = nir_channels(b, nir_load_workgroup_id(b), mask);
   nir_def *block_size = nir_channels(b, nir_load_workgroup_size(b), mask);
      }
      static void unpack_2x16(nir_builder *b, nir_def *src, nir_def **x, nir_def **y)
   {
      *x = nir_iand_imm(b, src, 0xffff);
      }
      static void unpack_2x16_signed(nir_builder *b, nir_def *src, nir_def **x, nir_def **y)
   {
      *x = nir_i2i32(b, nir_u2u16(b, src));
      }
      static nir_def *
   deref_ssa(nir_builder *b, nir_variable *var)
   {
         }
      /* Create a NIR compute shader implementing copy_image.
   *
   * This shader can handle 1D and 2D, linear and non-linear images.
   * It expects the source and destination (x,y,z) coords as user_data_amd,
   * packed into 3 SGPRs as 2x16bits per component.
   */
   void *si_create_copy_image_cs(struct si_context *sctx, unsigned wg_dim,
         {
      const nir_shader_compiler_options *options =
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, options, "copy_image_cs");
            /* The workgroup size is either 8x8 for normal (non-linear) 2D images,
   * or 64x1 for 1D and linear-2D images.
   */
            b.shader->info.cs.user_data_components_amd = 3;
            nir_def *coord_src = NULL, *coord_dst = NULL;
   unpack_2x16(&b, nir_trim_vector(&b, nir_load_user_data_amd(&b), 3),
            coord_src = nir_iadd(&b, coord_src, ids);
            /* Coordinates must have 4 channels in NIR. */
   coord_src = nir_pad_vector(&b, coord_src, 4);
                     if (src_is_1d_array)
         if (dst_is_1d_array)
            const struct glsl_type *src_img_type = glsl_image_type(src_is_1d_array ? GLSL_SAMPLER_DIM_1D
               const struct glsl_type *dst_img_type = glsl_image_type(dst_is_1d_array ? GLSL_SAMPLER_DIM_1D
                  nir_variable *img_src = nir_variable_create(b.shader, nir_var_image, src_img_type, "img_src");
            nir_variable *img_dst = nir_variable_create(b.shader, nir_var_image, dst_img_type, "img_dst");
            nir_def *undef32 = nir_undef(&b, 1, 32);
            nir_def *data = nir_image_deref_load(&b, /*num_components*/ 4, /*bit_size*/ 32,
                        }
      void *si_create_dcc_retile_cs(struct si_context *sctx, struct radeon_surf *surf)
   {
      const nir_shader_compiler_options *options =
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, options, "dcc_retile");
   b.shader->info.workgroup_size[0] = 8;
   b.shader->info.workgroup_size[1] = 8;
   b.shader->info.workgroup_size[2] = 1;
   b.shader->info.cs.user_data_components_amd = 3;
            /* Get user data SGPRs. */
            /* Relative offset from the displayable DCC to the non-displayable DCC in the same buffer. */
            nir_def *src_dcc_pitch, *dst_dcc_pitch, *src_dcc_height, *dst_dcc_height;
   unpack_2x16(&b, nir_channel(&b, user_sgprs, 1), &src_dcc_pitch, &src_dcc_height);
            /* Get the 2D coordinates. */
   nir_def *coord = get_global_ids(&b, 2);
            /* Multiply the coordinates by the DCC block size (they are DCC block coordinates). */
   coord = nir_imul(&b, coord, nir_imm_ivec2(&b, surf->u.gfx9.color.dcc_block_width,
            nir_def *src_offset =
      ac_nir_dcc_addr_from_coord(&b, &sctx->screen->info, surf->bpe, &surf->u.gfx9.color.dcc_equation,
                  src_offset = nir_iadd(&b, src_offset, src_dcc_offset);
            nir_def *dst_offset =
      ac_nir_dcc_addr_from_coord(&b, &sctx->screen->info, surf->bpe, &surf->u.gfx9.color.display_dcc_equation,
                              }
      void *gfx9_create_clear_dcc_msaa_cs(struct si_context *sctx, struct si_texture *tex)
   {
      const nir_shader_compiler_options *options =
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, options, "clear_dcc_msaa");
   b.shader->info.workgroup_size[0] = 8;
   b.shader->info.workgroup_size[1] = 8;
   b.shader->info.workgroup_size[2] = 1;
   b.shader->info.cs.user_data_components_amd = 2;
            /* Get user data SGPRs. */
   nir_def *user_sgprs = nir_load_user_data_amd(&b);
   nir_def *dcc_pitch, *dcc_height, *clear_value, *pipe_xor;
   unpack_2x16(&b, nir_channel(&b, user_sgprs, 0), &dcc_pitch, &dcc_height);
   unpack_2x16(&b, nir_channel(&b, user_sgprs, 1), &clear_value, &pipe_xor);
            /* Get the 2D coordinates. */
   nir_def *coord = get_global_ids(&b, 3);
            /* Multiply the coordinates by the DCC block size (they are DCC block coordinates). */
   coord = nir_imul(&b, coord,
                        nir_def *offset =
      ac_nir_dcc_addr_from_coord(&b, &sctx->screen->info, tex->surface.bpe,
                                 /* The trick here is that DCC elements for an even and the next odd sample are next to each other
   * in memory, so we only need to compute the address for sample 0 and the next DCC byte is always
   * sample 1. That's why the clear value has 2 bytes - we're clearing 2 samples at the same time.
   */
               }
      /* Create a compute shader implementing clear_buffer or copy_buffer. */
   void *si_create_clear_buffer_rmw_cs(struct si_context *sctx)
   {
      const nir_shader_compiler_options *options =
            nir_builder b =
         b.shader->info.workgroup_size[0] = 64;
   b.shader->info.workgroup_size[1] = 1;
   b.shader->info.workgroup_size[2] = 1;
   b.shader->info.cs.user_data_components_amd = 2;
            /* address = blockID * 64 + threadID; */
            /* address = address * 16; (byte offset, loading one vec4 per thread) */
   address = nir_ishl_imm(&b, address, 4);
      nir_def *zero = nir_imm_int(&b, 0);
            /* Get user data SGPRs. */
            /* data &= inverted_writemask; */
   data = nir_iand(&b, data, nir_channel(&b, user_sgprs, 1));
   /* data |= clear_value_masked; */
            nir_store_ssbo(&b, data, zero, address,
      .access = SI_COMPUTE_DST_CACHE_POLICY != L2_LRU ? ACCESS_NON_TEMPORAL : 0,
            }
      /* This is used when TCS is NULL in the VS->TCS->TES chain. In this case,
   * VS passes its outputs to TES directly, so the fixed-function shader only
   * has to write TESSOUTER and TESSINNER.
   */
   void *si_create_passthrough_tcs(struct si_context *sctx)
   {
      const nir_shader_compiler_options *options =
      sctx->b.screen->get_compiler_options(sctx->b.screen, PIPE_SHADER_IR_NIR,
                  struct si_shader_info *info = &sctx->shader.vs.cso->info;
   for (unsigned i = 0; i < info->num_outputs; i++) {
                  nir_shader *tcs =
                     }
      static nir_def *convert_linear_to_srgb(nir_builder *b, nir_def *input)
   {
      /* There are small precision differences compared to CB, so the gfx blit will return slightly
   * different results.
            nir_def *comp[4];
   for (unsigned i = 0; i < 3; i++)
                     }
      static nir_def *average_samples(nir_builder *b, nir_def **samples, unsigned num_samples)
   {
      /* This works like add-reduce by computing the sum of each pair independently, and then
   * computing the sum of each pair of sums, and so on, to get better instruction-level
   * parallelism.
   */
   if (num_samples == 16) {
      for (unsigned i = 0; i < 8; i++)
      }
   if (num_samples >= 8) {
      for (unsigned i = 0; i < 4; i++)
      }
   if (num_samples >= 4) {
      for (unsigned i = 0; i < 2; i++)
      }
   if (num_samples >= 2)
               }
      static nir_def *image_resolve_msaa(nir_builder *b, nir_variable *img, unsigned num_samples,
         {
      nir_def *zero = nir_imm_int(b, 0);
   nir_def *result = NULL;
            /* Gfx11 doesn't support samples_identical, so we can't use it. */
   if (gfx_level < GFX11) {
      /* We need a local variable to get the result out of conditional branches in SSA. */
            /* If all samples are identical, load only sample 0. */
   nir_push_if(b, nir_image_deref_samples_identical(b, 1, deref_ssa(b, img), coord));
   result = nir_image_deref_load(b, 4, 32, deref_ssa(b, img), coord, zero, zero);
                        /* We need to hide the constant sample indices behind the optimization barrier, otherwise
   * LLVM doesn't put loads into the same clause.
   *
   * TODO: nir_group_loads could do this.
   */
   nir_def *sample_index[16];
   for (unsigned i = 0; i < num_samples; i++)
            /* Load all samples. */
   nir_def *samples[16];
   for (unsigned i = 0; i < num_samples; i++) {
      samples[i] = nir_image_deref_load(b, 4, 32, deref_ssa(b, img),
                        if (gfx_level < GFX11) {
      /* Exit the conditional branch and get the result out of the branch. */
   nir_store_var(b, var, result, 0xf);
   nir_pop_if(b, NULL);
                  }
      static nir_def *apply_blit_output_modifiers(nir_builder *b, nir_def *color,
         {
      if (options->sint_to_uint)
            if (options->uint_to_sint)
            if (options->dst_is_srgb)
            nir_def *zero = nir_imm_int(b, 0);
            /* Set channels not present in src to 0 or 1. This will eliminate code loading and resolving
   * those channels.
   */
   for (unsigned chan = options->last_src_channel + 1; chan <= options->last_dst_channel; chan++)
            /* Discard channels not present in dst. The hardware fills unstored channels with 0. */
   if (options->last_dst_channel < 3)
            /* Convert to FP16 with rtz to match the pixel shader. Not necessary, but it helps verify
   * the behavior of the whole shader by comparing it to the gfx blit.
   */
   if (options->fp16_rtz)
               }
      /* The compute blit shader.
   *
   * Differences compared to u_blitter (the gfx blit):
   * - u_blitter doesn't preserve NaNs, but the compute blit does
   * - u_blitter has lower linear->SRGB precision because the CB block doesn't
   *   use FP32, but the compute blit does.
   *
   * Other than that, non-scaled blits are identical to u_blitter.
   *
   * Implementation details:
   * - Out-of-bounds dst coordinates are not clamped at all. The hw drops
   *   out-of-bounds stores for us.
   * - Out-of-bounds src coordinates are clamped by emulating CLAMP_TO_EDGE using
   *   the image_size NIR intrinsic.
   * - X/Y flipping just does this in the shader: -threadIDs - 1
   * - MSAA copies are implemented but disabled because MSAA image stores don't
   *   work.
   */
   void *si_create_blit_cs(struct si_context *sctx, const union si_compute_blit_shader_key *options)
   {
      const nir_shader_compiler_options *nir_options =
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, nir_options,
         b.shader->info.num_images = 2;
   if (options->src_is_msaa)
         if (options->dst_is_msaa)
         /* TODO: 1D blits are 8x slower because the workgroup size is 8x8 */
   b.shader->info.workgroup_size[0] = 8;
   b.shader->info.workgroup_size[1] = 8;
   b.shader->info.workgroup_size[2] = 1;
            const struct glsl_type *img_type[2] = {
      glsl_image_type(options->src_is_1d ? GLSL_SAMPLER_DIM_1D :
               glsl_image_type(options->dst_is_1d ? GLSL_SAMPLER_DIM_1D :
                     nir_variable *img_src = nir_variable_create(b.shader, nir_var_uniform, img_type[0], "img0");
            nir_variable *img_dst = nir_variable_create(b.shader, nir_var_uniform, img_type[1], "img1");
                     /* Instructions. */
   /* Let's work with 0-based src and dst coordinates (thread IDs) first. */
   nir_def *dst_xyz = nir_pad_vector_imm_int(&b, get_global_ids(&b, options->wg_dim), 0, 3);
            /* Flip src coordinates. */
   for (unsigned i = 0; i < 2; i++) {
      if (i ? options->flip_y : options->flip_x) {
      /* x goes from 0 to (dim - 1).
   * The flipped blit should load from -dim to -1.
   * Therefore do: x = -x - 1;
   */
   nir_def *comp = nir_channel(&b, src_xyz, i);
   comp = nir_iadd_imm(&b, nir_ineg(&b, comp), -1);
                  /* Add box.xyz. */
   nir_def *coord_src = NULL, *coord_dst = NULL;
   unpack_2x16_signed(&b, nir_trim_vector(&b, nir_load_user_data_amd(&b), 3),
         coord_dst = nir_iadd(&b, coord_dst, dst_xyz);
            /* Clamp to edge for src, only X and Y because Z can't be out of bounds. */
   if (options->xy_clamp_to_edge) {
      unsigned src_clamp_channels = options->src_is_1d ? 0x1 : 0x3;
   nir_def *dim = nir_image_deref_size(&b, 4, 32, deref_ssa(&b, img_src), zero);
            nir_def *coord_src_clamped = nir_channels(&b, coord_src, src_clamp_channels);
   coord_src_clamped = nir_imax(&b, coord_src_clamped, nir_imm_int(&b, 0));
            for (unsigned i = 0; i < util_bitcount(src_clamp_channels); i++)
               /* Swizzle coordinates for 1D_ARRAY. */
            if (options->src_is_1d)
         if (options->dst_is_1d)
            /* Coordinates must have 4 channels in NIR. */
   coord_src = nir_pad_vector(&b, coord_src, 4);
                     /* Execute the image loads and stores. */
   unsigned num_samples = 1 << options->log2_samples;
            if (options->src_is_msaa && !options->dst_is_msaa && !options->sample0_only) {
      /* MSAA resolving (downsampling). */
   assert(num_samples > 1);
   color = image_resolve_msaa(&b, img_src, num_samples, coord_src, sctx->gfx_level);
   color = apply_blit_output_modifiers(&b, color, options);
         } else if (options->src_is_msaa && options->dst_is_msaa) {
      /* MSAA copy. */
   nir_def *color[16];
   assert(num_samples > 1);
   /* Group loads together and then stores. */
   for (unsigned i = 0; i < num_samples; i++) {
      color[i] = nir_image_deref_load(&b, 4, 32, deref_ssa(&b, img_src), coord_src,
      }
   for (unsigned i = 0; i < num_samples; i++)
         for (unsigned i = 0; i < num_samples; i++) {
      nir_image_deref_store(&b, deref_ssa(&b, img_dst), coord_dst,
         } else if (!options->src_is_msaa && options->dst_is_msaa) {
      /* MSAA upsampling. */
   assert(num_samples > 1);
   color = nir_image_deref_load(&b, 4, 32, deref_ssa(&b, img_src), coord_src, zero, zero);
   color = apply_blit_output_modifiers(&b, color, options);
   for (unsigned i = 0; i < num_samples; i++) {
      nir_image_deref_store(&b, deref_ssa(&b, img_dst), coord_dst,
         } else {
      /* Non-MSAA copy or read sample 0 only. */
   /* src2 = sample_index (zero), src3 = lod (zero) */
   assert(num_samples == 1);
   color = nir_image_deref_load(&b, 4, 32, deref_ssa(&b, img_src), coord_src, zero, zero);
   color = apply_blit_output_modifiers(&b, color, options);
                  }
      void *si_clear_render_target_shader(struct si_context *sctx, enum pipe_texture_target type)
   {
      nir_def *address;
            const nir_shader_compiler_options *options =
            nir_builder b =
   nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, options, "clear_render_target");
   b.shader->info.num_ubos = 1;
   b.shader->info.num_images = 1;
            switch (type) {
      case PIPE_TEXTURE_1D_ARRAY:
      b.shader->info.workgroup_size[0] = 64;
   b.shader->info.workgroup_size[1] = 1;
   b.shader->info.workgroup_size[2] = 1;
   sampler_type = GLSL_SAMPLER_DIM_1D;
   address = get_global_ids(&b, 2);
      case PIPE_TEXTURE_2D_ARRAY:
      b.shader->info.workgroup_size[0] = 8;
   b.shader->info.workgroup_size[1] = 8;
   b.shader->info.workgroup_size[2] = 1;
   sampler_type = GLSL_SAMPLER_DIM_2D;
   address = get_global_ids(&b, 3);
      default:
               const struct glsl_type *img_type = glsl_image_type(sampler_type, true, GLSL_TYPE_FLOAT);
   nir_variable *output_img = nir_variable_create(b.shader, nir_var_image, img_type, "image");
            nir_def *zero = nir_imm_int(&b, 0);
            address = nir_iadd(&b, address, ubo);
                     nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img)->def, coord, zero, data, zero,
               }
      void *si_clear_12bytes_buffer_shader(struct si_context *sctx)
   {
      const nir_shader_compiler_options *options =
            nir_builder b =
   nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, options, "clear_12bytes_buffer");
   b.shader->info.workgroup_size[0] = 64;
   b.shader->info.workgroup_size[1] = 1;
   b.shader->info.workgroup_size[2] = 1;
            nir_def *offset = nir_imul_imm(&b, get_global_ids(&b, 1), 12);
            nir_store_ssbo(&b, value, nir_imm_int(&b, 0), offset,
               }
