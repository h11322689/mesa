   /*
   * Copyright © 2022 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      /* Implement query_size, query_levels, and query_samples by extracting the information from
   * descriptors. This is expected to be faster than image_resinfo.
   */
      #include "ac_nir.h"
   #include "nir_builder.h"
   #include "amdgfxregs.h"
      static nir_def *get_field(nir_builder *b, nir_def *desc, unsigned index, unsigned mask)
   {
         }
      static nir_def *handle_null_desc(nir_builder *b, nir_def *desc, nir_def *value)
   {
      nir_def *is_null = nir_ieq_imm(b, nir_channel(b, desc, 1), 0);
      }
      static nir_def *query_samples(nir_builder *b, nir_def *desc, enum glsl_sampler_dim dim)
   {
               if (dim == GLSL_SAMPLER_DIM_MS) {
      /* LAST_LEVEL contains log2(num_samples). */
   samples = get_field(b, desc, 3, ~C_00A00C_LAST_LEVEL);
      } else {
                     }
      static nir_def *query_levels(nir_builder *b, nir_def *desc)
   {
      nir_def *base_level = get_field(b, desc, 3, ~C_00A00C_BASE_LEVEL);
                        }
      static nir_def *
   lower_query_size(nir_builder *b, nir_def *desc, nir_src *lod,
         {
      if (dim == GLSL_SAMPLER_DIM_BUF) {
               if (gfx_level == GFX8) {
      /* On GFX8, the descriptor contains the size in bytes,
   * but TXQ must return the size in elements.
   * The stride is always non-zero for resources using TXQ.
   * Divide the size by the stride.
   */
      }
               /* Cube textures return (height, height) instead of (width, height) because it's fewer
   * instructions.
   */
   bool has_width = dim != GLSL_SAMPLER_DIM_CUBE;
   bool has_height = dim != GLSL_SAMPLER_DIM_1D;
   bool has_depth = dim == GLSL_SAMPLER_DIM_3D;
   nir_def *width = NULL, *height = NULL, *layers = NULL, *base_array = NULL;
            /* Get the width, height, depth, layers. */
   if (gfx_level >= GFX10) {
      if (has_width) {
      nir_def *width_lo = get_field(b, desc, 1, ~C_00A004_WIDTH_LO);
   nir_def *width_hi = get_field(b, desc, 2, ~C_00A008_WIDTH_HI);
   /* Use iadd to get s_lshl2_add_u32 in the end. */
      }
   if (has_height)
         if (has_depth)
            if (is_array) {
      last_array = get_field(b, desc, 4, ~C_00A010_DEPTH);
         } else {
      if (has_width)
         if (has_height)
         if (has_depth)
            if (is_array) {
               if (gfx_level == GFX9) {
         } else {
                        /* On GFX10.3+, DEPTH contains the pitch if the type is 1D, 2D, or 2D_MSAA. We only program
   * the pitch for 2D. We need to set depth and last_array to 0 in that case.
   */
   if (gfx_level >= GFX10_3 && (has_depth || is_array)) {
      nir_def *type = get_field(b, desc, 3, ~C_00A00C_TYPE);
            if (has_depth)
         if (is_array)
               /* All values are off by 1. */
   if (has_width)
         if (has_height)
         if (has_depth)
            if (is_array) {
      layers = nir_isub(b, last_array, base_array);
               /* Minify the dimensions according to base_level + lod. */
   if (dim != GLSL_SAMPLER_DIM_MS && dim != GLSL_SAMPLER_DIM_RECT) {
      nir_def *base_level = get_field(b, desc, 3, ~C_00A00C_BASE_LEVEL);
            if (has_width)
         if (has_height)
         if (has_depth)
            /* 1D and square texture can't have 0 size unless the lod is out-of-bounds, which is
   * undefined. Only non-square targets can have one of the sizes 0 with an in-bounds lod
   * after minification.
   */
   if (has_width && has_height) {
      if (has_width)
         if (has_height)
         if (has_depth)
                  /* Special case for sliced storage 3D views which shouldn't be minified. */
   if (gfx_level >= GFX10 && has_depth) {
      nir_def *uav3d =
         nir_def *layers_3d =
      nir_isub(b, get_field(b, desc, 4, ~C_00A010_DEPTH),
      layers_3d = nir_iadd_imm(b, layers_3d, 1);
                        /* Construct the result. */
   switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
      result = is_array ? nir_vec2(b, width, layers) : width;
      case GLSL_SAMPLER_DIM_CUBE:
      result = is_array ? nir_vec3(b, height, height, layers) : nir_vec2(b, height, height);
      case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_MS:
   case GLSL_SAMPLER_DIM_RECT:
   case GLSL_SAMPLER_DIM_EXTERNAL:
      result = is_array ? nir_vec3(b, width, height, layers) : nir_vec2(b, width, height);
      case GLSL_SAMPLER_DIM_3D:
      result = nir_vec3(b, width, height, depth);
      default:
                     }
      static bool lower_resinfo(nir_builder *b, nir_instr *instr, void *data)
   {
      enum amd_gfx_level gfx_level = *(enum amd_gfx_level*)data;
            if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   const struct glsl_type *type;
   enum glsl_sampler_dim dim;
   bool is_array;
            dst = &intr->def;
            switch (intr->intrinsic) {
   case nir_intrinsic_image_size:
   case nir_intrinsic_image_samples:
      dim = nir_intrinsic_image_dim(intr);
   is_array = nir_intrinsic_image_array(intr);
   desc = nir_image_descriptor_amd(b, dim == GLSL_SAMPLER_DIM_BUF ? 4 : 8,
               case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples:
      type = nir_instr_as_deref(intr->src[0].ssa->parent_instr)->type;
   dim = glsl_get_sampler_dim(type);
   is_array = glsl_sampler_type_is_array(type);
   desc = nir_image_deref_descriptor_amd(b, dim == GLSL_SAMPLER_DIM_BUF ? 4 : 8,
               case nir_intrinsic_bindless_image_size:
   case nir_intrinsic_bindless_image_samples:
      dim = nir_intrinsic_image_dim(intr);
   is_array = nir_intrinsic_image_array(intr);
   desc = nir_bindless_image_descriptor_amd(b, dim == GLSL_SAMPLER_DIM_BUF ? 4 : 8,
               default:
                  switch (intr->intrinsic) {
   case nir_intrinsic_image_size:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_bindless_image_size:
                  case nir_intrinsic_image_samples:
   case nir_intrinsic_image_deref_samples:
   case nir_intrinsic_bindless_image_samples:
                  default:
      assert(!desc);
         } else if (instr->type == nir_instr_type_tex) {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
   nir_tex_instr *new_tex;
   nir_def *desc = NULL;
            dst = &tex->def;
            switch (tex->op) {
   case nir_texop_txs:
   case nir_texop_query_levels:
   case nir_texop_texture_samples:
      for (unsigned i = 0; i < tex->num_srcs; i++) {
      switch (tex->src[i].src_type) {
   case nir_tex_src_texture_deref:
   case nir_tex_src_texture_handle:
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
                           case nir_tex_src_lod:
                  default:;
               switch (tex->op) {
   case nir_texop_txs:
      result = lower_query_size(b, desc, lod, tex->sampler_dim, tex->is_array,
            case nir_texop_query_levels:
      result = query_levels(b, desc);
      case nir_texop_texture_samples:
      result = query_samples(b, desc, tex->sampler_dim);
      default:
                     default:
                     if (!result)
            nir_def_rewrite_uses_after(dst, result, instr);
   nir_instr_remove(instr);
      }
      bool ac_nir_lower_resinfo(nir_shader *nir, enum amd_gfx_level gfx_level)
   {
      return nir_shader_instructions_pass(nir, lower_resinfo,
                  }
