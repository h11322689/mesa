   /*
   * Copyright © 2018 Intel Corporation
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "isl/isl.h"
      #include "brw_nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_format_convert.h"
      static nir_def *
   _load_image_param(nir_builder *b, nir_deref_instr *deref, unsigned offset)
   {
      nir_intrinsic_instr *load =
      nir_intrinsic_instr_create(b->shader,
      load->src[0] = nir_src_for_ssa(&deref->def);
            switch (offset) {
   case BRW_IMAGE_PARAM_OFFSET_OFFSET:
   case BRW_IMAGE_PARAM_SWIZZLING_OFFSET:
      load->num_components = 2;
      case BRW_IMAGE_PARAM_TILING_OFFSET:
   case BRW_IMAGE_PARAM_SIZE_OFFSET:
      load->num_components = 3;
      case BRW_IMAGE_PARAM_STRIDE_OFFSET:
      load->num_components = 4;
      default:
         }
            nir_builder_instr_insert(b, &load->instr);
      }
      #define load_image_param(b, d, o) \
            static nir_def *
   image_coord_is_in_bounds(nir_builder *b, nir_deref_instr *deref,
         {
      nir_def *size = load_image_param(b, deref, SIZE);
            unsigned coord_comps = glsl_get_sampler_coordinate_components(deref->type);
   nir_def *in_bounds = nir_imm_true(b);
   for (unsigned i = 0; i < coord_comps; i++)
               }
      /** Calculate the offset in memory of the texel given by \p coord.
   *
   * This is meant to be used with untyped surface messages to access a tiled
   * surface, what involves taking into account the tiling and swizzling modes
   * of the surface manually so it will hopefully not happen very often.
   *
   * The tiling algorithm implemented here matches either the X or Y tiling
   * layouts supported by the hardware depending on the tiling coefficients
   * passed to the program as uniforms.  See Volume 1 Part 2 Section 4.5
   * "Address Tiling Function" of the IVB PRM for an in-depth explanation of
   * the hardware tiling format.
   */
   static nir_def *
   image_address(nir_builder *b, const struct intel_device_info *devinfo,
         {
      if (glsl_get_sampler_dim(deref->type) == GLSL_SAMPLER_DIM_1D &&
      glsl_sampler_type_is_array(deref->type)) {
   /* It's easier if 1D arrays are treated like 2D arrays */
   coord = nir_vec3(b, nir_channel(b, coord, 0),
            } else {
      unsigned dims = glsl_get_sampler_coordinate_components(deref->type);
               nir_def *offset = load_image_param(b, deref, OFFSET);
   nir_def *tiling = load_image_param(b, deref, TILING);
            /* Shift the coordinates by the fixed surface offset.  It may be non-zero
   * if the image is a single slice of a higher-dimensional surface, or if a
   * non-zero mipmap level of the surface is bound to the pipeline.  The
   * offset needs to be applied here rather than at surface state set-up time
   * because the desired slice-level may start mid-tile, so simply shifting
   * the surface base address wouldn't give a well-formed tiled surface in
   * the general case.
   */
   nir_def *xypos = (coord->num_components == 1) ?
                        /* The layout of 3-D textures in memory is sort-of like a tiling
   * format.  At each miplevel, the slices are arranged in rows of
   * 2^level slices per row.  The slice row is stored in tmp.y and
   * the slice within the row is stored in tmp.x.
   *
   * The layout of 2-D array textures and cubemaps is much simpler:
   * Depending on whether the ARYSPC_LOD0 layout is in use it will be
   * stored in memory as an array of slices, each one being a 2-D
   * arrangement of miplevels, or as a 2D arrangement of miplevels,
   * each one being an array of slices.  In either case the separation
   * between slices of the same LOD is equal to the qpitch value
   * provided as stride.w.
   *
   * This code can be made to handle either 2D arrays and 3D textures
   * by passing in the miplevel as tile.z for 3-D textures and 0 in
   * tile.z for 2-D array textures.
   *
   * See Volume 1 Part 1 of the Gfx7 PRM, sections 6.18.4.7 "Surface
   * Arrays" and 6.18.6 "3D Surfaces" for a more extensive discussion
   * of the hardware 3D texture and 2D array layouts.
   */
   if (coord->num_components > 2) {
      /* Decompose z into a major (tmp.y) and a minor (tmp.x)
   * index.
   */
   nir_def *z = nir_channel(b, coord, 2);
   nir_def *z_x = nir_ubfe(b, z, nir_imm_int(b, 0),
                  /* Take into account the horizontal (tmp.x) and vertical (tmp.y)
   * slice offset.
   */
   xypos = nir_iadd(b, xypos, nir_imul(b, nir_vec2(b, z_x, z_y),
               nir_def *addr;
   if (coord->num_components > 1) {
      /* Calculate the major/minor x and y indices.  In order to
   * accommodate both X and Y tiling, the Y-major tiling format is
   * treated as being a bunch of narrow X-tiles placed next to each
   * other.  This means that the tile width for Y-tiling is actually
   * the width of one sub-column of the Y-major tile where each 4K
   * tile has 8 512B sub-columns.
   *
   * The major Y value is the row of tiles in which the pixel lives.
   * The major X value is the tile sub-column in which the pixel
   * lives; for X tiling, this is the same as the tile column, for Y
   * tiling, each tile has 8 sub-columns.  The minor X and Y indices
   * are the position within the sub-column.
            /* Calculate the minor x and y indices. */
   nir_def *minor = nir_ubfe(b, xypos, nir_imm_int(b, 0),
                  /* Calculate the texel index from the start of the tile row and the
   * vertical coordinate of the row.
   * Equivalent to:
   *   tmp.x = (major.x << tile.y << tile.x) +
   *           (minor.y << tile.x) + minor.x
   *   tmp.y = major.y << tile.y
   */
   nir_def *idx_x, *idx_y;
   idx_x = nir_ishl(b, nir_channel(b, major, 0), nir_channel(b, tiling, 1));
   idx_x = nir_iadd(b, idx_x, nir_channel(b, minor, 1));
   idx_x = nir_ishl(b, idx_x, nir_channel(b, tiling, 0));
   idx_x = nir_iadd(b, idx_x, nir_channel(b, minor, 0));
            /* Add it to the start of the tile row. */
   nir_def *idx;
   idx = nir_imul(b, idx_y, nir_channel(b, stride, 1));
            /* Multiply by the Bpp value. */
            if (devinfo->ver < 8 && devinfo->platform != INTEL_PLATFORM_BYT) {
      /* Take into account the two dynamically specified shifts.  Both are
   * used to implement swizzling of X-tiled surfaces.  For Y-tiled
   * surfaces only one bit needs to be XOR-ed with bit 6 of the memory
   * address, so a swz value of 0xff (actually interpreted as 31 by the
   * hardware) will be provided to cause the relevant bit of tmp.y to
   * be zero and turn the first XOR into the identity.  For linear
   * surfaces or platforms lacking address swizzling both shifts will
   * be 0xff causing the relevant bits of both tmp.x and .y to be zero,
   * what effectively disables swizzling.
   */
   nir_def *swizzle = load_image_param(b, deref, SWIZZLING);
                  /* XOR tmp.x and tmp.y with bit 6 of the memory address. */
   nir_def *bit = nir_iand(b, nir_ixor(b, shift0, shift1),
               } else {
      /* Multiply by the Bpp/stride value.  Note that the addr.y may be
   * non-zero even if the image is one-dimensional because a vertical
   * offset may have been applied above to select a non-zero slice or
   * level of a higher-dimensional texture.
   */
   nir_def *idx;
   idx = nir_imul(b, nir_channel(b, xypos, 1), nir_channel(b, stride, 1));
   idx = nir_iadd(b, nir_channel(b, xypos, 0), idx);
                  }
      struct format_info {
      const struct isl_format_layout *fmtl;
   unsigned chans;
      };
      static struct format_info
   get_format_info(enum isl_format fmt)
   {
               return (struct format_info) {
      .fmtl = fmtl,
   .chans = isl_format_get_num_channels(fmt),
   .bits = {
      fmtl->channels.r.bits,
   fmtl->channels.g.bits,
   fmtl->channels.b.bits,
            }
      static nir_def *
   convert_color_for_load(nir_builder *b, const struct intel_device_info *devinfo,
                     {
      if (image_fmt == lower_fmt)
            if (image_fmt == ISL_FORMAT_R11G11B10_FLOAT) {
      assert(lower_fmt == ISL_FORMAT_R32_UINT);
   color = nir_format_unpack_11f11f10f(b, color);
               struct format_info image = get_format_info(image_fmt);
            const bool needs_sign_extension =
      isl_format_has_snorm_channel(image_fmt) ||
         /* We only check the red channel to detect if we need to pack/unpack */
   assert(image.bits[0] != lower.bits[0] ||
            if (image.bits[0] != lower.bits[0] && lower_fmt == ISL_FORMAT_R32_UINT) {
      if (needs_sign_extension)
         else
      } else {
      /* All these formats are homogeneous */
   for (unsigned i = 1; i < image.chans; i++)
            /* On IVB, we rely on the undocumented behavior that typed reads from
   * surfaces of the unsupported R8 and R16 formats return useful data in
   * their least significant bits.  However, the data in the high bits is
   * garbage so we have to discard it.
   */
   if (devinfo->verx10 == 70 &&
      (lower_fmt == ISL_FORMAT_R16_UINT ||
               if (image.bits[0] != lower.bits[0]) {
      color = nir_format_bitcast_uvec_unmasked(b, color, lower.bits[0],
               if (needs_sign_extension)
               switch (image.fmtl->channels.r.type) {
   case ISL_UNORM:
      assert(isl_format_has_uint_channel(lower_fmt));
   color = nir_format_unorm_to_float(b, color, image.bits);
         case ISL_SNORM:
      assert(isl_format_has_uint_channel(lower_fmt));
   color = nir_format_snorm_to_float(b, color, image.bits);
         case ISL_SFLOAT:
      if (image.bits[0] == 16)
               case ISL_UINT:
   case ISL_SINT:
            default:
               expand_vec:
      assert(dest_components == 1 || dest_components == 4);
   assert(color->num_components <= dest_components);
   if (color->num_components == dest_components)
            nir_def *comps[4];
   for (unsigned i = 0; i < color->num_components; i++)
            for (unsigned i = color->num_components; i < 3; i++)
            if (color->num_components < 4) {
      if (isl_format_has_int_channel(image_fmt))
         else
                  }
      static bool
   lower_image_load_instr(nir_builder *b,
                     {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
            if (var->data.image.format == PIPE_FORMAT_NONE)
            const enum isl_format image_fmt =
            if (isl_has_matching_typed_storage_image_format(devinfo, image_fmt)) {
      const enum isl_format lower_fmt =
         const unsigned dest_components =
            /* Use an undef to hold the uses of the load while we do the color
   * conversion.
   */
   nir_def *placeholder = nir_undef(b, 4, 32);
            intrin->num_components = isl_format_get_num_channels(lower_fmt);
                     nir_def *color = convert_color_for_load(b, devinfo,
                        if (sparse) {
      /* Put the sparse component back on the original instruction */
                  /* Carry over the sparse component without modifying it with the
   * converted color.
   */
   nir_def *sparse_color[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < dest_components; i++)
         sparse_color[dest_components] =
                     nir_def_rewrite_uses(placeholder, color);
      } else {
      /* This code part is only useful prior to Gfx9, we do not have plans to
   * enable sparse there.
   */
            const struct isl_format_layout *image_fmtl =
         /* We have a matching typed format for everything 32b and below */
   assert(image_fmtl->bpb == 64 || image_fmtl->bpb == 128);
   enum isl_format raw_fmt = (image_fmtl->bpb == 64) ?
                                          nir_def *do_load = image_coord_is_in_bounds(b, deref, coord);
   if (devinfo->verx10 == 70) {
      /* Check whether the first stride component (i.e. the Bpp value)
   * is greater than four, what on Gfx7 indicates that a surface of
   * type RAW has been bound for untyped access.  Reading or writing
   * to a surface of type other than RAW using untyped surface
   * messages causes a hang on IVB and VLV.
   */
   nir_def *stride = load_image_param(b, deref, STRIDE);
   nir_def *is_raw =
            }
            nir_def *addr = image_address(b, devinfo, deref, coord);
   nir_def *load =
                                                      nir_def *color = convert_color_for_load(b, devinfo, value,
                                 }
      static nir_def *
   convert_color_for_store(nir_builder *b, const struct intel_device_info *devinfo,
               {
      struct format_info image = get_format_info(image_fmt);
                     if (image_fmt == lower_fmt)
            if (image_fmt == ISL_FORMAT_R11G11B10_FLOAT) {
      assert(lower_fmt == ISL_FORMAT_R32_UINT);
               switch (image.fmtl->channels.r.type) {
   case ISL_UNORM:
      assert(isl_format_has_uint_channel(lower_fmt));
   color = nir_format_float_to_unorm(b, color, image.bits);
         case ISL_SNORM:
      assert(isl_format_has_uint_channel(lower_fmt));
   color = nir_format_float_to_snorm(b, color, image.bits);
         case ISL_SFLOAT:
      if (image.bits[0] == 16)
               case ISL_UINT:
      color = nir_format_clamp_uint(b, color, image.bits);
         case ISL_SINT:
      color = nir_format_clamp_sint(b, color, image.bits);
         default:
                  if (image.bits[0] < 32 &&
      (isl_format_has_snorm_channel(image_fmt) ||
   isl_format_has_sint_channel(image_fmt)))
         if (image.bits[0] != lower.bits[0] && lower_fmt == ISL_FORMAT_R32_UINT) {
         } else {
      /* All these formats are homogeneous */
   for (unsigned i = 1; i < image.chans; i++)
            if (image.bits[0] != lower.bits[0]) {
      color = nir_format_bitcast_uvec_unmasked(b, color, image.bits[0],
                     }
      static bool
   lower_image_store_instr(nir_builder *b,
               {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
            /* For write-only surfaces, we trust that the hardware can just do the
   * conversion for us.
   */
   if (var->data.access & ACCESS_NON_READABLE)
            if (var->data.image.format == PIPE_FORMAT_NONE)
            const enum isl_format image_fmt =
            if (isl_has_matching_typed_storage_image_format(devinfo, image_fmt)) {
      const enum isl_format lower_fmt =
            /* Color conversion goes before the store */
            nir_def *color = convert_color_for_store(b, devinfo,
               intrin->num_components = isl_format_get_num_channels(lower_fmt);
      } else {
      const struct isl_format_layout *image_fmtl =
         /* We have a matching typed format for everything 32b and below */
   assert(image_fmtl->bpb == 64 || image_fmtl->bpb == 128);
   enum isl_format raw_fmt = (image_fmtl->bpb == 64) ?
                                    nir_def *do_store = image_coord_is_in_bounds(b, deref, coord);
   if (devinfo->verx10 == 70) {
      /* Check whether the first stride component (i.e. the Bpp value)
   * is greater than four, what on Gfx7 indicates that a surface of
   * type RAW has been bound for untyped access.  Reading or writing
   * to a surface of type other than RAW using untyped surface
   * messages causes a hang on IVB and VLV.
   */
   nir_def *stride = load_image_param(b, deref, STRIDE);
   nir_def *is_raw =
            }
            nir_def *addr = image_address(b, devinfo, deref, coord);
   nir_def *color = convert_color_for_store(b, devinfo,
                  nir_intrinsic_instr *store =
      nir_intrinsic_instr_create(b->shader,
      store->src[0] = nir_src_for_ssa(&deref->def);
   store->src[1] = nir_src_for_ssa(addr);
   store->src[2] = nir_src_for_ssa(color);
   store->num_components = image_fmtl->bpb / 32;
                           }
      static bool
   lower_image_atomic_instr(nir_builder *b,
               {
      if (devinfo->verx10 >= 75)
                              /* Use an undef to hold the uses of the load conversion. */
   nir_def *placeholder = nir_undef(b, 4, 32);
            /* Check the first component of the size field to find out if the
   * image is bound.  Necessary on IVB for typed atomics because
   * they don't seem to respect null surfaces and will happily
   * corrupt or read random memory when no image is bound.
   */
   nir_def *size = load_image_param(b, deref, SIZE);
   nir_def *zero = nir_imm_int(b, 0);
                              nir_def *result = nir_if_phi(b, &intrin->def, zero);
               }
      static bool
   lower_image_size_instr(nir_builder *b,
               {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
            /* For write-only images, we have an actual image surface so we fall back
   * and let the back-end emit a TXS for this.
   */
   if (var->data.access & ACCESS_NON_READABLE)
            if (var->data.image.format == PIPE_FORMAT_NONE)
            /* If we have a matching typed format, then we have an actual image surface
   * so we fall back and let the back-end emit a TXS for this.
   */
   const enum isl_format image_fmt =
         if (isl_has_matching_typed_storage_image_format(devinfo, image_fmt))
                                                assert(nir_intrinsic_image_dim(intrin) != GLSL_SAMPLER_DIM_CUBE);
   unsigned coord_comps = glsl_get_sampler_coordinate_components(deref->type);
   for (unsigned c = 0; c < coord_comps; c++)
            for (unsigned c = coord_comps; c < intrin->def.num_components; ++c)
            nir_def *vec = nir_vec(b, comps, intrin->def.num_components);
               }
      static bool
   brw_nir_lower_storage_image_instr(nir_builder *b,
               {
      if (instr->type != nir_instr_type_intrinsic)
                  nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_image_deref_load:
      if (opts->lower_loads)
               case nir_intrinsic_image_deref_sparse_load:
      if (opts->lower_loads)
               case nir_intrinsic_image_deref_store:
      if (opts->lower_stores)
               case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
      if (opts->lower_atomics)
               case nir_intrinsic_image_deref_size:
      if (opts->lower_get_size)
               default:
      /* Nothing to do */
         }
      bool
   brw_nir_lower_storage_image(nir_shader *shader,
         {
               const nir_lower_image_options image_options = {
      .lower_cube_size = true,
                        progress |= nir_shader_instructions_pass(shader,
                           }
