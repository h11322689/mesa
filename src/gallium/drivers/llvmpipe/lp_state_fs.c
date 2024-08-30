   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * @file
   * Code generate the whole fragment pipeline.
   *
   * The fragment pipeline consists of the following stages:
   * - early depth test
   * - fragment shader
   * - alpha test
   * - depth/stencil test
   * - blending
   *
   * This file has only the glue to assemble the fragment pipeline.  The actual
   * plumbing of converting Gallium state into LLVM IR is done elsewhere, in the
   * lp_bld_*.[ch] files, and in a complete generic and reusable way. Here we
   * muster the LLVM JIT execution engine to create a function that follows an
   * established binary interface and that can be called from C directly.
   *
   * A big source of complexity here is that we often want to run different
   * stages with different precisions and data types and precisions. For example,
   * the fragment shader needs typically to be done in floats, but the
   * depth/stencil test and blending is better done in the type that most closely
   * matches the depth/stencil and color buffer respectively.
   *
   * Since the width of a SIMD vector register stays the same regardless of the
   * element type, different types imply different number of elements, so we must
   * code generate more instances of the stages with larger types to be able to
   * feed/consume the stages with smaller types.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
      #include <limits.h>
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_pointer.h"
   #include "util/format/u_format.h"
   #include "util/u_dump.h"
   #include "util/u_string.h"
   #include "util/u_dual_blend.h"
   #include "util/u_upload_mgr.h"
   #include "util/os_time.h"
   #include "pipe/p_shader_tokens.h"
   #include "draw/draw_context.h"
   #include "nir/tgsi_to_nir.h"
   #include "gallivm/lp_bld_type.h"
   #include "gallivm/lp_bld_const.h"
   #include "gallivm/lp_bld_conv.h"
   #include "gallivm/lp_bld_init.h"
   #include "gallivm/lp_bld_intr.h"
   #include "gallivm/lp_bld_logic.h"
   #include "gallivm/lp_bld_tgsi.h"
   #include "gallivm/lp_bld_nir.h"
   #include "gallivm/lp_bld_swizzle.h"
   #include "gallivm/lp_bld_flow.h"
   #include "gallivm/lp_bld_debug.h"
   #include "gallivm/lp_bld_arit.h"
   #include "gallivm/lp_bld_bitarit.h"
   #include "gallivm/lp_bld_pack.h"
   #include "gallivm/lp_bld_format.h"
   #include "gallivm/lp_bld_quad.h"
   #include "gallivm/lp_bld_gather.h"
   #include "gallivm/lp_bld_jit_sample.h"
      #include "lp_bld_alpha.h"
   #include "lp_bld_blend.h"
   #include "lp_bld_depth.h"
   #include "lp_bld_interp.h"
   #include "lp_context.h"
   #include "lp_debug.h"
   #include "lp_perf.h"
   #include "lp_setup.h"
   #include "lp_state.h"
   #include "lp_tex_sample.h"
   #include "lp_flush.h"
   #include "lp_state_fs.h"
   #include "lp_rast.h"
   #include "nir/nir_to_tgsi_info.h"
      #include "lp_screen.h"
   #include "compiler/nir/nir_serialize.h"
   #include "util/mesa-sha1.h"
         /** Fragment shader number (for debugging) */
   static unsigned fs_no = 0;
         static void
   load_unswizzled_block(struct gallivm_state *gallivm,
                        LLVMTypeRef base_type,
   LLVMValueRef base_ptr,
   LLVMValueRef stride,
   unsigned block_width,
   unsigned block_height,
      /**
   * Checks if a format description is an arithmetic format
   *
   * A format which has irregular channel sizes such as R3_G3_B2 or R5_G6_B5.
   */
   static inline bool
   is_arithmetic_format(const struct util_format_description *format_desc)
   {
               for (unsigned i = 0; i < format_desc->nr_channels; ++i) {
      arith |= format_desc->channel[i].size != format_desc->channel[0].size;
                  }
         /**
   * Checks if this format requires special handling due to required expansion
   * to floats for blending, and furthermore has "natural" packed AoS ->
   * unpacked SoA conversion.
   */
   static inline bool
   format_expands_to_float_soa(const struct util_format_description *format_desc)
   {
      if (format_desc->format == PIPE_FORMAT_R11G11B10_FLOAT ||
      format_desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) {
      }
      }
         /**
   * Retrieves the type representing the memory layout for a format
   *
   * e.g. RGBA16F = 4x half-float and R3G3B2 = 1x byte
   */
   static inline void
   lp_mem_type_from_format_desc(const struct util_format_description *format_desc,
         {
      if (format_expands_to_float_soa(format_desc)) {
      /* just make this a uint with width of block */
   type->floating = false;
   type->fixed = false;
   type->sign = false;
   type->norm = false;
   type->width = format_desc->block.bits;
   type->length = 1;
                        memset(type, 0, sizeof(struct lp_type));
   type->floating = format_desc->channel[chan].type == UTIL_FORMAT_TYPE_FLOAT;
   type->fixed    = format_desc->channel[chan].type == UTIL_FORMAT_TYPE_FIXED;
   type->sign     = format_desc->channel[chan].type != UTIL_FORMAT_TYPE_UNSIGNED;
            if (is_arithmetic_format(format_desc)) {
      type->width = 0;
            for (unsigned i = 0; i < format_desc->nr_channels; ++i) {
            } else {
      type->width = format_desc->channel[chan].size;
         }
         /**
   * Expand the relevant bits of mask_input to a n*4-dword mask for the
   * n*four pixels in n 2x2 quads.  This will set the n*four elements of the
   * quad mask vector to 0 or ~0.
   * Grouping is 01, 23 for 2 quad mode hence only 0 and 2 are valid
   * quad arguments with fs length 8.
   *
   * \param first_quad  which quad(s) of the quad group to test, in [0,3]
   * \param mask_input  bitwise mask for the whole 4x4 stamp
   */
   static LLVMValueRef
   generate_quad_mask(struct gallivm_state *gallivm,
                     struct lp_type fs_type,
   {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMValueRef bits[16];
            /*
   * XXX: We'll need a different path for 16 x u8
   */
   assert(fs_type.width == 32);
   assert(fs_type.length <= ARRAY_SIZE(bits));
            /*
   * mask_input >>= (quad * 4)
   */
   int shift;
   switch (first_quad) {
   case 0:
      shift = 0;
      case 1:
      assert(fs_type.length == 4);
   shift = 2;
      case 2:
      shift = 8;
      case 3:
      assert(fs_type.length == 4);
   shift = 10;
      default:
      assert(0);
               mask_input = LLVMBuildLShr(builder, mask_input,
         mask_input = LLVMBuildTrunc(builder, mask_input, i32t, "");
   mask_input = LLVMBuildAnd(builder, mask_input,
         mask_input = LLVMBuildLShr(builder, mask_input,
            /*
   * mask = { mask_input & (1 << i), for i in [0,3] }
   */
   mask = lp_build_broadcast(gallivm,
                  for (int i = 0; i < fs_type.length / 4; i++) {
      unsigned j = 2 * (i % 2) + (i / 2) * 8;
   bits[4*i + 0] = LLVMConstInt(i32t, 1ULL << (j + 0), 0);
   bits[4*i + 1] = LLVMConstInt(i32t, 1ULL << (j + 1), 0);
   bits[4*i + 2] = LLVMConstInt(i32t, 1ULL << (j + 4), 0);
      }
   bits_vec = LLVMConstVector(bits, fs_type.length);
            /*
   * mask = mask == bits ? ~0 : 0
   */
   mask = lp_build_compare(gallivm,
                     }
         #define EARLY_DEPTH_TEST  0x1
   #define LATE_DEPTH_TEST   0x2
   #define EARLY_DEPTH_WRITE 0x4
   #define LATE_DEPTH_WRITE  0x8
   #define EARLY_DEPTH_TEST_INFERRED  0x10 //only with EARLY_DEPTH_TEST
      static unsigned
   get_cbuf_location(nir_variable *var, unsigned slot)
   {
         }
      static int
   find_output_by_frag_result(struct nir_shader *shader,
         {
      nir_foreach_shader_out_variable(var, shader) {
      int slots = nir_variable_count_slots(var, var->type);
   for (unsigned s = 0; s < slots; s++) {
      if (var->data.location + var->data.index + s == frag_result)
                     }
      /**
   * Fetch the specified lp_jit_viewport structure for a given viewport_index.
   */
   static LLVMValueRef
   lp_llvm_viewport(LLVMTypeRef context_type,
                     {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef ptr;
   LLVMValueRef res;
   struct lp_type viewport_type =
                  ptr = lp_jit_context_viewports(gallivm, context_type, context_ptr);
   ptr = LLVMBuildPointerCast(builder, ptr,
                        }
         static LLVMValueRef
   lp_build_depth_clamp(struct gallivm_state *gallivm,
                        LLVMBuilderRef builder,
   bool depth_clamp,
   bool restrict_depth,
   struct lp_type type,
   LLVMTypeRef context_type,
      {
      LLVMValueRef viewport, min_depth, max_depth;
   LLVMValueRef viewport_index;
            assert(type.floating);
            if (restrict_depth)
            if (!depth_clamp)
            /*
   * Assumes clamping of the viewport index will occur in setup/gs. Value
   * is passed through the rasterization stage via lp_rast_shader_inputs.
   *
   * See: draw_clamp_viewport_idx and lp_clamp_viewport_idx for clamping
   *      semantics.
   */
   viewport_index = lp_jit_thread_data_raster_state_viewport_index(gallivm,
                  /*
   * Load the min and max depth from the lp_jit_context.viewports
   * array of lp_jit_viewport structures.
   */
            /* viewports[viewport_index].min_depth */
   min_depth = LLVMBuildExtractElement(builder, viewport,
                  /* viewports[viewport_index].max_depth */
   max_depth = LLVMBuildExtractElement(builder, viewport,
                  /*
   * Clamp to the min and max depth values for the given viewport.
   */
      }
         static void
   lp_build_sample_alpha_to_coverage(struct gallivm_state *gallivm,
                                    struct lp_type type,
      {
      struct lp_build_context bld;
   LLVMBuilderRef builder = gallivm->builder;
            lp_build_context_init(&bld, gallivm, type);
   for (unsigned s = 0; s < coverage_samples; s++) {
      LLVMValueRef alpha_ref_value = lp_build_const_vec(gallivm, type, step * s);
            LLVMValueRef s_mask_idx = LLVMBuildMul(builder, lp_build_const_int32(gallivm, s), num_loop, "");
   s_mask_idx = LLVMBuildAdd(builder, s_mask_idx, loop_counter, "");
   LLVMValueRef s_mask_ptr = LLVMBuildGEP2(builder, coverage_mask_type,
         LLVMValueRef s_mask = LLVMBuildLoad2(builder, coverage_mask_type, s_mask_ptr, "");
   s_mask = LLVMBuildAnd(builder, s_mask, test, "");
         };
         struct lp_build_fs_llvm_iface {
      struct lp_build_fs_iface base;
   struct lp_build_interp_soa_context *interp;
   struct lp_build_for_loop_state *loop_state;
   LLVMTypeRef mask_type;
   LLVMValueRef mask_store;
   LLVMValueRef sample_id;
   LLVMValueRef color_ptr_ptr;
   LLVMValueRef color_stride_ptr;
   LLVMValueRef color_sample_stride_ptr;
   LLVMValueRef zs_base_ptr;
   LLVMValueRef zs_stride;
   LLVMValueRef zs_sample_stride;
      };
         static LLVMValueRef
   fs_interp(const struct lp_build_fs_iface *iface,
            struct lp_build_context *bld,
   unsigned attrib, unsigned chan,
   bool centroid, bool sample,
      {
      struct lp_build_fs_llvm_iface *fs_iface = (struct lp_build_fs_llvm_iface *)iface;
   struct lp_build_interp_soa_context *interp = fs_iface->interp;
   unsigned loc = TGSI_INTERPOLATE_LOC_CENTER;
   if (centroid)
         if (sample)
            return lp_build_interp_soa(interp, bld->gallivm, fs_iface->loop_state->counter,
            }
         /**
   * Convert depth-stencil format to a single component one, returning
   * PIPE_FORMAT_NONE if it doesn't contain the required component.
   */
   static enum pipe_format
   select_zs_component_format(enum pipe_format format,
         {
      const struct util_format_description* desc = util_format_description(format);
   if (fetch_stencil && !util_format_has_stencil(desc))
         if (!fetch_stencil && !util_format_has_depth(desc))
            switch (format) {
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
         case PIPE_FORMAT_S8_UINT_Z24_UNORM:
         case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
         default:
            }
      static void
   fs_fb_fetch(const struct lp_build_fs_iface *iface,
               struct lp_build_context *bld,
   {
      struct lp_build_fs_llvm_iface *fs_iface = (struct lp_build_fs_llvm_iface *)iface;
   struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef int32_type = LLVMInt32TypeInContext(gallivm->context);
   LLVMTypeRef int8_type = LLVMInt8TypeInContext(gallivm->context);
   LLVMTypeRef int8p_type = LLVMPointerType(int8_type, 0);
            LLVMValueRef buf_ptr;
   LLVMValueRef stride;
            const bool fetch_stencil = location == FRAG_RESULT_STENCIL;
   const bool fetch_zs = fetch_stencil || location == FRAG_RESULT_DEPTH;
   if (fetch_zs) {
      buf_ptr = fs_iface->zs_base_ptr;
   stride = fs_iface->zs_stride;
      } else {
      assert(location >= FRAG_RESULT_DATA0 && location <= FRAG_RESULT_DATA7);
   const int cbuf = location - FRAG_RESULT_DATA0;
            buf_ptr = LLVMBuildLoad2(builder, int8p_type,
               stride = LLVMBuildLoad2(builder, int32_type,
                           const struct util_format_description* out_format_desc = util_format_description(buf_format);
   if (out_format_desc->format == PIPE_FORMAT_NONE) {
      result[0] = result[1] = result[2] = result[3] = bld->undef;
               unsigned block_size = bld->type.length;
   unsigned block_height = key->resource_1d ? 1 : 2;
            if (key->multisample) {
               if (fetch_zs) {
         } else {
      LLVMValueRef index = lp_build_const_int32(gallivm, location - FRAG_RESULT_DATA0);
   sample_stride = LLVMBuildLoad2(builder, int32_type,
                                 LLVMValueRef sample_offset = LLVMBuildMul(builder, sample_stride, fs_iface->sample_id, "");
   buf_ptr = LLVMBuildGEP2(builder, int8_type,
               /* fragment shader executes on 4x4 blocks. depending on vector width it can
   * execute 2 or 4 iterations.  only move to the next row once the top row
   * has completed 8 wide 1 iteration, 4 wide 2 iterations */
   LLVMValueRef x_offset = NULL, y_offset = NULL;
   if (!key->resource_1d) {
               if (block_size == 4) {
      x_offset = LLVMBuildShl(builder,
                  }
               LLVMValueRef offsets[4 * 4];
   for (unsigned i = 0; i < block_size; i++) {
      unsigned x = i % block_width;
            if (block_size == 8) {
      /* remap the raw slots into the fragment shader execution mode. */
   /* this math took me way too long to work out, I'm sure it's
   * overkill.
   */
   x = (i & 1) + ((i >> 2) << 1);
   if (!key->resource_1d)
               LLVMValueRef x_val;
   if (x_offset) {
      x_val = LLVMBuildAdd(builder, lp_build_const_int32(gallivm, x), x_offset, "");
      } else {
                  LLVMValueRef y_val = lp_build_const_int32(gallivm, y);
   if (y_offset)
                     }
            struct lp_type texel_type = bld->type;
   if (out_format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB &&
      out_format_desc->channel[0].pure_integer) {
   if (out_format_desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED) {
         } else if (out_format_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED) {
            } else if (fetch_stencil) {
                  lp_build_fetch_rgba_soa(gallivm, out_format_desc, texel_type,
            }
      /**
   * Generate the fragment shader, depth/stencil test, and alpha tests.
   */
   static void
   generate_fs_loop(struct gallivm_state *gallivm,
                  struct lp_fragment_shader *shader,
   const struct lp_fragment_shader_variant_key *key,
   LLVMBuilderRef builder,
   struct lp_type type,
   LLVMTypeRef context_type,
   LLVMValueRef context_ptr,
   LLVMTypeRef resources_type,
   LLVMValueRef resources_ptr,
   LLVMTypeRef sample_pos_type,
   LLVMValueRef sample_pos_array,
   LLVMValueRef num_loop,
   struct lp_build_interp_soa_context *interp,
   const struct lp_build_sampler_soa *sampler,
   const struct lp_build_image_soa *image,
   LLVMTypeRef mask_type,
   LLVMValueRef mask_store,
   LLVMValueRef (*out_color)[4],
   LLVMValueRef depth_base_ptr,
   LLVMValueRef depth_stride,
   LLVMValueRef depth_sample_stride,
   LLVMValueRef color_ptr_ptr,
   LLVMValueRef color_stride_ptr,
   LLVMValueRef color_sample_stride_ptr,
      {
      struct lp_type int_type = lp_int_type(type);
   LLVMValueRef mask_ptr = NULL, mask_val = NULL;
   LLVMValueRef z;
   LLVMValueRef z_value, s_value;
   LLVMValueRef z_fb, s_fb;
   LLVMValueRef zs_samples = lp_build_const_int32(gallivm, key->zsbuf_nr_samples);
   LLVMValueRef z_out = NULL, s_out = NULL;
   struct lp_build_for_loop_state loop_state, sample_loop_state = {0};
   struct lp_build_mask_context mask;
   struct nir_shader *nir = shader->base.ir.nir;
   const bool dual_source_blend = key->blend.rt[0].blend_enable &&
                                    /* truncate then sign extend. */
   system_values.front_facing =
      LLVMBuildTrunc(gallivm->builder, facing,
      system_values.front_facing =
      LLVMBuildSExt(gallivm->builder, system_values.front_facing,
      system_values.view_index =
      lp_jit_thread_data_raster_state_view_index(gallivm,
               unsigned depth_mode;
   const struct util_format_description *zs_format_desc = NULL;
   if (key->depth.enabled ||
      key->stencil[0].enabled) {
            if (nir->info.fs.early_fragment_tests || nir->info.fs.post_depth_coverage) {
         } else if (!(nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) &&
            !(nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_STENCIL)) &&
   if (key->alpha.enabled ||
      key->blend.alpha_to_coverage ||
   nir->info.fs.uses_discard ||
   nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK)) {
   /* With alpha test and kill, can do the depth test early
   * and hopefully eliminate some quads.  But need to do a
   * special deferred depth write once the final mask value
   * is known. This only works though if there's either no
   * stencil test or the stencil value isn't written.
   */
   if (key->stencil[0].enabled && (key->stencil[0].writemask ||
                     else
      } else {
            } else {
                  if (!(key->depth.enabled && key->depth.writemask) &&
      !(key->stencil[0].enabled && (key->stencil[0].writemask ||
               } else {
                  LLVMTypeRef vec_type = lp_build_vec_type(gallivm, type);
            LLVMValueRef stencil_refs[2];
   stencil_refs[0] = lp_jit_context_stencil_ref_front_value(gallivm, context_type, context_ptr);
   stencil_refs[1] = lp_jit_context_stencil_ref_back_value(gallivm, context_type, context_ptr);
   /* convert scalar stencil refs into vectors */
   stencil_refs[0] = lp_build_broadcast(gallivm, int_vec_type, stencil_refs[0]);
                              LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][TGSI_NUM_CHANNELS];
            /* Allocate color storage for each fragment sample */
   LLVMValueRef color_store_size = num_loop;
   if (key->min_samples > 1)
            for (unsigned cbuf = 0; cbuf < key->nr_cbufs; cbuf++) {
      for (unsigned chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
      out_color[cbuf][chan] = lp_build_array_alloca(gallivm,
                     }
   if (dual_source_blend) {
      assert(key->nr_cbufs <= 1);
   for (unsigned chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
      out_color[1][chan] = lp_build_array_alloca(gallivm,
                     }
   if (nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
      z_out = lp_build_array_alloca(gallivm,
                     if (nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_STENCIL)) {
      s_out = lp_build_array_alloca(gallivm,
                     lp_build_for_loop_begin(&loop_state, gallivm,
                              LLVMValueRef sample_mask_in;
   if (key->multisample) {
      sample_mask_in = lp_build_const_int_vec(gallivm, type, 0);
   /* create shader execution mask by combining all sample masks. */
   for (unsigned s = 0; s < key->coverage_samples; s++) {
      LLVMValueRef s_mask_idx = LLVMBuildMul(builder, num_loop, lp_build_const_int32(gallivm, s), "");
   s_mask_idx = LLVMBuildAdd(builder, s_mask_idx, loop_state.counter, "");
   LLVMValueRef s_mask = lp_build_pointer_get2(builder, mask_type, mask_store, s_mask_idx);
   if (s == 0)
                        LLVMValueRef mask_in = LLVMBuildAnd(builder, s_mask, lp_build_const_int_vec(gallivm, type, (1ll << s)), "");
         } else {
      sample_mask_in = lp_build_const_int_vec(gallivm, type, 1);
   mask_ptr = LLVMBuildGEP2(builder, mask_type, mask_store,
                  LLVMValueRef mask_in = LLVMBuildAnd(builder, mask_val, lp_build_const_int_vec(gallivm, type, 1), "");
               /* 'mask' will control execution based on quad's pixel alive/killed state */
            if (!(depth_mode & EARLY_DEPTH_TEST))
            /* Create storage for recombining sample masks after early Z pass. */
   LLVMValueRef s_mask_or = lp_build_alloca(gallivm, int_vec_type, "cov_mask_early_depth");
            /* Create storage for post depth sample mask */
   LLVMValueRef post_depth_sample_mask_in = NULL;
   if (post_depth_coverage)
            LLVMValueRef s_mask = NULL, s_mask_ptr = NULL;
   LLVMValueRef z_sample_value_store = NULL, s_sample_value_store = NULL;
   LLVMValueRef z_fb_store = NULL, s_fb_store = NULL;
            /* Run early depth once per sample */
               if (zs_format_desc) {
      struct lp_type zs_type = lp_depth_type(zs_format_desc, type.length);
   struct lp_type z_type = zs_type;
   struct lp_type s_type = zs_type;
   if (zs_format_desc->block.bits < type.width)
         if (zs_format_desc->block.bits == 8) {
         } else if (zs_format_desc->block.bits > 32) {
      z_type.width = z_type.width / 2;
   s_type.width = s_type.width / 2;
      }
   z_sample_value_store = lp_build_array_alloca(gallivm, lp_build_int_vec_type(gallivm, type),
         s_sample_value_store = lp_build_array_alloca(gallivm, lp_build_int_vec_type(gallivm, type),
         z_fb_store = lp_build_array_alloca(gallivm, lp_build_vec_type(gallivm, z_type),
         s_fb_store = lp_build_array_alloca(gallivm, lp_build_vec_type(gallivm, s_type),
      }
   lp_build_for_loop_begin(&sample_loop_state, gallivm,
                        LLVMValueRef s_mask_idx = LLVMBuildMul(builder, sample_loop_state.counter, num_loop, "");
   s_mask_idx = LLVMBuildAdd(builder, s_mask_idx, loop_state.counter, "");
            s_mask = LLVMBuildLoad2(builder, mask_type, s_mask_ptr, "");
                  /* for multisample Z needs to be interpolated at sample points for testing. */
   lp_build_interp_soa_update_pos_dyn(interp, gallivm, loop_state.counter,
                        LLVMValueRef depth_ptr = depth_base_ptr;
   if (key->multisample) {
      LLVMValueRef sample_offset =
      LLVMBuildMul(builder, sample_loop_state.counter,
      depth_ptr = LLVMBuildGEP2(builder, LLVMInt8TypeInContext(gallivm->context),
               if (depth_mode & EARLY_DEPTH_TEST) {
      z = lp_build_depth_clamp(gallivm, builder, key->depth_clamp,
                        lp_build_depth_stencil_load_swizzled(gallivm, type,
                     lp_build_depth_stencil_test(gallivm,
                              &key->depth,
   key->stencil,
   type,
   zs_format_desc,
   key->multisample ? NULL : &mask,
   &s_mask,
               if (depth_mode & EARLY_DEPTH_WRITE) {
      lp_build_depth_stencil_write_swizzled(gallivm, type,
                        }
   /*
   * Note mask check if stencil is enabled must be after ds write not
   * after stencil test otherwise new stencil values may not get written
   * if all fragments got killed by depth/stencil test.
   */
   if (key->stencil[0].enabled && !key->multisample)
            if (key->multisample) {
      z_fb_type = LLVMTypeOf(z_fb);
   z_type = LLVMTypeOf(z_value);
   lp_build_pointer_set(builder, z_sample_value_store, sample_loop_state.counter, LLVMBuildBitCast(builder, z_value, lp_build_int_vec_type(gallivm, type), ""));
   lp_build_pointer_set(builder, s_sample_value_store, sample_loop_state.counter, LLVMBuildBitCast(builder, s_value, lp_build_int_vec_type(gallivm, type), ""));
   lp_build_pointer_set(builder, z_fb_store, sample_loop_state.counter, z_fb);
      }
   if (key->occlusion_count && !(depth_mode & EARLY_DEPTH_TEST_INFERRED)) {
      LLVMValueRef counter = lp_jit_thread_data_vis_counter(gallivm, thread_data_type, thread_data_ptr);
   lp_build_name(counter, "counter");
   lp_build_occlusion_count(gallivm, type,
                  if (key->multisample) {
      /*
   * Store the post-early Z coverage mask.
   * Recombine the resulting coverage masks post early Z into the fragment
   * shader execution mask.
   */
   LLVMValueRef tmp_s_mask_or = LLVMBuildLoad2(builder, int_vec_type, s_mask_or, "");
   tmp_s_mask_or = LLVMBuildOr(builder, tmp_s_mask_or, s_mask, "");
            if (post_depth_coverage) {
      LLVMValueRef mask_bit_idx = LLVMBuildShl(builder, lp_build_const_int32(gallivm, 1), sample_loop_state.counter, "");
   LLVMValueRef post_depth_mask_in = LLVMBuildLoad2(builder, int_vec_type, post_depth_sample_mask_in, "");
   mask_bit_idx = LLVMBuildAnd(builder, s_mask, lp_build_broadcast(gallivm, int_vec_type, mask_bit_idx), "");
   post_depth_mask_in = LLVMBuildOr(builder, post_depth_mask_in, mask_bit_idx, "");
                                 /* recombined all the coverage masks in the shader exec mask. */
   tmp_s_mask_or = LLVMBuildLoad2(builder, int_vec_type, s_mask_or, "");
            if (key->min_samples == 1) {
      /* for multisample Z needs to be re interpolated at pixel center */
   lp_build_interp_soa_update_pos_dyn(interp, gallivm, loop_state.counter, NULL);
   z = interp->pos[2];
         } else {
      if (post_depth_coverage) {
      LLVMValueRef post_depth_mask_in = LLVMBuildAnd(builder, lp_build_mask_value(&mask), lp_build_const_int_vec(gallivm, type, 1), "");
                  LLVMValueRef out_sample_mask_storage = NULL;
   if (nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK)) {
      out_sample_mask_storage = lp_build_alloca(gallivm, int_vec_type, "write_mask");
   if (key->min_samples > 1)
               if (post_depth_coverage) {
         } else {
         }
   if (key->multisample && key->min_samples > 1) {
      lp_build_for_loop_begin(&sample_loop_state, gallivm,
                              LLVMValueRef s_mask_idx = LLVMBuildMul(builder, sample_loop_state.counter, num_loop, "");
   s_mask_idx = LLVMBuildAdd(builder, s_mask_idx, loop_state.counter, "");
   s_mask_ptr = LLVMBuildGEP2(builder, mask_type, mask_store, &s_mask_idx, 1, "");
   s_mask = LLVMBuildLoad2(builder, mask_type, s_mask_ptr, "");
   lp_build_mask_force(&mask, s_mask);
   lp_build_interp_soa_update_pos_dyn(interp, gallivm, loop_state.counter, sample_loop_state.counter);
   system_values.sample_id = sample_loop_state.counter;
   system_values.sample_mask_in = LLVMBuildAnd(builder, system_values.sample_mask_in,
            } else {
            }
   system_values.sample_pos = sample_pos_array;
            lp_build_interp_soa_update_inputs_dyn(interp, gallivm, loop_state.counter,
            struct lp_build_fs_llvm_iface fs_iface = {
   .base.interp_fn = fs_interp,
   .base.fb_fetch = fs_fb_fetch,
   .interp = interp,
   .loop_state = &loop_state,
   .sample_id = system_values.sample_id,
   .mask_type = mask_type,
   .mask_store = mask_store,
   .color_ptr_ptr = color_ptr_ptr,
   .color_stride_ptr = color_stride_ptr,
   .color_sample_stride_ptr = color_sample_stride_ptr,
   .zs_base_ptr = depth_base_ptr,
   .zs_stride = depth_stride,
   .zs_sample_stride = depth_sample_stride,
   .key = key,
            struct lp_build_tgsi_params params;
            params.type = type;
   params.mask = &mask;
   params.fs_iface = &fs_iface.base;
   params.consts_ptr = consts_ptr;
   params.system_values = &system_values;
   params.inputs = interp->inputs;
   params.context_type = context_type;
   params.context_ptr = context_ptr;
   params.resources_type = resources_type;
   params.resources_ptr = resources_ptr;
   params.thread_data_type = thread_data_type;
   params.thread_data_ptr = thread_data_ptr;
   params.sampler = sampler;
   params.info = &shader->info.base;
   params.ssbo_ptr = ssbo_ptr;
   params.image = image;
            /* Build the actual shader */
            /* Alpha test */
   if (key->alpha.enabled) {
               if (color0 != -1 && outputs[color0][3]) {
      const struct util_format_description *cbuf_format_desc;
                                          lp_build_alpha_test(gallivm, key->alpha.func, type, cbuf_format_desc,
                        /* Emulate Alpha to Coverage with Alpha test */
   if (key->blend.alpha_to_coverage) {
               if (color0 != -1 && outputs[color0][3]) {
               if (!key->multisample) {
      lp_build_alpha_to_coverage(gallivm, type,
            } else {
      lp_build_sample_alpha_to_coverage(gallivm, type, key->coverage_samples, num_loop,
                           if (key->blend.alpha_to_one) {
      nir_foreach_shader_out_variable(var, nir) {
      if (var->data.location < FRAG_RESULT_DATA0)
         int slots = nir_variable_count_slots(var, var->type);
   for (unsigned s = 0; s < slots; s++) {
      unsigned cbuf = get_cbuf_location(var, s);
   if ((cbuf < key->nr_cbufs) || (cbuf == 1 && dual_source_blend))
      if (outputs[cbuf][3]) {
      LLVMBuildStore(builder, lp_build_const_vec(gallivm, type, 1.0),
                     if (nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK)) {
      LLVMValueRef output_smask = NULL;
            struct lp_build_context smask_bld;
            assert(smaski >= 0);
   output_smask = LLVMBuildLoad2(builder, vec_type, outputs[smaski][0], "smask");
   output_smask = LLVMBuildBitCast(builder, output_smask, smask_bld.vec_type, "");
   if (!key->multisample && key->no_ms_sample_mask_out) {
      output_smask = lp_build_and(&smask_bld, output_smask, smask_bld.one);
   output_smask = lp_build_cmp(&smask_bld, PIPE_FUNC_NOTEQUAL, output_smask, smask_bld.zero);
               if (key->min_samples > 1) {
      /* only the bit corresponding to this sample is to be used. */
   LLVMValueRef tmp_mask = LLVMBuildLoad2(builder, int_vec_type, out_sample_mask_storage, "tmp_mask");
   LLVMValueRef out_smask_idx = LLVMBuildShl(builder, lp_build_const_int32(gallivm, 1), sample_loop_state.counter, "");
   LLVMValueRef smask_bit = LLVMBuildAnd(builder, output_smask, lp_build_broadcast(gallivm, int_vec_type, out_smask_idx), "");
                           if (nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
               LLVMValueRef out = LLVMBuildLoad2(builder, vec_type, outputs[pos0][2], "");
   LLVMValueRef idx = loop_state.counter;
   if (key->min_samples > 1)
      idx = LLVMBuildAdd(builder, idx,
      LLVMValueRef ptr = LLVMBuildGEP2(builder, vec_type, z_out, &idx, 1, "");
               if (nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_STENCIL)) {
               LLVMValueRef out = LLVMBuildLoad2(builder, vec_type,
         LLVMValueRef idx = loop_state.counter;
   if (key->min_samples > 1)
      idx = LLVMBuildAdd(builder, idx,
      LLVMValueRef ptr = LLVMBuildGEP2(builder, vec_type, s_out, &idx, 1, "");
               bool has_cbuf0_write = false;
   /* Color write - per fragment sample */
   nir_foreach_shader_out_variable(var, nir) {
      if (var->data.location < FRAG_RESULT_DATA0)
                  for (unsigned s = 0; s < slots; s++) {
      unsigned cbuf = get_cbuf_location(var, s);
   unsigned attrib = var->data.driver_location + s;
   if ((cbuf < key->nr_cbufs) || (cbuf == 1 && dual_source_blend)) {
      if (cbuf == 0) {
      /* XXX: there is an edge case with FB fetch where gl_FragColor and
   * gl_LastFragData[0] are used together. This creates both
   * FRAG_RESULT_COLOR and FRAG_RESULT_DATA* output variables. This
   * loop then writes to cbuf 0 twice, owerwriting the correct value
   * from gl_FragColor with some garbage. This case is excercised in
   * one of deqp tests.  A similar bug can happen if
   * gl_SecondaryFragColorEXT and gl_LastFragData[1] are mixed in
   * the same fashion...  This workaround will break if
   * gl_LastFragData[0] goes in outputs list before
   * gl_FragColor. This doesn't seem to happen though.
   */
   if (has_cbuf0_write)
                     for (unsigned chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
      if (outputs[attrib][chan]) {
      /* XXX: just initialize outputs to point at colors[] and
   * skip this.
   */
   LLVMValueRef out = LLVMBuildLoad2(builder, vec_type, outputs[attrib][chan], "");
   LLVMValueRef color_ptr;
   LLVMValueRef color_idx = loop_state.counter;
   if (key->min_samples > 1)
      color_idx = LLVMBuildAdd(builder, color_idx,
      color_ptr = LLVMBuildGEP2(builder, vec_type, out_color[cbuf][chan],
         lp_build_name(out, "color%u.%c", attrib, "rgba"[chan]);
                           if (key->multisample && key->min_samples > 1) {
      LLVMBuildStore(builder, lp_build_mask_value(&mask), s_mask_ptr);
               if (key->multisample) {
      /* execute depth test for each sample */
   lp_build_for_loop_begin(&sample_loop_state, gallivm,
                        /* load the per-sample coverage mask */
   LLVMValueRef s_mask_idx = LLVMBuildMul(builder, sample_loop_state.counter, num_loop, "");
   s_mask_idx = LLVMBuildAdd(builder, s_mask_idx, loop_state.counter, "");
            /* combine the execution mask post fragment shader with the coverage mask. */
   s_mask = LLVMBuildLoad2(builder, mask_type, s_mask_ptr, "");
   if (key->min_samples == 1)
            /* if the shader writes sample mask use that,
   * but only if this isn't genuine early-depth to avoid breaking occlusion query */
   if (nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK) &&
      (!(depth_mode & EARLY_DEPTH_TEST) || (depth_mode & (EARLY_DEPTH_TEST_INFERRED)))) {
   LLVMValueRef out_smask_idx = LLVMBuildShl(builder, lp_build_const_int32(gallivm, 1), sample_loop_state.counter, "");
   out_smask_idx = lp_build_broadcast(gallivm, int_vec_type, out_smask_idx);
   LLVMValueRef output_smask = LLVMBuildLoad2(builder, int_vec_type, out_sample_mask_storage, "");
   LLVMValueRef smask_bit = LLVMBuildAnd(builder, output_smask, out_smask_idx, "");
                                 depth_ptr = depth_base_ptr;
   if (key->multisample) {
      LLVMValueRef sample_offset = LLVMBuildMul(builder, sample_loop_state.counter, depth_sample_stride, "");
   depth_ptr = LLVMBuildGEP2(builder, LLVMInt8TypeInContext(gallivm->context),
               /* Late Z test */
   if (depth_mode & LATE_DEPTH_TEST) {
      if (nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
      LLVMValueRef idx = loop_state.counter;
   if (key->min_samples > 1)
      idx = LLVMBuildAdd(builder, idx,
      LLVMValueRef ptr = LLVMBuildGEP2(builder, vec_type, z_out, &idx, 1, "");
      } else {
      if (key->multisample) {
      lp_build_interp_soa_update_pos_dyn(interp, gallivm, loop_state.counter, key->multisample ? sample_loop_state.counter : NULL);
                  /*
   * Clamp according to ARB_depth_clamp semantics.
   */
   z = lp_build_depth_clamp(gallivm, builder, key->depth_clamp,
                        if (nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_STENCIL)) {
      LLVMValueRef idx = loop_state.counter;
   if (key->min_samples > 1)
      idx = LLVMBuildAdd(builder, idx,
      LLVMValueRef ptr = LLVMBuildGEP2(builder, vec_type, s_out, &idx, 1, "");
   stencil_refs[0] = LLVMBuildLoad2(builder, vec_type, ptr, "output.s");
   /* there's only one value, and spec says to discard additional bits */
   LLVMValueRef s_max_mask = lp_build_const_int_vec(gallivm, int_type, 255);
   stencil_refs[0] = LLVMBuildBitCast(builder, stencil_refs[0], int_vec_type, "");
   stencil_refs[0] = LLVMBuildAnd(builder, stencil_refs[0], s_max_mask, "");
               lp_build_depth_stencil_load_swizzled(gallivm, type,
                        lp_build_depth_stencil_test(gallivm,
                              &key->depth,
   key->stencil,
   type,
   zs_format_desc,
   key->multisample ? NULL : &mask,
   &s_mask,
   stencil_refs,
      /* Late Z write */
   if (depth_mode & LATE_DEPTH_WRITE) {
      lp_build_depth_stencil_write_swizzled(gallivm, type,
                           } else if ((depth_mode & EARLY_DEPTH_TEST) &&
            /* Need to apply a reduced mask to the depth write.  Reload the
   * depth value, update from zs_value with the new mask value and
   * write that out.
   */
   if (key->multisample) {
      z_value = LLVMBuildBitCast(builder, lp_build_pointer_get2(builder, int_vec_type, z_sample_value_store, sample_loop_state.counter), z_type, "");
   s_value = lp_build_pointer_get2(builder, int_vec_type, s_sample_value_store, sample_loop_state.counter);
   z_fb = LLVMBuildBitCast(builder, lp_build_pointer_get2(builder, int_vec_type, z_fb_store, sample_loop_state.counter), z_fb_type, "");
      }
   lp_build_depth_stencil_write_swizzled(gallivm, type,
                                 if (key->occlusion_count && (!(depth_mode & EARLY_DEPTH_TEST) || (depth_mode & EARLY_DEPTH_TEST_INFERRED))) {
      LLVMValueRef counter = lp_jit_thread_data_vis_counter(gallivm, thread_data_type, thread_data_ptr);
            lp_build_occlusion_count(gallivm, type,
               /* if this is genuine early-depth in the shader, write samplemask now
   * after occlusion count has been updated
   */
   if (key->multisample &&
      nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK) &&
   (depth_mode & (EARLY_DEPTH_TEST_INFERRED | EARLY_DEPTH_TEST)) == EARLY_DEPTH_TEST) {
   /* if the shader writes sample mask use that */
      LLVMValueRef out_smask_idx = LLVMBuildShl(builder, lp_build_const_int32(gallivm, 1), sample_loop_state.counter, "");
   out_smask_idx = lp_build_broadcast(gallivm, int_vec_type, out_smask_idx);
   LLVMValueRef output_smask = LLVMBuildLoad2(builder, int_vec_type, out_sample_mask_storage, "");
   LLVMValueRef smask_bit = LLVMBuildAnd(builder, output_smask, out_smask_idx, "");
                              if (key->multisample) {
      /* store the sample mask for this loop */
   LLVMBuildStore(builder, s_mask, s_mask_ptr);
               mask_val = lp_build_mask_end(&mask);
   if (!key->multisample)
            }
         /**
   * This function will reorder pixels from the fragment shader SoA to memory
   * layout AoS
   *
   * Fragment Shader outputs pixels in small 2x2 blocks
   *  e.g. (0, 0), (1, 0), (0, 1), (1, 1) ; (2, 0) ...
   *
   * However in memory pixels are stored in rows
   *  e.g. (0, 0), (1, 0), (2, 0), (3, 0) ; (0, 1) ...
   *
   * @param type            fragment shader type (4x or 8x float)
   * @param num_fs          number of fs_src
   * @param is_1d           whether we're outputting to a 1d resource
   * @param dst_channels    number of output channels
   * @param fs_src          output from fragment shader
   * @param dst             pointer to store result
   * @param pad_inline      is channel padding inline or at end of row
   * @return                the number of dsts
   */
   static int
   generate_fs_twiddle(struct gallivm_state *gallivm,
                     struct lp_type type,
   unsigned num_fs,
   unsigned dst_channels,
   {
      LLVMValueRef src[16];
   unsigned pixels = type.length / 4;
   unsigned src_channels = dst_channels < 3 ? dst_channels : 4;
            assert(pixels == 2 || pixels == 1);
            /*
   * Transpose from SoA -> AoS
   */
   for (unsigned i = 0; i < num_fs; ++i) {
      lp_build_transpose_aos_n(gallivm, type, &fs_src[i][0], src_channels,
               /*
   * Pick transformation options
   */
   bool swizzle_pad = false;
   bool twiddle = false;
   bool split = false;
            if (dst_channels == 1) {
      twiddle = true;
   if (pixels == 2) {
            } else if (dst_channels == 2) {
      if (pixels == 1) {
            } else if (dst_channels > 2) {
      if (pixels == 1) {
         } else {
                  if (!pad_inline && dst_channels == 3 && pixels > 1) {
                     /*
   * Split the src in half
   */
   if (split) {
      for (unsigned i = num_fs; i > 0; --i) {
      src[(i - 1)*2 + 1] = lp_build_extract_range(gallivm, src[i - 1], 4, 4);
               src_count *= 2;
               /*
   * Ensure pixels are in memory order
   */
   if (reorder_group) {
      /* Twiddle pixels by reordering the array, e.g.:
   *
   * src_count =  8 -> 0 2 1 3 4 6 5 7
   * src_count = 16 -> 0 1 4 5 2 3 6 7 8 9 12 13 10 11 14 15
   */
            for (unsigned i = 0; i < src_count; ++i) {
      unsigned group = i / reorder_group;
   unsigned block = (group / 4) * 4 * reorder_group;
   unsigned j = block + (reorder_sw[group % 4] * reorder_group) + (i % reorder_group);
         } else if (twiddle) {
      /* Twiddle pixels across elements of array */
   /*
   * XXX: we should avoid this in some cases, but would need to tell
   * lp_build_conv to reorder (or deal with it ourselves).
   */
      } else {
      /* Do nothing */
               /*
   * Moves any padding between pixels to the end
   * e.g. RGBXRGBX -> RGBRGBXX
   */
   if (swizzle_pad) {
      unsigned char swizzles[16];
            for (unsigned i = 0; i < type.length; ++i) {
      if (i < elems)
         else
               for (unsigned i = 0; i < src_count; ++i) {
      dst[i] = lp_build_swizzle_aos_n(gallivm, dst[i], swizzles,
                     }
         /*
   * Untwiddle and transpose, much like the above.
   * However, this is after conversion, so we get packed vectors.
   * At this time only handle 4x16i8 rgba / 2x16i8 rg / 1x16i8 r data,
   * the vectors will look like:
   * r0r1r4r5r2r3r6r7r8r9r12... (albeit color channels may
   * be swizzled here). Extending to 16bit should be trivial.
   * Should also be extended to handle twice wide vectors with AVX2...
   */
   static void
   fs_twiddle_transpose(struct gallivm_state *gallivm,
                           {
      struct lp_type type64, type16, type32;
   LLVMTypeRef type64_t, type8_t, type16_t, type32_t;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef tmp[4], shuf[8];
   for (unsigned j = 0; j < 2; j++) {
      shuf[j*4 + 0] = lp_build_const_int32(gallivm, j*4 + 0);
   shuf[j*4 + 1] = lp_build_const_int32(gallivm, j*4 + 2);
   shuf[j*4 + 2] = lp_build_const_int32(gallivm, j*4 + 1);
               assert(src_count == 4 || src_count == 2 || src_count == 1);
   assert(type.width == 8);
                     type64 = type;
   type64.length /= 8;
   type64.width *= 8;
            type16 = type;
   type16.length /= 2;
   type16.width *= 2;
            type32 = type;
   type32.length /= 4;
   type32.width *= 4;
                     if (src_count == 1) {
      /* transpose was no-op, just untwiddle */
   LLVMValueRef shuf_vec;
   shuf_vec = LLVMConstVector(shuf, 8);
   tmp[0] = LLVMBuildBitCast(builder, src[0], type16_t, "");
   tmp[0] = LLVMBuildShuffleVector(builder, tmp[0], tmp[0], shuf_vec, "");
      } else if (src_count == 2) {
      LLVMValueRef shuf_vec;
            for (unsigned i = 0; i < 2; i++) {
      tmp[i] = LLVMBuildBitCast(builder, tmp[i], type32_t, "");
   tmp[i] = LLVMBuildShuffleVector(builder, tmp[i], tmp[i], shuf_vec, "");
         } else {
      for (unsigned j = 0; j < 2; j++) {
      LLVMValueRef lo, hi, lo2, hi2;
   /*
   * Note that if we only really have 3 valid channels (rgb)
   * and we don't need alpha we could substitute a undef here
   * for the respective channel (causing llvm to drop conversion
   * for alpha).
   */
   /* we now have rgba0rgba1rgba4rgba5 etc, untwiddle */
   lo2 = LLVMBuildBitCast(builder, tmp[j*2], type64_t, "");
   hi2 = LLVMBuildBitCast(builder, tmp[j*2 + 1], type64_t, "");
   lo = lp_build_interleave2(gallivm, type64, lo2, hi2, 0);
   hi = lp_build_interleave2(gallivm, type64, lo2, hi2, 1);
   dst[j*2] = LLVMBuildBitCast(builder, lo, type8_t, "");
            }
         /**
   * Load an unswizzled block of pixels from memory
   */
   static void
   load_unswizzled_block(struct gallivm_state *gallivm,
                        LLVMTypeRef base_type,
   LLVMValueRef base_ptr,
   LLVMValueRef stride,
   unsigned block_width,
   unsigned block_height,
      {
      LLVMBuilderRef builder = gallivm->builder;
            /* Ensure block exactly fits into dst */
            for (unsigned i = 0; i < dst_count; ++i) {
      unsigned x = i % row_size;
            LLVMValueRef bx = lp_build_const_int32(gallivm, x * (dst_type.width / 8) * dst_type.length);
            LLVMValueRef gep[2];
            gep[0] = lp_build_const_int32(gallivm, 0);
            dst_ptr = LLVMBuildGEP2(builder, base_type, base_ptr, gep, 2, "");
   dst_ptr = LLVMBuildBitCast(builder, dst_ptr,
            dst[i] = LLVMBuildLoad2(builder,
                        }
         /**
   * Store an unswizzled block of pixels to memory
   */
   static void
   store_unswizzled_block(struct gallivm_state *gallivm,
                        LLVMTypeRef base_type,
   LLVMValueRef base_ptr,
   LLVMValueRef stride,
   unsigned block_width,
   unsigned block_height,
      {
      LLVMBuilderRef builder = gallivm->builder;
            /* Ensure src exactly fits into block */
            for (unsigned i = 0; i < src_count; ++i) {
      unsigned x = i % row_size;
            LLVMValueRef bx = lp_build_const_int32(gallivm, x * (src_type.width / 8) * src_type.length);
            LLVMValueRef gep[2];
            gep[0] = lp_build_const_int32(gallivm, 0);
            src_ptr = LLVMBuildGEP2(builder, base_type, base_ptr, gep, 2, "");
   src_ptr = LLVMBuildBitCast(builder, src_ptr,
                           }
            /**
   * Retrieves the type for a format which is usable in the blending code.
   *
   * e.g. RGBA16F = 4x float, R3G3B2 = 3x byte
   */
   static inline void
   lp_blend_type_from_format_desc(const struct util_format_description *format_desc,
         {
      if (format_expands_to_float_soa(format_desc)) {
      /* always use ordinary floats for blending */
   type->floating = true;
   type->fixed = false;
   type->sign = true;
   type->norm = false;
   type->width = 32;
   type->length = 4;
                        memset(type, 0, sizeof(struct lp_type));
   type->floating = format_desc->channel[chan].type == UTIL_FORMAT_TYPE_FLOAT;
   type->fixed    = format_desc->channel[chan].type == UTIL_FORMAT_TYPE_FIXED;
   type->sign     = format_desc->channel[chan].type != UTIL_FORMAT_TYPE_UNSIGNED;
   type->norm     = format_desc->channel[chan].normalized;
   type->width    = format_desc->channel[chan].size;
            for (unsigned i = 1; i < format_desc->nr_channels; ++i) {
      if (format_desc->channel[i].size > type->width)
               if (type->floating) {
         } else {
      if (type->width <= 8) {
         } else if (type->width <= 16) {
         } else {
                     if (is_arithmetic_format(format_desc) && type->length == 3) {
            }
         /**
   * Scale a normalized value from src_bits to dst_bits.
   *
   * The exact calculation is
   *
   *    dst = iround(src * dst_mask / src_mask)
   *
   *  or with integer rounding
   *
   *    dst = src * (2*dst_mask + sign(src)*src_mask) / (2*src_mask)
   *
   *  where
   *
   *    src_mask = (1 << src_bits) - 1
   *    dst_mask = (1 << dst_bits) - 1
   *
   * but we try to avoid division and multiplication through shifts.
   */
   static inline LLVMValueRef
   scale_bits(struct gallivm_state *gallivm,
            int src_bits,
   int dst_bits,
      {
      LLVMBuilderRef builder = gallivm->builder;
            if (dst_bits < src_bits) {
                           if (dst_bits == 4) {
                     result = lp_build_unsigned_norm_to_float(gallivm, src_bits,
         result = lp_build_clamped_float_to_unsigned_norm(gallivm, flt_type,
         result = LLVMBuildTrunc(gallivm->builder, result,
                           /*
   * Approximate the rescaling with a single shift.
   *
                  result = LLVMBuildLShr(builder, src,
                  } else {
      /*
                  /*
   * Drop the least significant bits to make space for the
   * multiplication.
   *
   * XXX: A better approach would be to use a wider integer type as
   * intermediate.  But this is enough to convert alpha from 16bits ->
   * 2 when rendering to PIPE_FORMAT_R10G10B10A2_UNORM.
   */
   result = LLVMBuildLShr(builder, src,
                           result = LLVMBuildMul(builder, result,
                        /*
   * Add a rounding term before the division.
   *
   * TODO: Handle signed integers too.
   */
   if (!src_type.sign) {
      result = LLVMBuildAdd(builder, result,
                           /*
   * Approximate the division by src_mask with a src_bits shift.
   *
   * Given the src has already been shifted by dst_bits, all we need
                  result = LLVMBuildLShr(builder,
                        } else if (dst_bits > src_bits) {
      /* Scale up bits */
            /* Shift left by difference in bits */
   result = LLVMBuildShl(builder,
                        if (db <= src_bits) {
      /* Enough bits in src to fill the remainder */
   LLVMValueRef lower = LLVMBuildLShr(builder,
                           } else if (db > src_bits) {
                                       result = LLVMBuildOr(builder,
                                    }
      /**
   * If RT is a smallfloat (needing denorms) format
   */
   static inline int
   have_smallfloat_format(struct lp_type dst_type,
         {
      return ((dst_type.floating && dst_type.width != 32) ||
   /* due to format handling hacks this format doesn't have floating set
   * here (and actually has width set to 32 too) so special case this.
   */
      }
         /**
   * Convert from memory format to blending format
   *
   * e.g. GL_R3G3B2 is 1 byte in memory but 3 bytes for blending
   */
   static void
   convert_to_blend_type(struct gallivm_state *gallivm,
                        unsigned block_size,
   const struct util_format_description *src_fmt,
      {
      LLVMValueRef *dst = src;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_type blend_type;
   struct lp_type mem_type;
   unsigned i, j;
   unsigned pixels = block_size / num_srcs;
            /*
   * full custom path for packed floats and srgb formats - none of the later
   * functions would do anything useful, and given the lp_type representation
   * they can't be fixed. Should really have some SoA blend path for these
   * kind of formats rather than hacking them in here.
   */
   if (format_expands_to_float_soa(src_fmt)) {
      LLVMValueRef tmpsrc[4];
   /*
   * This is pretty suboptimal for this case blending in SoA would be much
   * better, since conversion gets us SoA values so need to convert back.
   */
   assert(src_type.width == 32 || src_type.width == 16);
   assert(dst_type.floating);
   assert(dst_type.width == 32);
   assert(dst_type.length % 4 == 0);
            if (src_type.width == 16) {
      /* expand 4x16bit values to 4x32bit */
   struct lp_type type32x4 = src_type;
   LLVMTypeRef ltype32x4;
   unsigned num_fetch = dst_type.length == 8 ? num_srcs / 2 : num_srcs / 4;
   type32x4.width = 32;
   ltype32x4 = lp_build_vec_type(gallivm, type32x4);
   for (i = 0; i < num_fetch; i++) {
         }
      }
   for (i = 0; i < 4; i++) {
         }
   for (i = 0; i < num_srcs / 4; i++) {
      LLVMValueRef tmpsoa[4];
   LLVMValueRef tmps = tmpsrc[i];
   if (dst_type.length == 8) {
      LLVMValueRef shuffles[8];
   unsigned j;
   /* fetch was 4 values but need 8-wide output values */
   tmps = lp_build_concat(gallivm, &tmpsrc[i * 2], src_type, 2);
   /*
   * for 8-wide aos transpose would give us wrong order not matching
   * incoming converted fs values and mask. ARGH.
   */
   for (j = 0; j < 4; j++) {
      shuffles[j] = lp_build_const_int32(gallivm, j * 2);
      }
   tmps = LLVMBuildShuffleVector(builder, tmps, tmps,
      }
   if (src_fmt->format == PIPE_FORMAT_R11G11B10_FLOAT) {
         } else {
         }
      }
               lp_mem_type_from_format_desc(src_fmt, &mem_type);
            /* Is the format arithmetic */
   is_arith = blend_type.length * blend_type.width != mem_type.width * mem_type.length;
            /* Pad if necessary */
   if (!is_arith && src_type.length < dst_type.length) {
      for (i = 0; i < num_srcs; ++i) {
                              /* Special case for half-floats */
   if (mem_type.width == 16 && mem_type.floating) {
      assert(blend_type.width == 32 && blend_type.floating);
   lp_build_conv_auto(gallivm, src_type, &dst_type, dst, num_srcs, dst);
               if (!is_arith) {
                  src_type.width = blend_type.width * blend_type.length;
   blend_type.length *= pixels;
            for (i = 0; i < num_srcs; ++i) {
      LLVMValueRef chans;
                     for (j = 0; j < src_fmt->nr_channels; ++j) {
         #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
                        /* Extract bits from source */
   chans = LLVMBuildLShr(builder,
                        chans = LLVMBuildAnd(builder,
                        /* Scale bits */
   if (src_type.norm) {
      chans = scale_bits(gallivm, src_fmt->channel[j].size,
               /* Insert bits into correct position */
   chans = LLVMBuildShl(builder,
                        if (j == 0) {
         } else {
                           }
         /**
   * Convert from blending format to memory format
   *
   * e.g. GL_R3G3B2 is 3 bytes for blending but 1 byte in memory
   */
   static void
   convert_from_blend_type(struct gallivm_state *gallivm,
                           unsigned block_size,
   const struct util_format_description *src_fmt,
   {
      LLVMValueRef* dst = src;
   unsigned i, j, k;
   struct lp_type mem_type;
   struct lp_type blend_type;
   LLVMBuilderRef builder = gallivm->builder;
   unsigned pixels = block_size / num_srcs;
            /*
   * full custom path for packed floats and srgb formats - none of the later
   * functions would do anything useful, and given the lp_type representation
   * they can't be fixed. Should really have some SoA blend path for these
   * kind of formats rather than hacking them in here.
   */
   if (format_expands_to_float_soa(src_fmt)) {
      /*
   * This is pretty suboptimal for this case blending in SoA would be much
   * better - we need to transpose the AoS values back to SoA values for
   * conversion/packing.
   */
   assert(src_type.floating);
   assert(src_type.width == 32);
   assert(src_type.length % 4 == 0);
            for (i = 0; i < num_srcs / 4; i++) {
      LLVMValueRef tmpsoa[4], tmpdst;
                  if (src_fmt->format == PIPE_FORMAT_R11G11B10_FLOAT) {
         } else {
      tmpdst = lp_build_float_to_srgb_packed(gallivm, src_fmt,
               if (src_type.length == 8) {
      LLVMValueRef tmpaos, shuffles[8];
   unsigned j;
   /*
   * for 8-wide aos transpose has given us wrong order not matching
   * output order. HMPF. Also need to split the output values
   * manually.
   */
   for (j = 0; j < 4; j++) {
      shuffles[j * 2] = lp_build_const_int32(gallivm, j);
      }
   tmpaos = LLVMBuildShuffleVector(builder, tmpdst, tmpdst,
         src[i * 2] = lp_build_extract_range(gallivm, tmpaos, 0, 4);
      } else {
            }
   if (dst_type.width == 16) {
      struct lp_type type16x8 = dst_type;
   struct lp_type type32x4 = dst_type;
   LLVMTypeRef ltype16x4, ltypei64, ltypei128;
   unsigned num_fetch = src_type.length == 8 ? num_srcs / 2 : num_srcs / 4;
   type16x8.length = 8;
   type32x4.width = 32;
   ltypei128 = LLVMIntTypeInContext(gallivm->context, 128);
   ltypei64 = LLVMIntTypeInContext(gallivm->context, 64);
   ltype16x4 = lp_build_vec_type(gallivm, dst_type);
   /* We could do vector truncation but it doesn't generate very good code */
   for (i = 0; i < num_fetch; i++) {
      src[i] = lp_build_pack2(gallivm, type32x4, type16x8,
         src[i] = LLVMBuildBitCast(builder, src[i], ltypei128, "");
   src[i] = LLVMBuildTrunc(builder, src[i], ltypei64, "");
         }
               lp_mem_type_from_format_desc(src_fmt, &mem_type);
                     /* Special case for half-floats */
   if (mem_type.width == 16 && mem_type.floating) {
      int length = dst_type.length;
                              dst_type.length = length;
               /* Remove any padding */
   if (!is_arith && (src_type.length % mem_type.length)) {
               for (i = 0; i < num_srcs; ++i) {
                     /* No bit arithmetic to do */
   if (!is_arith) {
                  src_type.length = pixels;
   src_type.width = blend_type.length * blend_type.width;
            for (i = 0; i < num_srcs; ++i) {
      LLVMValueRef chans;
                     for (j = 0; j < src_fmt->nr_channels; ++j) {
      unsigned mask = 0;
      #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
                        for (k = 0; k < blend_type.width; ++k) {
                  /* Extract bits */
   chans = LLVMBuildLShr(builder,
                              chans = LLVMBuildAnd(builder,
                        /* Scale down bits */
   if (src_type.norm) {
      chans = scale_bits(gallivm, blend_type.width,
      } else if (!src_type.floating && sz_a < blend_type.width) {
      LLVMValueRef mask_val = lp_build_const_int_vec(gallivm, src_type, (1UL << sz_a) - 1);
   LLVMValueRef mask = LLVMBuildICmp(builder, LLVMIntUGT, chans, mask_val, "");
               /* Insert bits */
   chans = LLVMBuildShl(builder,
                                 if (j == 0) {
         } else {
                                    }
         /**
   * Convert alpha to same blend type as src
   */
   static void
   convert_alpha(struct gallivm_state *gallivm,
               struct lp_type row_type,
   struct lp_type alpha_type,
   const unsigned block_size,
   const unsigned block_height,
   const unsigned src_count,
   const unsigned dst_channels,
   {
      LLVMBuilderRef builder = gallivm->builder;
   const unsigned length = row_type.length;
            /* Twiddle the alpha to match pixels */
            /*
   * TODO this should use single lp_build_conv call for
   * src_count == 1 && dst_channels == 1 case (dropping the concat below)
   */
   for (unsigned i = 0; i < block_height; ++i) {
      lp_build_conv(gallivm, alpha_type, row_type, &src_alpha[i], 1,
               alpha_type = row_type;
            /* If only one channel we can only need the single alpha value per pixel */
   if (src_count == 1 && dst_channels == 1) {
      lp_build_concat_n(gallivm, alpha_type, src_alpha, block_height,
      } else {
      /* If there are more srcs than rows then we need to split alpha up */
   if (src_count > block_height) {
      for (unsigned i = src_count; i > 0; --i) {
                     src_alpha[idx] =
      lp_build_extract_range(gallivm, src_alpha[(idx * pixels) / 4],
               /* If there is a src for each pixel broadcast the alpha across whole
   * row
   */
   if (src_count == block_size) {
      for (unsigned i = 0; i < src_count; ++i) {
      src_alpha[i] = lp_build_broadcast(gallivm,
         } else {
      unsigned pixels = block_size / src_count;
   unsigned channels = pad_inline ? TGSI_NUM_CHANNELS : dst_channels;
                  /* Check if we need 2 src_alphas for our shuffles */
   if (pixels > alpha_type.length) {
                  /* Broadcast alpha across all channels, e.g. a1a2 to a1a1a1a1a2a2a2a2 */
   for (unsigned j = 0; j < row_type.length; ++j) {
      if (j < pixels * channels) {
         } else {
                                       if (alpha_span > 1){
                        src_alpha[i] = LLVMBuildShuffleVector(builder,
                                 }
         /**
   * Generates the blend function for unswizzled colour buffers
   * Also generates the read & write from colour buffer
   */
   static void
   generate_unswizzled_blend(struct gallivm_state *gallivm,
                           unsigned rt,
   struct lp_fragment_shader_variant *variant,
   enum pipe_format out_format,
   unsigned int num_fs,
   struct lp_type fs_type,
   LLVMValueRef* fs_mask,
   LLVMValueRef fs_out_color[PIPE_MAX_COLOR_BUFS][TGSI_NUM_CHANNELS][4],
   LLVMTypeRef context_type,
   LLVMValueRef context_ptr,
   LLVMTypeRef color_type,
   {
      const unsigned alpha_channel = 3;
   const unsigned block_width = LP_RASTER_BLOCK_SIZE;
   const unsigned block_height = LP_RASTER_BLOCK_SIZE;
   const unsigned block_size = block_width * block_height;
            LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef fs_src[4][TGSI_NUM_CHANNELS];
   LLVMValueRef fs_src1[4][TGSI_NUM_CHANNELS];
   LLVMValueRef src_alpha[4 * 4];
   LLVMValueRef src1_alpha[4 * 4] = { NULL };
   LLVMValueRef src_mask[4 * 4];
   LLVMValueRef src[4 * 4];
   LLVMValueRef src1[4 * 4];
                     unsigned char swizzle[TGSI_NUM_CHANNELS];
            const struct util_format_description *out_format_desc =
            bool pad_inline = is_arithmetic_format(out_format_desc);
   const bool dual_source_blend =
      variant->key.blend.rt[0].blend_enable &&
         const bool is_1d = variant->key.resource_1d;
   const unsigned num_fullblock_fs = is_1d ? 2 * num_fs : num_fs;
                     /* Get type from output format */
   struct lp_type row_type, dst_type;
   lp_blend_type_from_format_desc(out_format_desc, &row_type);
            /*
   * Technically this code should go into lp_build_smallfloat_to_float
   * and lp_build_float_to_smallfloat but due to the
   * http://llvm.org/bugs/show_bug.cgi?id=6393
   * llvm reorders the mxcsr intrinsics in a way that breaks the code.
   * So the ordering is important here and there shouldn't be any
   * llvm ir instrunctions in this function before
   * this, otherwise half-float format conversions won't work
   * (again due to llvm bug #6393).
   */
   if (have_smallfloat_format(dst_type, out_format)) {
      /* We need to make sure that denorms are ok for half float
         fpstate = lp_build_fpstate_get(gallivm);
               struct lp_type mask_type = lp_int32_vec4_type();
            for (unsigned i = num_fs; i < num_fullblock_fs; i++) {
                  /* Do not bother executing code when mask is empty.. */
   if (do_branch) {
      LLVMValueRef check_mask =
            for (unsigned i = 0; i < num_fullblock_fs; ++i) {
                  lp_build_mask_begin(&mask_ctx, gallivm, mask_type, check_mask);
               partial_mask |= !variant->opaque;
                     row_type.length = fs_type.length;
   unsigned vector_width =
            /* Compute correct swizzle and count channels */
   memset(swizzle, LP_BLD_SWIZZLE_DONTCARE, TGSI_NUM_CHANNELS);
            bool has_alpha = false;
   for (unsigned i = 0; i < TGSI_NUM_CHANNELS; ++i) {
      /* Ensure channel is used */
   if (out_format_desc->swizzle[i] >= TGSI_NUM_CHANNELS) {
                  /* Ensure not already written to (happens in case with GL_ALPHA) */
   if (swizzle[out_format_desc->swizzle[i]] < TGSI_NUM_CHANNELS) {
                  /* Ensure we haven't already found all channels */
   if (dst_channels >= out_format_desc->nr_channels) {
                  swizzle[out_format_desc->swizzle[i]] = i;
            if (i == alpha_channel) {
                     if (format_expands_to_float_soa(out_format_desc)) {
      /*
   * the code above can't work for layout_other
   * for srgb it would sort of work but we short-circuit swizzles, etc.
   * as that is done as part of unpack / pack.
   */
   dst_channels = 4; /* HACK: this is fake 4 really but need it due to transpose stuff later */
   has_alpha = true;
   swizzle[0] = 0;
   swizzle[1] = 1;
   swizzle[2] = 2;
   swizzle[3] = 3;
               /* If 3 channels then pad to include alpha for 4 element transpose */
   if (dst_channels == 3) {
      assert (!has_alpha);
   for (unsigned i = 0; i < TGSI_NUM_CHANNELS; i++) {
      if (swizzle[i] > TGSI_NUM_CHANNELS)
      }
   if (out_format_desc->nr_channels == 4) {
      dst_channels = 4;
   /*
   * We use alpha from the color conversion, not separate one.
   * We had to include it for transpose, hence it will get converted
   * too (albeit when doing transpose after conversion, that would
   * no longer be the case necessarily).
   * (It works only with 4 channel dsts, e.g. rgbx formats, because
   * otherwise we really have padding, not alpha, included.)
   */
                  /*
   * Load shader output
   */
   for (unsigned i = 0; i < num_fullblock_fs; ++i) {
      /* Always load alpha for use in blending */
   LLVMValueRef alpha;
   if (i < num_fs) {
      alpha = LLVMBuildLoad2(builder, fs_vec_type,
      } else {
                  /* Load each channel */
   for (unsigned j = 0; j < dst_channels; ++j) {
      assert(swizzle[j] < 4);
   if (i < num_fs) {
      fs_src[i][j] = LLVMBuildLoad2(builder, fs_vec_type,
      } else {
                     /* If 3 channels then pad to include alpha for 4 element transpose */
   /*
   * XXX If we include that here maybe could actually use it instead of
   * separate alpha for blending?
   * (Difficult though we actually convert pad channels, not alpha.)
   */
   if (dst_channels == 3 && !has_alpha) {
                  /* We split the row_mask and row_alpha as we want 128bit interleave */
   if (fs_type.length == 8) {
      src_mask[i*2 + 0]  = lp_build_extract_range(gallivm, fs_mask[i],
         src_mask[i*2 + 1]  = lp_build_extract_range(gallivm, fs_mask[i],
                  src_alpha[i*2 + 0] = lp_build_extract_range(gallivm, alpha,
         src_alpha[i*2 + 1] = lp_build_extract_range(gallivm, alpha,
            } else {
      src_mask[i] = fs_mask[i];
         }
   if (dual_source_blend) {
      /* same as above except different src/dst, skip masks and comments... */
   for (unsigned i = 0; i < num_fullblock_fs; ++i) {
      LLVMValueRef alpha;
   if (i < num_fs) {
      alpha = LLVMBuildLoad2(builder, fs_vec_type,
      } else {
                  for (unsigned j = 0; j < dst_channels; ++j) {
      assert(swizzle[j] < 4);
   if (i < num_fs) {
      fs_src1[i][j] = LLVMBuildLoad2(builder, fs_vec_type,
      } else {
            }
   if (dst_channels == 3 && !has_alpha) {
         }
   if (fs_type.length == 8) {
      src1_alpha[i*2 + 0] = lp_build_extract_range(gallivm, alpha, 0, src_channels);
   src1_alpha[i*2 + 1] = lp_build_extract_range(gallivm, alpha,
      } else {
                        if (util_format_is_pure_integer(out_format)) {
      /*
   * In this case fs_type was really ints or uints disguised as floats,
   * fix that up now.
   */
   fs_type.floating = 0;
   fs_type.sign = dst_type.sign;
   fs_vec_type = lp_build_vec_type(gallivm, fs_type);
   for (unsigned i = 0; i < num_fullblock_fs; ++i) {
      for (unsigned j = 0; j < dst_channels; ++j) {
      fs_src[i][j] = LLVMBuildBitCast(builder, fs_src[i][j],
      }
   if (dst_channels == 3 && !has_alpha) {
      fs_src[i][3] = LLVMBuildBitCast(builder, fs_src[i][3],
                     /*
   * We actually should generally do conversion first (for non-1d cases)
   * when the blend format is 8 or 16 bits. The reason is obvious,
   * there's 2 or 4 times less vectors to deal with for the interleave...
   * Albeit for the AVX (not AVX2) case there's no benefit with 16 bit
   * vectors (as it can do 32bit unpack with 256bit vectors, but 8/16bit
   * unpack only with 128bit vectors).
   * Note: for 16bit sizes really need matching pack conversion code
   */
   bool twiddle_after_convert = false;
   if (!is_1d && dst_channels != 3 && dst_type.width == 8) {
                  /*
   * Pixel twiddle from fragment shader order to memory order
   */
   unsigned src_count;
   if (!twiddle_after_convert) {
      src_count = generate_fs_twiddle(gallivm, fs_type, num_fullblock_fs,
         if (dual_source_blend) {
      generate_fs_twiddle(gallivm, fs_type, num_fullblock_fs, dst_channels,
         } else {
      src_count = num_fullblock_fs * dst_channels;
   /*
   * We reorder things a bit here, so the cases for 4-wide and 8-wide
   * (AVX) turn out the same later when untwiddling/transpose (albeit
   * for true AVX2 path untwiddle needs to be different).
   * For now just order by colors first (so we can use unpack later).
   */
   for (unsigned j = 0; j < num_fullblock_fs; j++) {
      for (unsigned i = 0; i < dst_channels; i++) {
      src[i*num_fullblock_fs + j] = fs_src[j][i];
   if (dual_source_blend) {
                           src_channels = dst_channels < 3 ? dst_channels : 4;
   if (src_count != num_fullblock_fs * src_channels) {
      unsigned ds = src_count / (num_fullblock_fs * src_channels);
   row_type.length /= ds;
   fs_type.length = row_type.length;
               struct lp_type blend_type = row_type;
            /* Convert src to row_type */
   if (dual_source_blend) {
      struct lp_type old_row_type = row_type;
   lp_build_conv_auto(gallivm, fs_type, &row_type, src, src_count, src);
   src_count = lp_build_conv_auto(gallivm, fs_type, &old_row_type,
      } else {
      src_count = lp_build_conv_auto(gallivm, fs_type, &row_type,
               /* If the rows are not an SSE vector, combine them to become SSE size! */
   if ((row_type.width * row_type.length) % 128) {
      unsigned bits = row_type.width * row_type.length;
                              combined = lp_build_concat_n(gallivm, row_type, src, src_count,
         if (dual_source_blend) {
                  row_type.length *= combined;
            bits = row_type.width * row_type.length;
               if (twiddle_after_convert) {
      fs_twiddle_transpose(gallivm, row_type, src, src_count, src);
   if (dual_source_blend) {
                     /*
   * Blend Colour conversion
   */
   LLVMValueRef blend_color =
         blend_color = LLVMBuildPointerCast(builder, blend_color,
               blend_color = LLVMBuildLoad2(builder, fs_vec_type,
                        /* Convert */
   lp_build_conv(gallivm, fs_type, blend_type, &blend_color, 1,
            if (out_format_desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) {
      /*
   * since blending is done with floats, there was no conversion.
   * However, the rules according to fixed point renderbuffers still
   * apply, that is we must clamp inputs to 0.0/1.0.
   * (This would apply to separate alpha conversion too but we currently
   * force has_alpha to be true.)
   * TODO: should skip this with "fake" blend, since post-blend conversion
   * will clamp anyway.
   * TODO: could also skip this if fragment color clamping is enabled.
   * We don't support it natively so it gets baked into the shader
   * however, so can't really tell here.
   */
   struct lp_build_context f32_bld;
   assert(row_type.floating);
   lp_build_context_init(&f32_bld, gallivm, row_type);
   for (unsigned i = 0; i < src_count; i++) {
         }
   if (dual_source_blend) {
      for (unsigned i = 0; i < src_count; i++) {
            }
   /* probably can't be different than row_type but better safe than sorry... */
   lp_build_context_init(&f32_bld, gallivm, blend_type);
   blend_color = lp_build_clamp(&f32_bld, blend_color,
               /* Extract alpha */
   LLVMValueRef blend_alpha =
      lp_build_extract_broadcast(gallivm, blend_type, row_type,
               /* Swizzle to appropriate channels, e.g. from RGBA to BGRA BGRA */
   pad_inline &= (dst_channels * (block_size / src_count) * row_type.width)
         if (pad_inline) {
      /* Use all 4 channels e.g. from RGBA RGBA to RGxx RGxx */
   blend_color = lp_build_swizzle_aos_n(gallivm, blend_color, swizzle,
      } else {
      /* Only use dst_channels e.g. RGBA RGBA to RG RG xxxx */
   blend_color = lp_build_swizzle_aos_n(gallivm, blend_color, swizzle,
               /*
   * Mask conversion
   */
   lp_bld_quad_twiddle(gallivm, mask_type, &src_mask[0],
            if (src_count < block_height) {
         } else if (src_count > block_height) {
      for (unsigned i = src_count; i > 0; --i) {
                     src_mask[idx] = lp_build_extract_range(gallivm,
                                 for (unsigned i = 0; i < src_count; ++i) {
      unsigned pixels = block_size / src_count;
            if (pixel_width == 24) {
      mask_type.width = 8;
      } else {
                     /*
   * If mask_type width is smaller than 32bit, this doesn't quite
   * generate the most efficient code (could use some pack).
   */
   src_mask[i] = LLVMBuildIntCast(builder, src_mask[i],
                  mask_type.length *= dst_channels;
               src_mask[i] = LLVMBuildBitCast(builder, src_mask[i],
                           /*
   * Alpha conversion
   */
   if (!has_alpha) {
      struct lp_type alpha_type = fs_type;
   alpha_type.length = 4;
   convert_alpha(gallivm, row_type, alpha_type,
               block_size, block_height,
   if (dual_source_blend) {
      convert_alpha(gallivm, row_type, alpha_type,
                                 /*
   * Load dst from memory
   */
   unsigned dst_count;
   if (src_count < block_height) {
         } else {
                           if (format_expands_to_float_soa(out_format_desc)) {
      /*
   * we need multiple values at once for the conversion, so can as well
   * load them vectorized here too instead of concatenating later.
   * (Still need concatenation later for 8-wide vectors).
   */
   dst_count = block_height;
               /*
   * Compute the alignment of the destination pointer in bytes
   * We fetch 1-4 pixels, if the format has pot alignment then those fetches
   * are always aligned by MIN2(16, fetch_width) except for buffers (not
   * 1d tex but can't distinguish here) so need to stick with per-pixel
   * alignment in this case.
   */
   unsigned dst_alignment;
   if (is_1d) {
         } else {
         }
   /* Force power-of-two alignment by extracting only the least-significant-bit */
   dst_alignment = 1 << (ffs(dst_alignment) - 1);
   /*
   * Resource base and stride pointers are aligned to 16 bytes, so that's
   * the maximum alignment we can guarantee
   */
                     if (dst_count > src_count) {
      if ((dst_type.width == 8 || dst_type.width == 16) &&
      util_is_power_of_two_or_zero(dst_type.length) &&
   dst_type.length * dst_type.width < 128) {
   /*
   * Never try to load values as 4xi8 which we will then
   * concatenate to larger vectors. This gives llvm a real
   * headache (the problem is the type legalizer (?) will
   * try to load that as 4xi8 zext to 4xi32 to fill the vector,
   * then the shuffles to concatenate are more or less impossible
   * - llvm is easily capable of generating a sequence of 32
   * pextrb/pinsrb instructions for that. Albeit it appears to
   * be fixed in llvm 4.0. So, load and concatenate with 32bit
   * width to avoid the trouble (16bit seems not as bad, llvm
   * probably recognizes the load+shuffle as only one shuffle
   * is necessary, but we can do just the same anyway).
   */
   ls_type.length = dst_type.length * dst_type.width / 32;
                  if (is_1d) {
      load_unswizzled_block(gallivm, color_type, color_ptr, stride, block_width, 1,
         for (unsigned i = dst_count / 4; i < dst_count; i++) {
            } else {
      load_unswizzled_block(gallivm, color_type, color_ptr, stride, block_width,
                        /*
   * Convert from dst/output format to src/blending format.
   *
   * This is necessary as we can only read 1 row from memory at a time,
   * so the minimum dst_count will ever be at this point is 4.
   *
   * With, for example, R8 format you can have all 16 pixels in a 128 bit
   * vector, this will take the 4 dsts and combine them into 1 src so we can
   * perform blending on all 16 pixels in that single vector at once.
   */
   if (dst_count > src_count) {
      if (ls_type.length != dst_type.length && ls_type.length == 1) {
      LLVMTypeRef elem_type = lp_build_elem_type(gallivm, ls_type);
   LLVMTypeRef ls_vec_type = LLVMVectorType(elem_type, 1);
   for (unsigned i = 0; i < dst_count; i++) {
                              if (ls_type.length != dst_type.length) {
      struct lp_type tmp_type = dst_type;
   tmp_type.length = dst_type.length * 4 / src_count;
   for (unsigned i = 0; i < src_count; i++) {
      dst[i] = LLVMBuildBitCast(builder, dst[i],
                     /*
   * Blending
   */
   /* XXX this is broken for RGB8 formats -
   * they get expanded from 12 to 16 elements (to include alpha)
   * by convert_to_blend_type then reduced to 15 instead of 12
   * by convert_from_blend_type (a simple fix though breaks A8...).
   * R16G16B16 also crashes differently however something going wrong
   * inside llvm handling npot vector sizes seemingly.
   * It seems some cleanup could be done here (like skipping conversion/blend
   * when not needed).
   */
   convert_to_blend_type(gallivm, block_size, out_format_desc, dst_type,
            /*
   * FIXME: Really should get logic ops / masks out of generic blend / row
   * format. Logic ops will definitely not work on the blend float format
   * used for SRGB here and I think OpenGL expects this to work as expected
   * (that is incoming values converted to srgb then logic op applied).
   */
   for (unsigned i = 0; i < src_count; ++i) {
      dst[i] = lp_build_blend_aos(gallivm,
                              &variant->key.blend,
   out_format,
   row_type,
   rt,
   src[i],
   has_alpha ? NULL : src_alpha[i],
   src1[i],
   has_alpha ? NULL : src1_alpha[i],
   dst[i],
            convert_from_blend_type(gallivm, block_size, out_format_desc,
            /* Split the blend rows back to memory rows */
   if (dst_count > src_count) {
               if (src_count == 1) {
      dst[1] = lp_build_extract_range(gallivm, dst[0],
                              row_type.length /= 2;
               dst[3] = lp_build_extract_range(gallivm, dst[1], row_type.length / 2,
         dst[2] = lp_build_extract_range(gallivm, dst[1], 0, row_type.length / 2);
   dst[1] = lp_build_extract_range(gallivm, dst[0], row_type.length / 2,
                  row_type.length /= 2;
               /*
   * Store blend result to memory
   */
   if (is_1d) {
      store_unswizzled_block(gallivm, color_type, color_ptr, stride, block_width, 1,
      } else {
      store_unswizzled_block(gallivm, color_type, color_ptr, stride, block_width,
                     if (do_branch) {
                  if (fpstate) {
            }
         /**
   * Generate the runtime callable function for the whole fragment pipeline.
   * Note that the function which we generate operates on a block of 16
   * pixels at at time.  The block contains 2x2 quads.  Each quad contains
   * 2x2 pixels.
   */
   static void
   generate_fragment(struct llvmpipe_context *lp,
                     {
      assert(partial_mask == RAST_WHOLE ||
            struct nir_shader *nir = shader->base.ir.nir;
   struct gallivm_state *gallivm = variant->gallivm;
   struct lp_fragment_shader_variant_key *key = &variant->key;
   struct lp_shader_input inputs[PIPE_MAX_SHADER_INPUTS];
   LLVMTypeRef fs_elem_type;
   LLVMTypeRef blend_vec_type;
   LLVMTypeRef arg_types[16];
   LLVMTypeRef func_type;
   LLVMTypeRef int32_type = LLVMInt32TypeInContext(gallivm->context);
   LLVMTypeRef int32p_type = LLVMPointerType(int32_type, 0);
   LLVMTypeRef int8_type = LLVMInt8TypeInContext(gallivm->context);
   LLVMTypeRef int8p_type = LLVMPointerType(int8_type, 0);
   LLVMValueRef context_ptr;
   LLVMValueRef resources_ptr;
   LLVMValueRef x;
   LLVMValueRef y;
   LLVMValueRef a0_ptr;
   LLVMValueRef dadx_ptr;
   LLVMValueRef dady_ptr;
   LLVMValueRef color_ptr_ptr;
   LLVMValueRef stride_ptr;
   LLVMValueRef color_sample_stride_ptr;
   LLVMValueRef depth_ptr;
   LLVMValueRef depth_stride;
   LLVMValueRef depth_sample_stride;
   LLVMValueRef mask_input;
   LLVMValueRef thread_data_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   struct lp_build_interp_soa_context interp;
   LLVMValueRef fs_mask[(16 / 4) * LP_MAX_SAMPLES];
   LLVMValueRef fs_out_color[LP_MAX_SAMPLES][PIPE_MAX_COLOR_BUFS][TGSI_NUM_CHANNELS][16 / 4];
   LLVMValueRef function;
   LLVMValueRef facing;
   const bool dual_source_blend = key->blend.rt[0].blend_enable &&
                     /* Adjust color input interpolation according to flatshade state:
   */
   nir_foreach_shader_in_variable(var, nir) {
      unsigned idx = var->data.driver_location;
   unsigned slots = nir_variable_count_slots(var, var->type);
   memcpy(&inputs[idx], &shader->inputs[idx], (sizeof inputs[0] * slots));
   for (unsigned s = 0; s < slots; s++) {
      if (inputs[idx + s].interp == LP_INTERP_COLOR)
                  /* TODO: actually pick these based on the fs and color buffer
            struct lp_type fs_type;
   memset(&fs_type, 0, sizeof fs_type);
   fs_type.floating = true;      /* floating point values */
   fs_type.sign = true;          /* values are signed */
   fs_type.norm = false;         /* values are not limited to [0,1] or [-1,1] */
   fs_type.width = 32;           /* 32-bit float */
            struct lp_type blend_type;
   memset(&blend_type, 0, sizeof blend_type);
   blend_type.floating = false; /* values are integers */
   blend_type.sign = false;     /* values are unsigned */
   blend_type.norm = true;      /* values are in [0,1] or [-1,1] */
   blend_type.width = 8;        /* 8-bit ubyte values */
            /*
   * Generate the function prototype. Any change here must be reflected in
   * lp_jit.h's lp_jit_frag_func function pointer type, and vice-versa.
                              char func_name[64];
   snprintf(func_name, sizeof(func_name), "fs_variant_%s",
            arg_types[0] = variant->jit_context_ptr_type;       /* context */
   arg_types[1] = variant->jit_resources_ptr_type;       /* context */
   arg_types[2] = int32_type;                          /* x */
   arg_types[3] = int32_type;                          /* y */
   arg_types[4] = int32_type;                          /* facing */
   arg_types[5] = LLVMPointerType(fs_elem_type, 0);    /* a0 */
   arg_types[6] = LLVMPointerType(fs_elem_type, 0);    /* dadx */
   arg_types[7] = LLVMPointerType(fs_elem_type, 0);    /* dady */
   arg_types[8] = LLVMPointerType(int8p_type, 0);  /* color */
   arg_types[9] = int8p_type;       /* depth */
   arg_types[10] = LLVMInt64TypeInContext(gallivm->context);  /* mask_input */
   arg_types[11] = variant->jit_thread_data_ptr_type;  /* per thread data */
   arg_types[12] = int32p_type;     /* stride */
   arg_types[13] = int32_type;                         /* depth_stride */
   arg_types[14] = int32p_type;     /* color sample strides */
            func_type = LLVMFunctionType(LLVMVoidTypeInContext(gallivm->context),
            function = LLVMAddFunction(gallivm->module, func_name, func_type);
                     /* XXX: need to propagate noalias down into color param now we are
   * passing a pointer-to-pointer?
   */
   for (unsigned i = 0; i < ARRAY_SIZE(arg_types); ++i)
      if (LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind)
         if (variant->gallivm->cache->data_size)
            context_ptr  = LLVMGetParam(function, 0);
   resources_ptr  = LLVMGetParam(function, 1);
   x            = LLVMGetParam(function, 2);
   y            = LLVMGetParam(function, 3);
   facing       = LLVMGetParam(function, 4);
   a0_ptr       = LLVMGetParam(function, 5);
   dadx_ptr     = LLVMGetParam(function, 6);
   dady_ptr     = LLVMGetParam(function, 7);
   color_ptr_ptr = LLVMGetParam(function, 8);
   depth_ptr    = LLVMGetParam(function, 9);
   mask_input   = LLVMGetParam(function, 10);
   thread_data_ptr  = LLVMGetParam(function, 11);
   stride_ptr   = LLVMGetParam(function, 12);
   depth_stride = LLVMGetParam(function, 13);
   color_sample_stride_ptr = LLVMGetParam(function, 14);
            lp_build_name(context_ptr, "context");
   lp_build_name(resources_ptr, "resources");
   lp_build_name(x, "x");
   lp_build_name(y, "y");
   lp_build_name(a0_ptr, "a0");
   lp_build_name(dadx_ptr, "dadx");
   lp_build_name(dady_ptr, "dady");
   lp_build_name(color_ptr_ptr, "color_ptr_ptr");
   lp_build_name(depth_ptr, "depth");
   lp_build_name(mask_input, "mask_input");
   lp_build_name(thread_data_ptr, "thread_data");
   lp_build_name(stride_ptr, "stride_ptr");
   lp_build_name(depth_stride, "depth_stride");
   lp_build_name(color_sample_stride_ptr, "color_sample_stride_ptr");
            /*
   * Function body
            block = LLVMAppendBasicBlockInContext(gallivm->context, function, "entry");
   builder = gallivm->builder;
   assert(builder);
            /*
   * Must not count ps invocations if there's a null shader.
   * (It would be ok to count with null shader if there's d/s tests,
   * but only if there's d/s buffers too, which is different
   * to implicit rasterization disable which must not depend
   * on the d/s buffers.)
   * Could use popcount on mask, but pixel accuracy is not required.
   * Could disable if there's no stats query, but maybe not worth it.
   */
   if (shader->info.base.num_instructions > 1) {
      LLVMValueRef invocs, val;
   LLVMTypeRef invocs_type = LLVMInt64TypeInContext(gallivm->context);
   invocs = lp_jit_thread_data_ps_invocations(gallivm, variant->jit_thread_data_type, thread_data_ptr);
   val = LLVMBuildLoad2(builder, invocs_type, invocs, "");
   val = LLVMBuildAdd(builder, val,
                                 /* code generated texture sampling */
   struct lp_build_sampler_soa *sampler =
      lp_llvm_sampler_soa_create(lp_fs_variant_key_samplers(key),
            struct lp_build_image_soa *image =
            unsigned num_fs = 16 / fs_type.length; /* number of loops per 4x4 stamp */
   /* for 1d resources only run "upper half" of stamp */
   if (key->resource_1d)
            {
      LLVMValueRef num_loop = lp_build_const_int32(gallivm, num_fs);
   LLVMTypeRef mask_type = lp_build_int_vec_type(gallivm, fs_type);
   LLVMValueRef num_loop_samp =
         LLVMValueRef mask_store =
      lp_build_array_alloca(gallivm, mask_type,
      LLVMTypeRef flt_type = LLVMFloatTypeInContext(gallivm->context);
   LLVMValueRef glob_sample_pos =
      LLVMAddGlobal(gallivm->module,
               if (key->multisample && key->coverage_samples == 4) {
      LLVMValueRef sample_pos_arr[8];
   for (unsigned i = 0; i < 4; i++) {
      sample_pos_arr[i * 2] = LLVMConstReal(flt_type,
         sample_pos_arr[i * 2 + 1] = LLVMConstReal(flt_type,
      }
   sample_pos_array =
      LLVMConstArray(LLVMFloatTypeInContext(gallivm->context),
   } else {
      LLVMValueRef sample_pos_arr[2];
   sample_pos_arr[0] = LLVMConstReal(flt_type, 0.5);
   sample_pos_arr[1] = LLVMConstReal(flt_type, 0.5);
   sample_pos_array =
      LLVMConstArray(LLVMFloatTypeInContext(gallivm->context),
   }
            LLVMValueRef color_store[PIPE_MAX_COLOR_BUFS][TGSI_NUM_CHANNELS];
            /*
   * The shader input interpolation info is not explicitely baked in the
   * shader key, but everything it derives from (TGSI, and flatshade) is
   * already included in the shader key.
   */
   lp_build_interp_soa_init(&interp,
                           gallivm,
   nir->num_inputs,
   inputs,
   pixel_center_integer,
   key->coverage_samples,
   LLVMTypeOf(sample_pos_array),
            for (unsigned i = 0; i < num_fs; i++) {
      if (key->multisample) {
      LLVMValueRef smask_val =
                        /*
   * For multisampling, extract the per-sample mask from the
   * incoming 64-bit mask, store to the per sample mask storage. Or
   * all of them together to generate the fragment shader
   * mask. (sample shading TODO).  Take the incoming state coverage
   * mask into account.
   */
   for (unsigned s = 0; s < key->coverage_samples; s++) {
      LLVMValueRef sindexi =
         LLVMValueRef sample_mask_ptr =
      LLVMBuildGEP2(builder, mask_type, mask_store, &sindexi, 1,
      LLVMValueRef s_mask =
      generate_quad_mask(gallivm, fs_type,
      LLVMValueRef smask_bit =
      LLVMBuildAnd(builder, smask_val,
      LLVMValueRef cmp =
      LLVMBuildICmp(builder, LLVMIntNE, smask_bit,
                     s_mask = LLVMBuildAnd(builder, s_mask, smask_bit, "");
         } else {
      LLVMValueRef mask;
   LLVMValueRef indexi = lp_build_const_int32(gallivm, i);
                  if (partial_mask) {
      mask = generate_quad_mask(gallivm, fs_type,
      } else {
         }
                  generate_fs_loop(gallivm,
                  shader, key,
   builder,
   fs_type,
   variant->jit_context_type,
   context_ptr,
   variant->jit_resources_type,
   resources_ptr,
   LLVMTypeOf(sample_pos_array),
   glob_sample_pos,
   num_loop,
   &interp,
   sampler,
   image,
   mask_type,
   mask_store, /* output */
   color_store,
   depth_ptr,
   depth_stride,
   depth_sample_stride,
   color_ptr_ptr,
   stride_ptr,
               LLVMTypeRef fs_vec_type = lp_build_vec_type(gallivm, fs_type);
   for (unsigned i = 0; i < num_fs; i++) {
      LLVMValueRef ptr;
   for (unsigned s = 0; s < key->coverage_samples; s++) {
      int idx = (i + (s * num_fs));
                              for (unsigned s = 0; s < key->min_samples; s++) {
      /* This is fucked up need to reorganize things */
   int idx = s * num_fs + i;
   LLVMValueRef sindexi = lp_build_const_int32(gallivm, idx);
   for (unsigned cbuf = 0; cbuf < key->nr_cbufs; cbuf++) {
      for (unsigned chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
      ptr = LLVMBuildGEP2(builder, fs_vec_type,
                     }
   if (dual_source_blend) {
      /* only support one dual source blend target hence always use
   * output 1
   */
   for (unsigned chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
      ptr = LLVMBuildGEP2(builder, fs_vec_type,
                                       lp_bld_llvm_sampler_soa_destroy(sampler);
            /* Loop over color outputs / color buffers to do blending */
   for (unsigned cbuf = 0; cbuf < key->nr_cbufs; cbuf++) {
      if (key->cbuf_format[cbuf] != PIPE_FORMAT_NONE &&
      (key->blend.rt[cbuf].blend_enable || key->blend.logicop_enable ||
   find_output_by_frag_result(nir, FRAG_RESULT_DATA0 + cbuf) != -1)) {
   LLVMValueRef color_ptr;
   LLVMValueRef stride;
                  bool do_branch = ((key->depth.enabled
                        color_ptr = LLVMBuildLoad2(builder, int8p_type,
                        stride = LLVMBuildLoad2(builder, int32_type,
                        if (key->cbuf_nr_samples[cbuf] > 1)
      sample_stride = LLVMBuildLoad2(builder, int32_type,
                           for (unsigned s = 0; s < key->cbuf_nr_samples[cbuf]; s++) {
      unsigned mask_idx = num_fs * (key->multisample ? s : 0);
                  if (sample_stride) {
      LLVMValueRef sample_offset =
      LLVMBuildMul(builder, sample_stride,
         }
                           generate_unswizzled_blend(gallivm, cbuf, variant,
                           key->cbuf_format[cbuf],
                                 }
         static void
   dump_fs_variant_key(struct lp_fragment_shader_variant_key *key)
   {
               if (key->flatshade) {
         }
   if (key->depth_clamp)
            if (key->restrict_depth_values)
            if (key->multisample) {
      debug_printf("multisample = 1\n");
   debug_printf("coverage samples = %d\n", key->coverage_samples);
      }
   for (unsigned i = 0; i < key->nr_cbufs; ++i) {
      debug_printf("cbuf_format[%u] = %s\n", i, util_format_name(key->cbuf_format[i]));
      }
   if (key->depth.enabled || key->stencil[0].enabled) {
      debug_printf("depth.format = %s\n", util_format_name(key->zsbuf_format));
      }
   if (key->depth.enabled) {
      debug_printf("depth.func = %s\n", util_str_func(key->depth.func, true));
               for (unsigned i = 0; i < 2; ++i) {
      if (key->stencil[i].enabled) {
      debug_printf("stencil[%u].func = %s\n", i, util_str_func(key->stencil[i].func, true));
   debug_printf("stencil[%u].fail_op = %s\n", i, util_str_stencil_op(key->stencil[i].fail_op, true));
   debug_printf("stencil[%u].zpass_op = %s\n", i, util_str_stencil_op(key->stencil[i].zpass_op, true));
   debug_printf("stencil[%u].zfail_op = %s\n", i, util_str_stencil_op(key->stencil[i].zfail_op, true));
   debug_printf("stencil[%u].valuemask = 0x%x\n", i, key->stencil[i].valuemask);
                  if (key->alpha.enabled) {
                  if (key->occlusion_count) {
                  if (key->blend.logicop_enable) {
         } else if (key->blend.rt[0].blend_enable) {
      debug_printf("blend.rgb_func = %s\n",   util_str_blend_func  (key->blend.rt[0].rgb_func, true));
   debug_printf("blend.rgb_src_factor = %s\n",   util_str_blend_factor(key->blend.rt[0].rgb_src_factor, true));
   debug_printf("blend.rgb_dst_factor = %s\n",   util_str_blend_factor(key->blend.rt[0].rgb_dst_factor, true));
   debug_printf("blend.alpha_func = %s\n",       util_str_blend_func  (key->blend.rt[0].alpha_func, true));
   debug_printf("blend.alpha_src_factor = %s\n", util_str_blend_factor(key->blend.rt[0].alpha_src_factor, true));
      }
   debug_printf("blend.colormask = 0x%x\n", key->blend.rt[0].colormask);
   if (key->blend.alpha_to_coverage) {
         }
   for (unsigned i = 0; i < key->nr_samplers; ++i) {
      const struct lp_sampler_static_state *samplers = lp_fs_variant_key_samplers(key);
   const struct lp_static_sampler_state *sampler = &samplers[i].sampler_state;
   debug_printf("sampler[%u] = \n", i);
   debug_printf("  .wrap = %s %s %s\n",
               util_str_tex_wrap(sampler->wrap_s, true),
   debug_printf("  .min_img_filter = %s\n",
         debug_printf("  .min_mip_filter = %s\n",
         debug_printf("  .mag_img_filter = %s\n",
         if (sampler->compare_mode != PIPE_TEX_COMPARE_NONE)
         debug_printf("  .normalized_coords = %u\n", sampler->normalized_coords);
   debug_printf("  .min_max_lod_equal = %u\n", sampler->min_max_lod_equal);
   debug_printf("  .lod_bias_non_zero = %u\n", sampler->lod_bias_non_zero);
   debug_printf("  .apply_min_lod = %u\n", sampler->apply_min_lod);
   debug_printf("  .apply_max_lod = %u\n", sampler->apply_max_lod);
   debug_printf("  .reduction_mode = %u\n", sampler->reduction_mode);
      }
   for (unsigned i = 0; i < key->nr_sampler_views; ++i) {
      const struct lp_sampler_static_state *samplers = lp_fs_variant_key_samplers(key);
   const struct lp_static_texture_state *texture = &samplers[i].texture_state;
   debug_printf("texture[%u] = \n", i);
   debug_printf("  .format = %s\n",
         debug_printf("  .target = %s\n",
         debug_printf("  .level_zero_only = %u\n",
         debug_printf("  .pot = %u %u %u\n",
                  }
   struct lp_image_static_state *images = lp_fs_variant_key_images(key);
   for (unsigned i = 0; i < key->nr_images; ++i) {
      const struct lp_static_texture_state *image = &images[i].image_state;
   debug_printf("image[%u] = \n", i);
   debug_printf("  .format = %s\n",
         debug_printf("  .target = %s\n",
         debug_printf("  .level_zero_only = %u\n",
         debug_printf("  .pot = %u %u %u\n",
                     }
         const char *
   lp_debug_fs_kind(enum lp_fs_kind kind)
   {
      switch (kind) {
   case LP_FS_KIND_GENERAL:
         case LP_FS_KIND_BLIT_RGBA:
         case LP_FS_KIND_BLIT_RGB1:
         case LP_FS_KIND_AERO_MINIFICATION:
         case LP_FS_KIND_LLVM_LINEAR:
         default:
            }
         void
   lp_debug_fs_variant(struct lp_fragment_shader_variant *variant)
   {
      debug_printf("llvmpipe: Fragment shader #%u variant #%u:\n",
         nir_print_shader(variant->shader->base.ir.nir, stderr);
   dump_fs_variant_key(&variant->key);
   debug_printf("variant->opaque = %u\n", variant->opaque);
   debug_printf("variant->potentially_opaque = %u\n", variant->potentially_opaque);
   debug_printf("variant->blit = %u\n", variant->blit);
   debug_printf("shader->kind = %s\n", lp_debug_fs_kind(variant->shader->kind));
      }
         static void
   lp_fs_get_ir_cache_key(struct lp_fragment_shader_variant *variant,
         {
      struct blob blob = { 0 };
   unsigned ir_size;
            blob_init(&blob);
   nir_serialize(&blob, variant->shader->base.ir.nir, true);
   ir_binary = blob.data;
            struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);
   _mesa_sha1_update(&ctx, &variant->key, variant->shader->variant_key_size);
   _mesa_sha1_update(&ctx, ir_binary, ir_size);
               }
         /**
   * Generate a new fragment shader variant from the shader code and
   * other state indicated by the key.
   */
   static struct lp_fragment_shader_variant *
   generate_variant(struct llvmpipe_context *lp,
               {
      struct nir_shader *nir = shader->base.ir.nir;
   struct lp_fragment_shader_variant *variant =
         if (!variant)
                     pipe_reference_init(&variant->reference, 1);
                     struct llvmpipe_screen *screen = llvmpipe_screen(lp->pipe.screen);
   struct lp_cached_code cached = { 0 };
   unsigned char ir_sha1_cache_key[20];
   bool needs_caching = false;
   if (shader->base.ir.nir) {
               lp_disk_cache_find_shader(screen, &cached, ir_sha1_cache_key);
   if (!cached.data_size)
               char module_name[64];
   snprintf(module_name, sizeof(module_name), "fs%u_variant%u",
         variant->gallivm = gallivm_create(module_name, lp->context, &cached);
   if (!variant->gallivm) {
      FREE(variant);
               variant->list_item_global.base = variant;
   variant->list_item_local.base = variant;
            /*
   * Determine whether we are touching all channels in the color buffer.
   */
   const struct util_format_description *cbuf0_format_desc = NULL;
   bool fullcolormask = false;
   if (key->nr_cbufs == 1) {
      cbuf0_format_desc = util_format_description(key->cbuf_format[0]);
   fullcolormask = util_format_colormask_full(cbuf0_format_desc,
               /* The scissor is ignored here as only tiles inside the scissoring
   * rectangle will refer to this.
   */
   const bool no_kill =
         fullcolormask &&
   !key->stencil[0].enabled &&
   !key->alpha.enabled &&
   !key->multisample &&
   !key->blend.alpha_to_coverage &&
   !key->depth.enabled &&
   !nir->info.fs.uses_discard &&
            variant->opaque =
         no_kill &&
   !key->blend.logicop_enable &&
            variant->potentially_opaque =
         no_kill &&
   !key->blend.logicop_enable &&
   key->blend.rt[0].blend_enable &&
   key->blend.rt[0].rgb_func == PIPE_BLEND_ADD &&
   key->blend.rt[0].rgb_dst_factor == PIPE_BLENDFACTOR_INV_SRC_ALPHA &&
   key->blend.rt[0].alpha_func == key->blend.rt[0].rgb_func &&
   key->blend.rt[0].alpha_dst_factor == key->blend.rt[0].rgb_dst_factor &&
   shader->base.type == PIPE_SHADER_IR_TGSI &&
   /*
   * FIXME: for NIR, all of the fields of info.xxx (except info.base)
   * are zeros, hence shader analysis (here and elsewhere) using these
   * bits cannot work and will silently fail (cbuf is the only pointer
   * field, hence causing a crash).
   */
            /* We only care about opaque blits for now */
   if (variant->opaque &&
      (shader->kind == LP_FS_KIND_BLIT_RGBA ||
   shader->kind == LP_FS_KIND_BLIT_RGB1)) {
   const struct lp_sampler_static_state *samp0 =
                  const enum pipe_format texture_format = samp0->texture_state.format;
   const enum pipe_texture_target target = samp0->texture_state.target;
   const unsigned min_img_filter = samp0->sampler_state.min_img_filter;
            unsigned min_mip_filter;
   if (samp0->texture_state.level_zero_only) {
         } else {
                  if (target == PIPE_TEXTURE_2D &&
      min_img_filter == PIPE_TEX_FILTER_NEAREST &&
   mag_img_filter == PIPE_TEX_FILTER_NEAREST &&
   min_mip_filter == PIPE_TEX_MIPFILTER_NONE &&
   ((texture_format &&
      util_is_format_compatible(util_format_description(texture_format),
      (shader->kind == LP_FS_KIND_BLIT_RGB1 &&
      (texture_format == PIPE_FORMAT_B8G8R8A8_UNORM ||
   texture_format == PIPE_FORMAT_B8G8R8X8_UNORM) &&
   (key->cbuf_format[0] == PIPE_FORMAT_B8G8R8A8_UNORM ||
                     /* Determine whether this shader + pipeline state is a candidate for
   * the linear path.
   */
   const bool linear_pipeline =
         !key->stencil[0].enabled &&
   !key->depth.enabled &&
   !nir->info.fs.uses_discard &&
   !key->blend.logicop_enable &&
   (key->cbuf_format[0] == PIPE_FORMAT_B8G8R8A8_UNORM ||
   key->cbuf_format[0] == PIPE_FORMAT_B8G8R8X8_UNORM ||
                     if ((LP_DEBUG & DEBUG_FS) || (gallivm_debug & GALLIVM_DEBUG_IR)) {
                                    if (variant->jit_function[RAST_EDGE_TEST] == NULL)
            if (variant->jit_function[RAST_WHOLE] == NULL) {
      if (variant->opaque) {
      /* Specialized shader, which doesn't need to read the color buffer. */
                  if (linear_pipeline) {
      /* Currently keeping both the old fastpaths and new linear path
   * active.  The older code is still somewhat faster for the cases
   * it covers.
   *
   * XXX: consider restricting this to aero-mode only.
   */
   if (fullcolormask &&
      !key->alpha.enabled &&
   !key->blend.alpha_to_coverage) {
               /* If the original fastpath doesn't cover this variant, try the new
   * code:
   */
   if (variant->jit_linear == NULL) {
      if (shader->kind == LP_FS_KIND_BLIT_RGBA ||
      shader->kind == LP_FS_KIND_BLIT_RGB1 ||
   shader->kind == LP_FS_KIND_LLVM_LINEAR) {
            } else {
      if (LP_DEBUG & DEBUG_LINEAR) {
      lp_debug_fs_variant(variant);
                  /*
   * Compile everything
                              if (variant->function[RAST_EDGE_TEST]) {
      variant->jit_function[RAST_EDGE_TEST] = (lp_jit_frag_func)
                     if (variant->function[RAST_WHOLE]) {
      variant->jit_function[RAST_WHOLE] = (lp_jit_frag_func)
      gallivm_jit_function(variant->gallivm,
   } else if (!variant->jit_function[RAST_WHOLE]) {
      variant->jit_function[RAST_WHOLE] = (lp_jit_frag_func)
               if (linear_pipeline) {
      if (variant->linear_function) {
      variant->jit_linear_llvm = (lp_jit_linear_llvm_func)
               /*
   * This must be done after LLVM compilation, as it will call the JIT'ed
   * code to determine active inputs.
   */
               if (needs_caching) {
                              }
         static void *
   llvmpipe_create_fs_state(struct pipe_context *pipe,
         {
               struct lp_fragment_shader *shader = CALLOC_STRUCT(lp_fragment_shader);
   if (!shader)
            pipe_reference_init(&shader->reference, 1);
   shader->no = fs_no++;
                     if (templ->type == PIPE_SHADER_IR_TGSI) {
         } else {
                  /* lower FRAG_RESULT_COLOR -> DATA[0-7] to correctly handle unused attachments */
   nir_shader *nir = shader->base.ir.nir;
            nir_shader_gather_info(nir, nir_shader_get_entrypoint(nir));
   nir_tgsi_scan_shader(nir, &shader->info.base, true);
                     shader->draw_data = draw_create_fragment_shader(llvmpipe->draw, templ);
   if (shader->draw_data == NULL) {
      FREE(shader);
               const int nr_samplers = BITSET_LAST_BIT(nir->info.samplers_used);
   const int nr_sampler_views = BITSET_LAST_BIT(nir->info.textures_used);
            shader->variant_key_size = lp_fs_variant_key_size(MAX2(nr_samplers,
                  nir_foreach_shader_in_variable(var, nir) {
      unsigned idx = var->data.driver_location;
            if (var->data.centroid)
         if (var->data.sample)
            enum glsl_base_type base_type =
         switch (var->data.interpolation) {
   case INTERP_MODE_NONE:
      if (glsl_base_type_is_integer(base_type) || var->data.per_primitive) {
      shader->inputs[idx].interp = LP_INTERP_CONSTANT;
      }
   if (var->data.location == VARYING_SLOT_COL0 ||
      var->data.location == VARYING_SLOT_COL1) {
   shader->inputs[idx].interp = LP_INTERP_COLOR;
      }
      case INTERP_MODE_SMOOTH:
      shader->inputs[idx].interp = LP_INTERP_PERSPECTIVE;
      case INTERP_MODE_NOPERSPECTIVE:
      shader->inputs[idx].interp = LP_INTERP_LINEAR;
      case INTERP_MODE_FLAT:
      shader->inputs[idx].interp = LP_INTERP_CONSTANT;
               /* XXX this is a completely pointless index map... */
   shader->inputs[idx].src_index = idx + 1;
   if (var->data.location == VARYING_SLOT_FACE)
         else if (var->data.location == VARYING_SLOT_POS) {
      shader->inputs[idx].src_index = 0;
               shader->inputs[idx].usage_mask = shader->info.base.input_usage_mask[idx];
   for (unsigned s = 1; s < slots; s++) {
      shader->inputs[idx + s] = shader->inputs[idx];
   shader->inputs[idx + s].src_index = idx + s + 1;
                              }
         static void
   llvmpipe_bind_fs_state(struct pipe_context *pipe, void *fs)
   {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_fragment_shader *lp_fs = (struct lp_fragment_shader *)fs;
   if (llvmpipe->fs == lp_fs)
            draw_bind_fragment_shader(llvmpipe->draw,
                     /* invalidate the setup link, NEW_FS will make it update */
   lp_setup_set_fs_variant(llvmpipe->setup, NULL);
      }
         /**
   * Remove shader variant from two lists: the shader's variant list
   * and the context's variant list.
   */
   static void
   llvmpipe_remove_shader_variant(struct llvmpipe_context *lp,
         {
      if ((LP_DEBUG & DEBUG_FS) || (gallivm_debug & GALLIVM_DEBUG_IR)) {
      debug_printf("llvmpipe: del fs #%u var %u v created %u v cached %u "
               "v total cached %u inst %u total inst %u\n",
   variant->shader->no, variant->no,
               /* remove from shader's list */
   list_del(&variant->list_item_local.list);
            /* remove from context's list */
   list_del(&variant->list_item_global.list);
   lp->nr_fs_variants--;
      }
         void
   llvmpipe_destroy_shader_variant(struct llvmpipe_context *lp,
         {
      gallivm_destroy(variant->gallivm);
   lp_fs_reference(lp, &variant->shader, NULL);
      }
         void
   llvmpipe_destroy_fs(struct llvmpipe_context *llvmpipe,
         {
      /* Delete draw module's data */
                     ralloc_free(shader->base.ir.nir);
   assert(shader->variants_cached == 0);
      }
         static void
   llvmpipe_delete_fs_state(struct pipe_context *pipe, void *fs)
   {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_fragment_shader *shader = fs;
            /* Delete all the variants */
   LIST_FOR_EACH_ENTRY_SAFE(li, next, &shader->variants.list, list) {
      struct lp_fragment_shader_variant *variant;
   variant = li->base;
   llvmpipe_remove_shader_variant(llvmpipe, li->base);
                  }
         static void
   llvmpipe_set_constant_buffer(struct pipe_context *pipe,
                     {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
            assert(shader < PIPE_SHADER_MESH_TYPES);
            /* note: reference counting */
   util_copy_constant_buffer(&llvmpipe->constants[shader][index], cb,
            /* user_buffer is only valid until the next set_constant_buffer (at most,
   * possibly until shader deletion), so we need to upload it now to make
   * sure it doesn't get updated/freed out from under us.
   */
   if (constants->user_buffer) {
      u_upload_data(llvmpipe->pipe.const_uploader, 0, constants->buffer_size,
            }
   if (constants->buffer) {
      if (!(constants->buffer->bind & PIPE_BIND_CONSTANT_BUFFER)) {
      debug_printf("Illegal set constant without bind flag\n");
      }
               switch (shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL: {
               const uint8_t *data = NULL;
   if (constants->buffer) {
      data = (uint8_t *) llvmpipe_resource_data(constants->buffer)
               draw_set_mapped_constant_buffer(llvmpipe->draw, shader,
            }
   case PIPE_SHADER_COMPUTE:
      llvmpipe->cs_dirty |= LP_CSNEW_CONSTANTS;
      case PIPE_SHADER_FRAGMENT:
      llvmpipe->dirty |= LP_NEW_FS_CONSTANTS;
      case PIPE_SHADER_TASK:
      llvmpipe->dirty |= LP_NEW_TASK_CONSTANTS;
      case PIPE_SHADER_MESH:
      llvmpipe->dirty |= LP_NEW_MESH_CONSTANTS;
      default:
      unreachable("Illegal shader type");
         }
         static void
   llvmpipe_set_shader_buffers(struct pipe_context *pipe,
                           {
               unsigned i, idx;
   for (i = start_slot, idx = 0; i < start_slot + count; i++, idx++) {
                        if (buffer && buffer->buffer) {
      bool read_only = !(writable_bitmask & (1 << idx));
   llvmpipe_flush_resource(pipe, buffer->buffer, 0, read_only, false,
               switch (shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL: {
      const unsigned size = buffer ? buffer->buffer_size : 0;
   const uint8_t *data = NULL;
   if (buffer && buffer->buffer)
         if (data)
         draw_set_mapped_shader_buffer(llvmpipe->draw, shader,
            }
   case PIPE_SHADER_COMPUTE:
      llvmpipe->cs_dirty |= LP_CSNEW_SSBOS;
      case PIPE_SHADER_TASK:
      llvmpipe->dirty |= LP_NEW_TASK_SSBOS;
      case PIPE_SHADER_MESH:
      llvmpipe->dirty |= LP_NEW_MESH_SSBOS;
      case PIPE_SHADER_FRAGMENT:
      llvmpipe->fs_ssbo_write_mask &= ~(((1 << count) - 1) << start_slot);
   llvmpipe->fs_ssbo_write_mask |= writable_bitmask << start_slot;
   llvmpipe->dirty |= LP_NEW_FS_SSBOS;
      default:
      unreachable("Illegal shader type");
            }
         static void
   llvmpipe_set_shader_images(struct pipe_context *pipe,
                     {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
            draw_flush(llvmpipe->draw);
   for (i = start_slot, idx = 0; i < start_slot + count; i++, idx++) {
                        if (image && image->resource) {
      bool read_only = !(image->access & PIPE_IMAGE_ACCESS_WRITE);
   llvmpipe_flush_resource(pipe, image->resource, 0, read_only, false,
                  llvmpipe->num_images[shader] = start_slot + count;
   switch (shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL:
      draw_set_images(llvmpipe->draw, shader, llvmpipe->images[shader],
            case PIPE_SHADER_COMPUTE:
      llvmpipe->cs_dirty |= LP_CSNEW_IMAGES;
      case PIPE_SHADER_FRAGMENT:
      llvmpipe->dirty |= LP_NEW_FS_IMAGES;
      case PIPE_SHADER_TASK:
      llvmpipe->dirty |= LP_NEW_TASK_IMAGES;
      case PIPE_SHADER_MESH:
      llvmpipe->dirty |= LP_NEW_MESH_IMAGES;
      default:
      unreachable("Illegal shader type");
               if (unbind_num_trailing_slots) {
      llvmpipe_set_shader_images(pipe, shader, start_slot + count,
         }
         /**
   * Return the blend factor equivalent to a destination alpha of one.
   */
   static inline enum pipe_blendfactor
   force_dst_alpha_one(enum pipe_blendfactor factor, bool clamped_zero)
   {
      switch (factor) {
   case PIPE_BLENDFACTOR_DST_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_ALPHA:
         case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      if (clamped_zero)
         else
      default:
            }
         /**
   * We need to generate several variants of the fragment pipeline to match
   * all the combinations of the contributing state atoms.
   *
   * TODO: there is actually no reason to tie this to context state -- the
   * generated code could be cached globally in the screen.
   */
   static struct lp_fragment_shader_variant_key *
   make_variant_key(struct llvmpipe_context *lp,
               {
      struct lp_fragment_shader_variant_key *key =
                           if (lp->framebuffer.zsbuf) {
      const enum pipe_format zsbuf_format = lp->framebuffer.zsbuf->format;
   const struct util_format_description *zsbuf_desc =
            if (lp->depth_stencil->depth_enabled &&
      util_format_has_depth(zsbuf_desc)) {
   key->zsbuf_format = zsbuf_format;
   key->depth.enabled = lp->depth_stencil->depth_enabled;
   key->depth.writemask = lp->depth_stencil->depth_writemask;
      }
   if (lp->depth_stencil->stencil[0].enabled &&
      util_format_has_stencil(zsbuf_desc)) {
   key->zsbuf_format = zsbuf_format;
   memcpy(&key->stencil, &lp->depth_stencil->stencil,
      }
   if (llvmpipe_resource_is_1d(lp->framebuffer.zsbuf->texture)) {
         }
   key->zsbuf_nr_samples =
            /*
   * Restrict depth values if the API is clamped (GL, VK with ext)
   * for non float Z buffer
   */
   key->restrict_depth_values =
      !(lp->rasterizer->unclamped_fragment_depth_values &&
            /*
   * Propagate the depth clamp setting from the rasterizer state.
   */
            /* alpha test only applies if render buffer 0 is non-integer
   * (or does not exist)
   */
   if (!lp->framebuffer.nr_cbufs ||
      !lp->framebuffer.cbufs[0] ||
   !util_format_is_pure_integer(lp->framebuffer.cbufs[0]->format)) {
      }
   if (key->alpha.enabled) {
      key->alpha.func = lp->depth_stencil->alpha_func;
               key->flatshade = lp->rasterizer->flatshade;
   key->multisample = lp->rasterizer->multisample;
   key->no_ms_sample_mask_out = lp->rasterizer->no_ms_sample_mask_out;
   if (lp->active_occlusion_queries && !lp->queries_disabled) {
                           key->coverage_samples = 1;
   key->min_samples = 1;
   if (key->multisample) {
      key->coverage_samples =
         /* Per EXT_shader_framebuffer_fetch spec:
   *
   *   "1. How is framebuffer data treated during multisample rendering?
   *
   *    RESOLVED: Reading the value of gl_LastFragData produces a
   *    different result for each sample. This implies that all or part
   *    of the shader be run once for each sample, but has no additional
   *    implications on fragment shader input variables which may still
   *    be interpolated per pixel by the implementation."
   *
   * ARM_shader_framebuffer_fetch_depth_stencil spec further says:
   *
   *   "(1) When multisampling is enabled, does the shader run per sample?
   *
   *    RESOLVED.
   *
   *    This behavior is inherited from either
   *    EXT_shader_framebuffer_fetch or ARM_shader_framebuffer_fetch as
   *    described in the interactions section.  If neither extension is
   *    supported, the shader runs once per fragment."
   *
   * Therefore we should always enable per-sample shading when FB fetch is
   * used.
   */
   if (lp->min_samples > 1 || nir->info.fs.uses_fbfetch_output)
      }
            if (!key->blend.independent_blend_enable) {
      // we always need independent blend otherwise the fixups below won't work
   for (unsigned i = 1; i < key->nr_cbufs; i++) {
      memcpy(&key->blend.rt[i], &key->blend.rt[0],
      }
               for (unsigned i = 0; i < lp->framebuffer.nr_cbufs; i++) {
               if (lp->framebuffer.cbufs[i]) {
               key->cbuf_format[i] = format;
                  /*
   * Figure out if this is a 1d resource. Note that OpenGL allows crazy
   * mixing of 2d textures with height 1 and 1d textures, so make sure
   * we pick 1d if any cbuf or zsbuf is 1d.
   */
   if (llvmpipe_resource_is_1d(lp->framebuffer.cbufs[i]->texture)) {
                  const struct util_format_description *format_desc =
                        /*
   * Mask out color channels not present in the color buffer.
                  /*
   * Disable blend for integer formats.
   */
   if (util_format_is_pure_integer(format)) {
                  /*
   * Our swizzled render tiles always have an alpha channel, but the
   * linear render target format often does not, so force here the dst
   * alpha to be one.
   *
   * This is not a mere optimization. Wrong results will be produced if
   * the dst alpha is used, the dst format does not have alpha, and the
   * previous rendering was not flushed from the swizzled to linear
   * buffer. For example, NonPowTwo DCT.
   *
   * TODO: This should be generalized to all channels for better
   * performance, but only alpha causes correctness issues.
   *
   * Also, force rgb/alpha func/factors match, to make AoS blending
   * easier.
   */
   if (format_desc->swizzle[3] > PIPE_SWIZZLE_W ||
      format_desc->swizzle[3] == format_desc->swizzle[0]) {
   // Doesn't cover mixed snorm/unorm but can't render to them anyway
   bool clamped_zero = !util_format_is_float(format) &&
         blend_rt->rgb_src_factor =
         blend_rt->rgb_dst_factor =
         blend_rt->alpha_func       = blend_rt->rgb_func;
   blend_rt->alpha_src_factor = blend_rt->rgb_src_factor;
         } else {
      /* no color buffer for this fragment output */
   key->cbuf_format[i] = PIPE_FORMAT_NONE;
   key->cbuf_nr_samples[i] = 0;
   blend_rt->colormask = 0x0;
                  /* This value will be the same for all the variants of a given shader:
   */
   key->nr_samplers = BITSET_LAST_BIT(nir->info.samplers_used);
            struct lp_sampler_static_state *fs_sampler =
            memset(fs_sampler, 0,
            for (unsigned i = 0; i < key->nr_samplers; ++i) {
      if (BITSET_TEST(nir->info.samplers_used, i)) {
      lp_sampler_static_sampler_state(&fs_sampler[i].sampler_state,
                  /*
   * XXX If TGSI_FILE_SAMPLER_VIEW exists assume all texture opcodes
   * are dx10-style? Can't really have mixed opcodes, at least not
   * if we want to skip the holes here (without rescanning tgsi).
   */
   if (key->nr_sampler_views) {
      for (unsigned i = 0; i < key->nr_sampler_views; ++i) {
      /*
   * Note sview may exceed what's representable by file_mask.
   * This will still work, the only downside is that not actually
   * used views may be included in the shader key.
   */
   if (BITSET_TEST(nir->info.textures_used, i)) {
      lp_sampler_static_texture_state(&fs_sampler[i].texture_state,
            } else {
      key->nr_sampler_views = key->nr_samplers;
   for (unsigned i = 0; i < key->nr_sampler_views; ++i) {
      if (BITSET_TEST(nir->info.samplers_used, i)) {
      lp_sampler_static_texture_state(&fs_sampler[i].texture_state,
                     struct lp_image_static_state *lp_image = lp_fs_variant_key_images(key);
   key->nr_images = BITSET_LAST_BIT(nir->info.images_used);
   if (key->nr_images)
      memset(lp_image, 0,
      for (unsigned i = 0; i < key->nr_images; ++i) {
      if (BITSET_TEST(nir->info.images_used, i)) {
      lp_sampler_static_texture_state_image(&lp_image[i].image_state,
                  if (shader->kind == LP_FS_KIND_AERO_MINIFICATION) {
      struct lp_sampler_static_state *samp0 =
         assert(samp0);
   samp0->sampler_state.min_img_filter = PIPE_TEX_FILTER_NEAREST;
                  }
         /**
   * Update fragment shader state.  This is called just prior to drawing
   * something when some fragment-related state has changed.
   */
   void
   llvmpipe_update_fs(struct llvmpipe_context *lp)
   {
               char store[LP_FS_MAX_VARIANT_KEY_SIZE];
   const struct lp_fragment_shader_variant_key *key =
            struct lp_fragment_shader_variant *variant = NULL;
   struct lp_fs_variant_list_item *li;
   /* Search the variants for one which matches the key */
   LIST_FOR_EACH_ENTRY(li, &shader->variants.list, list) {
      if (memcmp(&li->base->key, key, shader->variant_key_size) == 0) {
      variant = li->base;
                  if (variant) {
      /* Move this variant to the head of the list to implement LRU
   * deletion of shader's when we have too many.
   */
      } else {
               if (LP_DEBUG & DEBUG_FS) {
      debug_printf("%u variants,\t%u instrs,\t%u instrs/variant\n",
                           /* First, check if we've exceeded the max number of shader variants.
   * If so, free 6.25% of them (the least recently used ones).
   */
   const unsigned variants_to_cull =
                  if (variants_to_cull ||
      lp->nr_fs_instrs >= LP_MAX_SHADER_INSTRUCTIONS) {
   if (gallivm_debug & GALLIVM_DEBUG_PERF) {
      debug_printf("Evicting FS: %u fs variants,\t%u total variants,"
               "\t%u instrs,\t%u instrs/variant\n",
               /*
   * We need to re-check lp->nr_fs_variants because an arbitrarily
   * large number of shader variants (potentially all of them) could
                  for (unsigned i = 0;
      i < variants_to_cull ||
         i++) {
   struct lp_fs_variant_list_item *item;
   if (list_is_empty(&lp->fs_variants_list.list)) {
         }
   item = list_last_entry(&lp->fs_variants_list.list,
         assert(item);
   assert(item->base);
   llvmpipe_remove_shader_variant(lp, item->base);
   struct lp_fragment_shader_variant *variant = item->base;
                  /*
   * Generate the new variant.
   */
   int64_t t0 = os_time_get();
   variant = generate_variant(lp, shader, key);
   int64_t t1 = os_time_get();
   int64_t dt = t1 - t0;
   LP_COUNT_ADD(llvm_compile_time, dt);
            /* Put the new variant into the list */
   if (variant) {
      list_add(&variant->list_item_local.list, &shader->variants.list);
   list_add(&variant->list_item_global.list, &lp->fs_variants_list.list);
   lp->nr_fs_variants++;
   lp->nr_fs_instrs += variant->nr_instrs;
                  /* Bind this variant */
      }
         void
   llvmpipe_init_fs_funcs(struct llvmpipe_context *llvmpipe)
   {
      llvmpipe->pipe.create_fs_state = llvmpipe_create_fs_state;
   llvmpipe->pipe.bind_fs_state   = llvmpipe_bind_fs_state;
   llvmpipe->pipe.delete_fs_state = llvmpipe_delete_fs_state;
   llvmpipe->pipe.set_constant_buffer = llvmpipe_set_constant_buffer;
   llvmpipe->pipe.set_shader_buffers = llvmpipe_set_shader_buffers;
      }
