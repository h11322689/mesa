   /*
   * Copyright 2022 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      /* This lowers image and texture opcodes to typed buffer opcodes (equivalent to image buffers)
   * for some CDNA chips. Sampler buffers and image buffers are not lowered.
   *
   * Only the subset of opcodes and states that is used by VAAPI and OpenMAX is lowered.
   * That means CLAMP_TO_EDGE is always used. Only level 0 can be accessed. The minification
   * and magnification filter settings are assumed to be equal.
   *
   * This uses a custom image descriptor that is used in conjunction with this pass. The first
   * 4 dwords of the descriptor contain the buffer descriptor where the format matches the image
   * format and the stride matches the pixel size, and the last 4 dwords contain parameters
   * for manual address computations and bounds checking like the pitch, the number of elements
   * per slice, etc.
   *
   */
      #include "ac_nir.h"
   #include "nir_builder.h"
   #include "amdgfxregs.h"
      static nir_def *get_field(nir_builder *b, nir_def *desc, unsigned index, unsigned mask)
   {
         }
      static unsigned get_coord_components(enum glsl_sampler_dim dim, bool is_array)
   {
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_RECT:
         case GLSL_SAMPLER_DIM_3D:
         default:
            }
      /* Lower image coordinates to a buffer element index. Return UINT_MAX if the image coordinates
   * are out of bounds.
   */
   static nir_def *lower_image_coords(nir_builder *b, nir_def *desc, nir_def *coord,
               {
      unsigned num_coord_components = get_coord_components(dim, is_array);
            /* Get coordinates. */
   nir_def *x = nir_channel(b, coord, 0);
   nir_def *y = num_coord_components >= 2 ? nir_channel(b, coord, 1) : NULL;
            if (dim == GLSL_SAMPLER_DIM_1D && is_array) {
      z = y;
               if (is_array) {
      nir_def *first_layer = get_field(b, desc, 5, 0xffff0000);
               /* Compute the buffer element index. */
   nir_def *index = x;
   if (y) {
      nir_def *pitch = nir_channel(b, desc, 6);
      }
   if (z) {
      nir_def *slice_elements = nir_channel(b, desc, 7);
               /* Determine whether the coordinates are out of bounds. */
            if (handle_out_of_bounds) {
      nir_def *width = get_field(b, desc, 4, 0xffff);
            if (y) {
      nir_def *height = get_field(b, desc, 4, 0xffff0000);
   out_of_bounds = nir_ior(b, out_of_bounds,
      }
   if (z) {
      nir_def *depth = get_field(b, desc, 5, 0xffff);
   out_of_bounds = nir_ior(b, out_of_bounds,
               /* Make the buffer opcode out of bounds by setting UINT_MAX. */
                  }
      static nir_def *emulated_image_load(nir_builder *b, unsigned num_components, unsigned bit_size,
                     {
               return nir_load_buffer_amd(b, num_components, bit_size, nir_channels(b, desc, 0xf),
                              zero, zero,
   }
      static void emulated_image_store(nir_builder *b, nir_def *desc, nir_def *coord,
               {
               nir_store_buffer_amd(b, data, nir_channels(b, desc, 0xf), zero, zero,
                        }
      /* Return the width, height, or depth for dim=0,1,2. */
   static nir_def *get_dim(nir_builder *b, nir_def *desc, unsigned dim)
   {
         }
      /* Lower txl with lod=0 to typed buffer loads. This is based on the equations in the GL spec.
   * This basically converts the tex opcode into 1 or more image_load opcodes.
   */
   static nir_def *emulated_tex_level_zero(nir_builder *b, unsigned num_components,
                     {
      const enum gl_access_qualifier access =
         const unsigned num_coord_components = get_coord_components(sampler_dim, is_array);
   const unsigned num_dim_coords = num_coord_components - is_array;
            nir_def *zero = nir_imm_int(b, 0);
   nir_def *fp_one = nir_imm_floatN_t(b, 1, bit_size);
            assert(num_coord_components <= 3);
   for (unsigned i = 0; i < num_coord_components; i++)
            /* Convert to unnormalized coordinates. */
   if (sampler_dim != GLSL_SAMPLER_DIM_RECT) {
      for (unsigned dim = 0; dim < num_dim_coords; dim++)
               /* The layer index is handled differently and ignores the filter and wrap mode. */
   if (is_array) {
      coord[array_comp] = nir_f2i32(b, nir_fround_even(b, coord[array_comp]));
   coord[array_comp] = nir_iclamp(b, coord[array_comp], zero,
               /* Determine the filter by reading the first bit of the XY_MAG_FILTER field,
   * which is 1 for linear, 0 for nearest.
   *
   * We assume that XY_MIN_FILTER and Z_FILTER are identical.
   */
   nir_def *is_nearest =
                  nir_if *if_nearest = nir_push_if(b, is_nearest);
   {
      /* Nearest filter. */
   nir_def *coord0[3] = {0};
            for (unsigned dim = 0; dim < num_dim_coords; dim++) {
                     /* Apply the wrap mode. We assume it's always CLAMP_TO_EDGE, so clamp. */
               /* Load the texel. */
   result_nearest = emulated_image_load(b, num_components, bit_size, desc,
            }
   nir_push_else(b, if_nearest);
   {
      /* Linear filter. */
   nir_def *coord0[3] = {0};
   nir_def *coord1[3] = {0};
                     for (unsigned dim = 0; dim < num_dim_coords; dim++) {
                                    /* Floor to get the top-left texel of the filter. */
   /* Add 1 to get the bottom-right texel. */
                  /* Apply the wrap mode. We assume it's always CLAMP_TO_EDGE, so clamp. */
   coord0[dim] = nir_iclamp(b, coord0[dim], zero, nir_iadd_imm(b, get_dim(b, desc, dim), -1));
               /* Load all texels for the linear filter.
   * This is 2 texels for 1D, 4 texels for 2D, and 8 texels for 3D.
   */
            for (unsigned i = 0; i < (1 << num_dim_coords); i++) {
               /* Determine whether the current texel should use channels from coord0
   * or coord1. The i-th bit of the texel index determines that.
   */
                  /* Add the layer index, which doesn't change between texels. */
                  /* Compute how much the texel contributes to the final result. */
   nir_def *texel_weight = fp_one;
   for (unsigned dim = 0; dim < num_dim_coords; dim++) {
      /* Let's see what "i" represents:
   *    Texel i=0 = 000
   *    Texel i=1 = 001
   *    Texel i=2 = 010 (2D & 3D only)
   *    Texel i=3 = 011 (2D & 3D only)
   *    Texel i=4 = 100 (3D only)
   *    Texel i=5 = 101 (3D only)
   *    Texel i=6 = 110 (3D only)
   *    Texel i=7 = 111 (3D only)
   *
   * The rightmost bit (LSB) represents the X direction, the middle bit represents
   * the Y direction, and the leftmost bit (MSB) represents the Z direction.
   * If we shift the texel index "i" by the dimension "dim", we'll get whether that
   * texel value should be multiplied by (1 - weight[dim]) or (weight[dim]).
   */
   texel_weight = nir_fmul(b, texel_weight,
                     /* Load the linear filter texel. */
   texel[i] = emulated_image_load(b, num_components, bit_size, desc,
                  /* Multiply the texel by the weight. */
               /* Sum up all weighted texels to get the final result of linear filtering. */
   result_linear = zero;
   for (unsigned i = 0; i < (1 << num_dim_coords); i++)
      }
               }
      static bool lower_image_opcodes(nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   nir_deref_instr *deref;
   enum gl_access_qualifier access;
   enum glsl_sampler_dim dim;
   bool is_array;
   nir_def *desc = NULL, *result = NULL;
            nir_def *dst = &intr->def;
            switch (intr->intrinsic) {
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
      access = nir_intrinsic_access(intr);
   dim = nir_intrinsic_image_dim(intr);
   if (dim == GLSL_SAMPLER_DIM_BUF)
         is_array = nir_intrinsic_image_array(intr);
   desc = nir_image_descriptor_amd(b, dim == GLSL_SAMPLER_DIM_BUF ? 4 : 8,
               case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
      deref = nir_instr_as_deref(intr->src[0].ssa->parent_instr);
   access = nir_deref_instr_get_variable(deref)->data.access;
   dim = glsl_get_sampler_dim(deref->type);
   if (dim == GLSL_SAMPLER_DIM_BUF)
         is_array = glsl_sampler_type_is_array(deref->type);
   desc = nir_image_deref_descriptor_amd(b, dim == GLSL_SAMPLER_DIM_BUF ? 4 : 8,
               case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_store:
      access = nir_intrinsic_access(intr);
   dim = nir_intrinsic_image_dim(intr);
   if (dim == GLSL_SAMPLER_DIM_BUF)
         is_array = nir_intrinsic_image_array(intr);
   desc = nir_bindless_image_descriptor_amd(b, dim == GLSL_SAMPLER_DIM_BUF ? 4 : 8,
               default:
               /* No other intrinsics are expected from VAAPI and OpenMAX.
   * (this lowering is only used by CDNA, which only uses those frontends)
   */
   if (strstr(intr_name, "image") == intr_name ||
      strstr(intr_name, "bindless_image") == intr_name) {
   fprintf(stderr, "Unexpected image opcode: ");
   nir_print_instr(instr, stderr);
   fprintf(stderr, "\nAborting to prevent a hang.");
      }
               switch (intr->intrinsic) {
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_bindless_image_load:
      result = emulated_image_load(b, intr->def.num_components, intr->def.bit_size,
         nir_def_rewrite_uses_after(dst, result, instr);
               case nir_intrinsic_image_store:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_bindless_image_store:
      emulated_image_store(b, desc, intr->src[1].ssa, intr->src[3].ssa, access, dim, is_array);
               default:
            } else if (instr->type == nir_instr_type_tex) {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
   nir_tex_instr *new_tex;
            nir_def *dst = &tex->def;
            switch (tex->op) {
   case nir_texop_tex:
   case nir_texop_txl:
   case nir_texop_txf:
      for (unsigned i = 0; i < tex->num_srcs; i++) {
      switch (tex->src[i].src_type) {
   case nir_tex_src_texture_deref:
   case nir_tex_src_texture_handle:
      if (tex->sampler_dim == GLSL_SAMPLER_DIM_BUF)
         new_tex = nir_tex_instr_create(b->shader, 1);
   new_tex->op = nir_texop_descriptor_amd;
   new_tex->sampler_dim = tex->sampler_dim;
   new_tex->is_array = tex->is_array;
   new_tex->texture_index = tex->texture_index;
   new_tex->sampler_index = tex->sampler_index;
   new_tex->dest_type = nir_type_int32;
   new_tex->src[0].src = nir_src_for_ssa(tex->src[i].src.ssa);
   new_tex->src[0].src_type = tex->src[i].src_type;
   nir_def_init(&new_tex->instr, &new_tex->def,
                           case nir_tex_src_sampler_deref:
   case nir_tex_src_sampler_handle:
      if (tex->sampler_dim == GLSL_SAMPLER_DIM_BUF)
         new_tex = nir_tex_instr_create(b->shader, 1);
   new_tex->op = nir_texop_sampler_descriptor_amd;
   new_tex->sampler_dim = tex->sampler_dim;
   new_tex->is_array = tex->is_array;
   new_tex->texture_index = tex->texture_index;
   new_tex->sampler_index = tex->sampler_index;
   new_tex->dest_type = nir_type_int32;
   new_tex->src[0].src = nir_src_for_ssa(tex->src[i].src.ssa);
   new_tex->src[0].src_type = tex->src[i].src_type;
   nir_def_init(&new_tex->instr, &new_tex->def,
                           case nir_tex_src_coord:
                  case nir_tex_src_projector:
   case nir_tex_src_comparator:
   case nir_tex_src_offset:
   case nir_tex_src_texture_offset:
   case nir_tex_src_sampler_offset:
                  default:;
               switch (tex->op) {
   case nir_texop_txf:
      result = emulated_image_load(b, tex->def.num_components, tex->def.bit_size,
                     nir_def_rewrite_uses_after(dst, result, instr);
               case nir_texop_tex:
   case nir_texop_txl:
      result = emulated_tex_level_zero(b, tex->def.num_components, tex->def.bit_size,
         nir_def_rewrite_uses_after(dst, result, instr);
               default:
                     case nir_texop_descriptor_amd:
   case nir_texop_sampler_descriptor_amd:
            default:
      fprintf(stderr, "Unexpected texture opcode: ");
   nir_print_instr(instr, stderr);
   fprintf(stderr, "\nAborting to prevent a hang.");
                     }
      bool ac_nir_lower_image_opcodes(nir_shader *nir)
   {
      return nir_shader_instructions_pass(nir, lower_image_opcodes,
                  }
