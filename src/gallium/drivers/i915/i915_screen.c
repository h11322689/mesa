   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
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
      #include "compiler/nir/nir.h"
   #include "draw/draw_context.h"
   #include "nir/nir_to_tgsi.h"
   #include "util/format/u_format.h"
   #include "util/format/u_format_s3tc.h"
   #include "util/os_misc.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_screen.h"
   #include "util/u_string.h"
      #include "i915_context.h"
   #include "i915_debug.h"
   #include "i915_fpc.h"
   #include "i915_public.h"
   #include "i915_reg.h"
   #include "i915_resource.h"
   #include "i915_screen.h"
   #include "i915_winsys.h"
      /*
   * Probe functions
   */
      static const char *
   i915_get_vendor(struct pipe_screen *screen)
   {
         }
      static const char *
   i915_get_device_vendor(struct pipe_screen *screen)
   {
         }
      static const char *
   i915_get_name(struct pipe_screen *screen)
   {
      static char buffer[128];
            switch (i915_screen(screen)->iws->pci_id) {
   case PCI_CHIP_I915_G:
      chipset = "915G";
      case PCI_CHIP_I915_GM:
      chipset = "915GM";
      case PCI_CHIP_I945_G:
      chipset = "945G";
      case PCI_CHIP_I945_GM:
      chipset = "945GM";
      case PCI_CHIP_I945_GME:
      chipset = "945GME";
      case PCI_CHIP_G33_G:
      chipset = "G33";
      case PCI_CHIP_Q35_G:
      chipset = "Q35";
      case PCI_CHIP_Q33_G:
      chipset = "Q33";
      case PCI_CHIP_PINEVIEW_G:
      chipset = "Pineview G";
      case PCI_CHIP_PINEVIEW_M:
      chipset = "Pineview M";
      default:
      chipset = "unknown";
               snprintf(buffer, sizeof(buffer), "i915 (chipset: %s)", chipset);
      }
      static const nir_shader_compiler_options i915_compiler_options = {
      .fdot_replicates = true,
   .fuse_ffma32 = true,
   .lower_bitops = true, /* required for !CAP_INTEGERS nir_to_tgsi */
   .lower_extract_byte = true,
   .lower_extract_word = true,
   .lower_fdiv = true,
   .lower_fdph = true,
   .lower_flrp32 = true,
   .lower_fmod = true,
   .lower_sincos = true,
   .lower_uniforms_to_ubo = true,
   .lower_vector_cmp = true,
   .use_interpolated_input_intrinsics = true,
   .force_indirect_unrolling = nir_var_all,
   .force_indirect_unrolling_sampler = true,
   .max_unroll_iterations = 32,
   .no_integers = true,
      };
      static const struct nir_shader_compiler_options gallivm_nir_options = {
      .fdot_replicates = true,
   .lower_bitops = true, /* required for !CAP_INTEGERS nir_to_tgsi */
   .lower_scmp = true,
   .lower_flrp32 = true,
   .lower_flrp64 = true,
   .lower_fsat = true,
   .lower_bitfield_insert = true,
   .lower_bitfield_extract = true,
   .lower_fdph = true,
   .lower_ffma16 = true,
   .lower_ffma32 = true,
   .lower_ffma64 = true,
   .lower_fmod = true,
   .lower_hadd = true,
   .lower_uadd_sat = true,
   .lower_usub_sat = true,
   .lower_iadd_sat = true,
   .lower_ldexp = true,
   .lower_pack_snorm_2x16 = true,
   .lower_pack_snorm_4x8 = true,
   .lower_pack_unorm_2x16 = true,
   .lower_pack_unorm_4x8 = true,
   .lower_pack_half_2x16 = true,
   .lower_pack_split = true,
   .lower_unpack_snorm_2x16 = true,
   .lower_unpack_snorm_4x8 = true,
   .lower_unpack_unorm_2x16 = true,
   .lower_unpack_unorm_4x8 = true,
   .lower_unpack_half_2x16 = true,
   .lower_extract_byte = true,
   .lower_extract_word = true,
   .lower_uadd_carry = true,
   .lower_usub_borrow = true,
   .lower_mul_2x32_64 = true,
   .lower_ifind_msb = true,
   .max_unroll_iterations = 32,
   .use_interpolated_input_intrinsics = true,
   .lower_cs_local_index_to_id = true,
   .lower_uniforms_to_ubo = true,
   .lower_vector_cmp = true,
   .lower_device_index_to_zero = true,
      };
      static const void *
   i915_get_compiler_options(struct pipe_screen *pscreen, enum pipe_shader_ir ir,
         {
      assert(ir == PIPE_SHADER_IR_NIR);
   if (shader == PIPE_SHADER_FRAGMENT)
         else
      }
      static void
   i915_optimize_nir(struct nir_shader *s)
   {
               do {
                        NIR_PASS(progress, s, nir_copy_prop);
   NIR_PASS(progress, s, nir_opt_algebraic);
   NIR_PASS(progress, s, nir_opt_constant_folding);
   NIR_PASS(progress, s, nir_opt_remove_phis);
   NIR_PASS(progress, s, nir_opt_conditional_discard);
   NIR_PASS(progress, s, nir_opt_dce);
   NIR_PASS(progress, s, nir_opt_dead_cf);
   NIR_PASS(progress, s, nir_opt_cse);
   NIR_PASS(progress, s, nir_opt_find_array_copies);
   NIR_PASS(progress, s, nir_opt_if,
               NIR_PASS(progress, s, nir_opt_peephole_select, ~0 /* flatten all IFs. */,
         NIR_PASS(progress, s, nir_opt_algebraic);
   NIR_PASS(progress, s, nir_opt_constant_folding);
   NIR_PASS(progress, s, nir_opt_shrink_stores, true);
   NIR_PASS(progress, s, nir_opt_shrink_vectors);
   NIR_PASS(progress, s, nir_opt_trivial_continues);
   NIR_PASS(progress, s, nir_opt_undef);
                  NIR_PASS(progress, s, nir_remove_dead_variables, nir_var_function_temp,
            /* Group texture loads together to try to avoid hitting the
   * texture indirection phase limit.
   */
      }
      static char *
   i915_check_control_flow(nir_shader *s)
   {
      if (s->info.stage == MESA_SHADER_FRAGMENT) {
      nir_function_impl *impl = nir_shader_get_entrypoint(s);
   nir_block *first = nir_start_block(impl);
            if (next) {
      switch (next->type) {
   case nir_cf_node_if:
      return "if/then statements not supported by i915 fragment shaders, "
      case nir_cf_node_loop:
      return "looping not supported i915 fragment shaders, all loops "
      default:
                           }
      static char *
   i915_finalize_nir(struct pipe_screen *pscreen, void *nir)
   {
               if (s->info.stage == MESA_SHADER_FRAGMENT)
            /* st_program.c's parameter list optimization requires that future nir
   * variants don't reallocate the uniform storage, so we have to remove
   * uniforms that occupy storage.  But we don't want to remove samplers,
   * because they're needed for YUV variant lowering.
   */
   nir_remove_dead_derefs(s);
   nir_foreach_uniform_variable_safe (var, s) {
      if (var->data.mode == nir_var_uniform &&
      (glsl_type_get_image_count(var->type) ||
                  }
                     char *msg = i915_check_control_flow(s);
   if (msg) {
      if (I915_DBG_ON(DBG_FS) && (!s->info.internal || NIR_DEBUG(PRINT_INTERNAL))) {
      mesa_logi("failing shader:");
      }
               if (s->info.stage == MESA_SHADER_FRAGMENT)
         else
      }
      static int
   i915_get_shader_param(struct pipe_screen *screen, enum pipe_shader_type shader,
         {
      switch (cap) {
   case PIPE_SHADER_CAP_SUPPORTED_IRS:
            case PIPE_SHADER_CAP_INTEGERS:
      /* mesa/st requires that this cap is the same across stages, and the FS
   * can't do ints.
   */
         /* i915 can't do these, and even if gallivm NIR can we call nir_to_tgsi
   * manually and TGSI can't.
   */
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
            case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
      /* While draw could normally handle this for the VS, the NIR lowering
   * to regs can't handle our non-native-integers, so we have to lower to
   * if ladders.
   */
         default:
                  switch (shader) {
   case PIPE_SHADER_VERTEX:
      switch (cap) {
   case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
         case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
   case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
         default:
            case PIPE_SHADER_FRAGMENT:
      /* XXX: some of these are just shader model 2.0 values, fix this! */
   switch (cap) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
         case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
         case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
         case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         case PIPE_SHADER_CAP_MAX_INPUTS:
         case PIPE_SHADER_CAP_MAX_OUTPUTS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         case PIPE_SHADER_CAP_MAX_TEMPS:
      /* 16 inter-phase temps, 3 intra-phase temps.  i915c reported 16. too. */
      case PIPE_SHADER_CAP_CONT_SUPPORTED:
   case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
         case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
   case PIPE_SHADER_CAP_SUBROUTINES:
         case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
         case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
   case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
   case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
            default:
      debug_printf("%s: Unknown cap %u.\n", __func__, cap);
      }
      default:
            }
      static int
   i915_get_param(struct pipe_screen *screen, enum pipe_cap cap)
   {
               switch (cap) {
   /* Supported features (boolean caps). */
   case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_PRIMITIVE_RESTART: /* draw module */
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_VERTEX_COLOR_CLAMPED:
   case PIPE_CAP_USER_VERTEX_BUFFERS:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
   case PIPE_CAP_TGSI_TEXCOORD:
            case PIPE_CAP_TEXTURE_TRANSFER_MODES:
   case PIPE_CAP_PCI_GROUP:
   case PIPE_CAP_PCI_BUS:
   case PIPE_CAP_PCI_DEVICE:
   case PIPE_CAP_PCI_FUNCTION:
            case PIPE_CAP_ALLOW_MAPPED_BUFFERS_DURING_EXECUTION:
            case PIPE_CAP_SHAREABLE_SHADERS:
      /* Can't expose shareable shaders because the draw shaders reference the
   * draw module's state, which is per-context.
   */
         case PIPE_CAP_MAX_GS_INVOCATIONS:
            case PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT:
            case PIPE_CAP_MAX_VIEWPORTS:
            case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
            case PIPE_CAP_GLSL_FEATURE_LEVEL:
   case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
            case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
            /* Texturing. */
   case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
         case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
         case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            /* Render targets. */
   case PIPE_CAP_MAX_RENDER_TARGETS:
            case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
            /* Fragment coordinate conventions. */
   case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
         case PIPE_CAP_ENDIANNESS:
         case PIPE_CAP_MAX_VARYINGS:
            case PIPE_CAP_NIR_IMAGES_AS_DEREF:
            case PIPE_CAP_VENDOR_ID:
         case PIPE_CAP_DEVICE_ID:
         case PIPE_CAP_ACCELERATED:
         case PIPE_CAP_VIDEO_MEMORY: {
      /* Once a batch uses more than 75% of the maximum mappable size, we
   * assume that there's some fragmentation, and we start doing extra
   * flushing, etc.  That's the big cliff apps will care about.
   */
   const int gpu_mappable_megabytes =
                  if (!os_get_total_physical_memory(&system_memory))
               }
   case PIPE_CAP_UMA:
            default:
            }
      static float
   i915_get_paramf(struct pipe_screen *screen, enum pipe_capf cap)
   {
      switch (cap) {
   case PIPE_CAPF_MIN_LINE_WIDTH:
   case PIPE_CAPF_MIN_LINE_WIDTH_AA:
   case PIPE_CAPF_MIN_POINT_SIZE:
   case PIPE_CAPF_MIN_POINT_SIZE_AA:
            case PIPE_CAPF_POINT_SIZE_GRANULARITY:
   case PIPE_CAPF_LINE_WIDTH_GRANULARITY:
            case PIPE_CAPF_MAX_LINE_WIDTH:
         case PIPE_CAPF_MAX_LINE_WIDTH_AA:
            case PIPE_CAPF_MAX_POINT_SIZE:
         case PIPE_CAPF_MAX_POINT_SIZE_AA:
            case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
            case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
            case PIPE_CAPF_MIN_CONSERVATIVE_RASTER_DILATE:
         case PIPE_CAPF_MAX_CONSERVATIVE_RASTER_DILATE:
         case PIPE_CAPF_CONSERVATIVE_RASTER_DILATE_GRANULARITY:
            default:
      debug_printf("%s: Unknown cap %u.\n", __func__, cap);
         }
      bool
   i915_is_format_supported(struct pipe_screen *screen, enum pipe_format format,
               {
      static const enum pipe_format tex_supported[] = {
      PIPE_FORMAT_B8G8R8A8_UNORM, PIPE_FORMAT_B8G8R8A8_SRGB,
   PIPE_FORMAT_B8G8R8X8_UNORM, PIPE_FORMAT_R8G8B8A8_UNORM,
   PIPE_FORMAT_R8G8B8X8_UNORM, PIPE_FORMAT_B4G4R4A4_UNORM,
   PIPE_FORMAT_B5G6R5_UNORM, PIPE_FORMAT_B5G5R5A1_UNORM,
   PIPE_FORMAT_B10G10R10A2_UNORM, PIPE_FORMAT_L8_UNORM, PIPE_FORMAT_A8_UNORM,
   PIPE_FORMAT_I8_UNORM, PIPE_FORMAT_L8A8_UNORM, PIPE_FORMAT_UYVY,
   PIPE_FORMAT_YUYV,
   /* XXX why not?
   PIPE_FORMAT_Z16_UNORM, */
   PIPE_FORMAT_DXT1_RGB, PIPE_FORMAT_DXT1_SRGB, PIPE_FORMAT_DXT1_RGBA,
   PIPE_FORMAT_DXT1_SRGBA, PIPE_FORMAT_DXT3_RGBA, PIPE_FORMAT_DXT3_SRGBA,
   PIPE_FORMAT_DXT5_RGBA, PIPE_FORMAT_DXT5_SRGBA, PIPE_FORMAT_Z24X8_UNORM,
   PIPE_FORMAT_FXT1_RGB, PIPE_FORMAT_FXT1_RGBA,
      };
   static const enum pipe_format render_supported[] = {
      PIPE_FORMAT_B8G8R8A8_UNORM, PIPE_FORMAT_B8G8R8X8_UNORM,
   PIPE_FORMAT_R8G8B8A8_UNORM, PIPE_FORMAT_R8G8B8X8_UNORM,
   PIPE_FORMAT_B5G6R5_UNORM,   PIPE_FORMAT_B5G5R5A1_UNORM,
   PIPE_FORMAT_B4G4R4A4_UNORM, PIPE_FORMAT_B10G10R10A2_UNORM,
   PIPE_FORMAT_L8_UNORM,       PIPE_FORMAT_A8_UNORM,
      };
   static const enum pipe_format depth_supported[] = {
      /* XXX why not?
   PIPE_FORMAT_Z16_UNORM, */
   PIPE_FORMAT_Z24X8_UNORM, PIPE_FORMAT_Z24_UNORM_S8_UINT,
      };
   const enum pipe_format *list;
            if (sample_count > 1)
            if (MAX2(1, sample_count) != MAX2(1, storage_sample_count))
            if (tex_usage & PIPE_BIND_DEPTH_STENCIL)
         else if (tex_usage & PIPE_BIND_RENDER_TARGET)
         else if (tex_usage & PIPE_BIND_SAMPLER_VIEW)
         else
            for (i = 0; list[i] != PIPE_FORMAT_NONE; i++) {
      if (list[i] == format)
                  }
      /*
   * Fence functions
   */
      static void
   i915_fence_reference(struct pipe_screen *screen, struct pipe_fence_handle **ptr,
         {
                  }
      static bool
   i915_fence_finish(struct pipe_screen *screen, struct pipe_context *ctx,
         {
               if (!timeout)
               }
      /*
   * Generic functions
   */
      static void
   i915_destroy_screen(struct pipe_screen *screen)
   {
               if (is->iws)
               }
      static int
   i915_screen_get_fd(struct pipe_screen *screen)
   {
                  }
      /**
   * Create a new i915_screen object
   */
   struct pipe_screen *
   i915_screen_create(struct i915_winsys *iws)
   {
               if (!is)
            switch (iws->pci_id) {
   case PCI_CHIP_I915_G:
   case PCI_CHIP_I915_GM:
      is->is_i945 = false;
         case PCI_CHIP_I945_G:
   case PCI_CHIP_I945_GM:
   case PCI_CHIP_I945_GME:
   case PCI_CHIP_G33_G:
   case PCI_CHIP_Q33_G:
   case PCI_CHIP_Q35_G:
   case PCI_CHIP_PINEVIEW_G:
   case PCI_CHIP_PINEVIEW_M:
      is->is_i945 = true;
         default:
      debug_printf("%s: unknown pci id 0x%x, cannot create screen\n", __func__,
         FREE(is);
                                 is->base.get_name = i915_get_name;
   is->base.get_vendor = i915_get_vendor;
   is->base.get_device_vendor = i915_get_device_vendor;
   is->base.get_screen_fd = i915_screen_get_fd;
   is->base.get_param = i915_get_param;
   is->base.get_shader_param = i915_get_shader_param;
   is->base.get_paramf = i915_get_paramf;
   is->base.get_compiler_options = i915_get_compiler_options;
   is->base.finalize_nir = i915_finalize_nir;
                     is->base.fence_reference = i915_fence_reference;
                                 }
