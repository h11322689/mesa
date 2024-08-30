   /*
   * Copyright 2023 Valve Corpoation
   * Copyright 2020 Raspberry Pi Ltd
   * SPDX-License-Identifier: MIT
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_intrinsics_indices.h"
      static void
   rewrite_offset(nir_builder *b, nir_intrinsic_instr *instr,
         {
      /* Compute the maximum offset being accessed and if it is out of bounds
   * rewrite it to 0 to ensure the access is within bounds.
   */
   const uint32_t access_size = instr->num_components * type_sz;
   nir_def *max_access_offset =
         nir_def *offset =
      nir_bcsel(b, nir_uge(b, max_access_offset, size), nir_imm_int(b, 0),
         /* Rewrite offset */
      }
      /*
   * Wrap a intrinsic in an if, predicated on a "valid" condition. If the
   * intrinsic produces a destination, it will be zero in the invalid case.
   */
   static void
   wrap_in_if(nir_builder *b, nir_intrinsic_instr *instr, nir_def *valid)
   {
      bool has_dest = nir_intrinsic_infos[instr->intrinsic].has_dest;
            if (has_dest) {
      zero = nir_imm_zero(b, instr->def.num_components,
               nir_push_if(b, valid);
   {
      nir_instr *orig = nir_instr_clone(b->shader, &instr->instr);
            if (has_dest)
      }
            if (has_dest)
            /* We've cloned and wrapped, so drop original instruction */
      }
      static void
   lower_buffer_load(nir_builder *b,
               {
      uint32_t type_sz = instr->def.bit_size / 8;
   nir_def *size;
            if (instr->intrinsic == nir_intrinsic_load_ubo) {
         } else {
                     }
      static void
   lower_buffer_store(nir_builder *b, nir_intrinsic_instr *instr)
   {
      uint32_t type_sz = nir_src_bit_size(instr->src[0]) / 8;
   rewrite_offset(b, instr, type_sz, 2,
      }
      static void
   lower_buffer_atomic(nir_builder *b, nir_intrinsic_instr *instr)
   {
         }
      static void
   lower_buffer_shared(nir_builder *b, nir_intrinsic_instr *instr)
   {
      uint32_t type_sz, offset_src;
   if (instr->intrinsic == nir_intrinsic_load_shared) {
      offset_src = 0;
      } else if (instr->intrinsic == nir_intrinsic_store_shared) {
      offset_src = 1;
      } else {
      /* atomic */
   offset_src = 0;
               rewrite_offset(b, instr, type_sz, offset_src,
      }
      static bool
   lower_image(nir_builder *b,
               {
      enum glsl_sampler_dim dim = nir_intrinsic_image_dim(instr);
   bool atomic = (instr->intrinsic == nir_intrinsic_image_atomic ||
         if (!opts->lower_image &&
      !(opts->lower_buffer_image && dim == GLSL_SAMPLER_DIM_BUF) &&
   !(opts->lower_image_atomic && atomic))
         uint32_t num_coords = nir_image_intrinsic_coord_components(instr);
   bool is_array = nir_intrinsic_image_array(instr);
            /* Get image size. imageSize for cubes returns the size of a single face. */
   unsigned size_components = num_coords;
   if (dim == GLSL_SAMPLER_DIM_CUBE && !is_array)
            nir_def *size =
      nir_image_size(b, size_components, 32,
               if (dim == GLSL_SAMPLER_DIM_CUBE) {
      nir_def *z = is_array ? nir_imul_imm(b, nir_channel(b, size, 2), 6)
                        /* Only execute if coordinates are in-bounds. Otherwise, return zero. */
   wrap_in_if(b, instr, nir_ball(b, nir_ult(b, coord, size)));
      }
      static bool
   lower(nir_builder *b, nir_instr *instr, void *_opts)
   {
      const nir_lower_robust_access_options *opts = _opts;
   if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
            switch (intr->intrinsic) {
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
            case nir_intrinsic_load_ubo:
      if (opts->lower_ubo) {
      lower_buffer_load(b, intr, opts);
      }
         case nir_intrinsic_load_ssbo:
      if (opts->lower_ssbo) {
      lower_buffer_load(b, intr, opts);
      }
      case nir_intrinsic_store_ssbo:
      if (opts->lower_ssbo) {
      lower_buffer_store(b, intr);
      }
      case nir_intrinsic_ssbo_atomic:
      if (opts->lower_ssbo) {
      lower_buffer_atomic(b, intr);
      }
         case nir_intrinsic_store_shared:
   case nir_intrinsic_load_shared:
   case nir_intrinsic_shared_atomic:
   case nir_intrinsic_shared_atomic_swap:
      if (opts->lower_shared) {
      lower_buffer_shared(b, intr);
      }
         default:
            }
      bool
   nir_lower_robust_access(nir_shader *s,
         {
      return nir_shader_instructions_pass(s, lower, nir_metadata_block_index | nir_metadata_dominance,
      }
