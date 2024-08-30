   /*
   * Copyright 2021 Alyssa Rosenzweig
   * Copyright 2020 Collabora Ltd.
   * Copyright 2016 Broadcom
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_builtin_builder.h"
   #include "agx_compile.h"
   #include "agx_compiler.h"
   #include "agx_internal_formats.h"
   #include "agx_nir.h"
   #include "nir_builder_opcodes.h"
   #include "nir_intrinsics.h"
   #include "nir_intrinsics_indices.h"
      #define AGX_FORMAT_RGB32_EMULATED 0x36
   #define AGX_LAYOUT_LINEAR         0x0
      static nir_def *
   texture_descriptor_ptr_for_handle(nir_builder *b, nir_def *handle)
   {
      /* Bindless handles are a vec2, where the first source is the (constant)
   * uniform register number and the second source is the byte offset.
   */
   nir_scalar uniform = nir_scalar_resolved(handle, 0);
            nir_def *base = nir_load_preamble(b, 1, 64, uniform_idx);
               }
      static nir_def *
   texture_descriptor_ptr(nir_builder *b, nir_tex_instr *tex)
   {
      int handle_idx = nir_tex_instr_src_index(tex, nir_tex_src_texture_handle);
   assert(handle_idx >= 0 && "must be bindless");
      }
      /* Implement txs for buffer textures. There is no mipmapping to worry about, so
   * this is just a uniform pull. However, we lower buffer textures to 2D so the
   * original size is irrecoverable. Instead, we stash it in the "Acceleration
   * buffer" field, which is unused for linear images. Fetch just that.
   */
   static nir_def *
   agx_txs_buffer(nir_builder *b, nir_def *descriptor)
   {
                  }
      static nir_def *
   agx_txs(nir_builder *b, nir_tex_instr *tex)
   {
      nir_def *ptr = texture_descriptor_ptr(b, tex);
            if (tex->sampler_dim == GLSL_SAMPLER_DIM_BUF)
            nir_def *desc = nir_load_global_constant(b, ptr, 8, 4, 32);
   nir_def *w0 = nir_channel(b, desc, 0);
   nir_def *w1 = nir_channel(b, desc, 1);
            /* Width minus 1: bits [28, 42) */
   nir_def *width_m1 =
            /* Height minus 1: bits [42, 56) */
            /* Depth minus 1: bits [110, 124) */
            /* First level: bits [56, 60) */
            /* Add LOD offset to first level to get the interesting LOD */
   int lod_idx = nir_tex_instr_src_index(tex, nir_tex_src_lod);
   if (lod_idx >= 0) {
                  if (tex->sampler_dim == GLSL_SAMPLER_DIM_2D && tex->is_array) {
      /* Linear 2D arrays are special and have their depth in the next word,
   * since the depth read above is actually the stride for linear. We handle
   * this case specially.
   *
   * TODO: Optimize this, since linear 2D arrays aren't needed for APIs and
   * this just gets used internally for blits.
   */
            /* Get the 2 bytes after the first 128-bit descriptor */
   nir_def *extension =
                              depth_m1 = nir_bcsel(b, nir_ieq_imm(b, layout, AGX_LAYOUT_LINEAR),
               /* Add 1 to width-1, height-1 to get base dimensions */
   nir_def *width = nir_iadd_imm(b, width_m1, 1);
   nir_def *height = nir_iadd_imm(b, height_m1, 1);
            /* 1D Arrays have their second component as the layer count */
   if (tex->sampler_dim == GLSL_SAMPLER_DIM_1D && tex->is_array)
            /* How we finish depends on the size of the result */
   unsigned nr_comps = tex->def.num_components;
            /* Adjust for LOD, do not adjust array size */
   assert(!(nr_comps <= 1 && tex->is_array));
            if (!(nr_comps == 2 && tex->is_array))
            if (!(nr_comps == 3 && tex->is_array))
            /* Cube maps have equal width and height, we save some instructions by only
   * reading one. Dead code elimination will remove the redundant instructions.
   */
   if (tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE)
            comp[0] = width;
   comp[1] = height;
               }
      static bool
   lower_txs(nir_builder *b, nir_instr *instr, UNUSED void *data)
   {
      if (instr->type != nir_instr_type_tex)
            nir_tex_instr *tex = nir_instr_as_tex(instr);
            if (tex->op != nir_texop_txs)
            nir_def *res = agx_txs(b, tex);
   nir_def_rewrite_uses_after(&tex->def, res, instr);
   nir_instr_remove(instr);
      }
      static nir_def *
   format_is_rgb32(nir_builder *b, nir_tex_instr *tex)
   {
      nir_def *ptr = texture_descriptor_ptr(b, tex);
   nir_def *desc = nir_load_global_constant(b, ptr, 8, 1, 32);
               }
      /* Load from an RGB32 buffer texture */
   static nir_def *
   load_rgb32(nir_builder *b, nir_tex_instr *tex, nir_def *coordinate)
   {
      /* Base address right-shifted 4: bits [66, 102) */
   nir_def *ptr_hi = nir_iadd_imm(b, texture_descriptor_ptr(b, tex), 8);
   nir_def *desc_hi_words = nir_load_global_constant(b, ptr_hi, 8, 2, 32);
   nir_def *desc_hi = nir_pack_64_2x32(b, desc_hi_words);
   nir_def *base_shr4 =
                  nir_def *raw = nir_load_constant_agx(
      b, 3, tex->def.bit_size, base, nir_imul_imm(b, coordinate, 3),
         /* Set alpha to 1 (in the appropriate format) */
            nir_def *swizzled[4] = {
      nir_channel(b, raw, 0), nir_channel(b, raw, 1), nir_channel(b, raw, 2),
            }
      /*
   * Given a 1D buffer texture coordinate, calculate the 2D coordinate vector that
   * will be used to access the linear 2D texture bound to the buffer.
   */
   static nir_def *
   coords_for_buffer_texture(nir_builder *b, nir_def *coord)
   {
      return nir_vec2(b, nir_iand_imm(b, coord, BITFIELD_MASK(10)),
      }
      /*
   * Buffer textures are lowered to 2D (1024xN) textures in the driver to access
   * more storage. When lowering, we need to fix up the coordinate accordingly.
   *
   * Furthermore, RGB32 formats are emulated by lowering to global memory access,
   * so to read a buffer texture we generate code that looks like:
   *
   *    if (descriptor->format == RGB32)
   *       return ((uint32_t *) descriptor->address)[x];
   *    else
   *       return txf(texture_as_2d, vec2(x % 1024, x / 1024));
   */
   static bool
   lower_buffer_texture(nir_builder *b, nir_tex_instr *tex)
   {
               /* The OpenGL ES 3.2 specification says on page 187:
   *
   *    When a buffer texture is accessed in a shader, the results of a texel
   *    fetch are undefined if the specified texel coordinate is negative, or
   *    greater than or equal to the clamped number of texels in the texture
   *    image.
   *
   * However, faulting would be undesirable for robustness, so clamp.
   */
   nir_def *size = nir_get_texture_size(b, tex);
            /* Lower RGB32 reads if the format requires */
   nir_if *nif = nir_push_if(b, format_is_rgb32(b, tex));
   nir_def *rgb32 = load_rgb32(b, tex, coord);
            /* Otherwise, lower the texture instruction to read from 2D */
   assert(coord->num_components == 1 && "buffer textures are 1D");
            nir_def *coord2d = coords_for_buffer_texture(b, coord);
   nir_instr_remove(&tex->instr);
   nir_builder_instr_insert(b, &tex->instr);
   nir_tex_instr_add_src(tex, nir_tex_src_backend1, coord2d);
   nir_block *else_block = nir_cursor_current_block(b->cursor);
            /* Put it together with a phi */
   nir_def *phi = nir_if_phi(b, rgb32, &tex->def);
   nir_def_rewrite_uses(&tex->def, phi);
   nir_phi_instr *phi_instr = nir_instr_as_phi(phi->parent_instr);
   nir_phi_src *else_src = nir_phi_get_src_from_block(phi_instr, else_block);
   nir_src_rewrite(&else_src->src, &tex->def);
      }
      /*
   * Given a 1D texture coordinate, calculate the 2D coordinate vector that
   * will be used to access the linear 2D texture bound to the 1D texture.
   */
   static nir_def *
   coords_for_1d_texture(nir_builder *b, nir_def *coord, bool is_array)
   {
      /* Add a zero Y component to the coordinate */
   if (is_array) {
      assert(coord->num_components >= 2);
   return nir_vec3(b, nir_channel(b, coord, 0),
            } else {
      assert(coord->num_components >= 1);
         }
      /*
   * NIR indexes into array textures with unclamped floats (integer for txf). AGX
   * requires the index to be a clamped integer. Lower tex_src_coord into
   * tex_src_backend1 for array textures by type-converting and clamping.
   */
   static bool
   lower_regular_texture(nir_builder *b, nir_instr *instr, UNUSED void *data)
   {
      if (instr->type != nir_instr_type_tex)
            nir_tex_instr *tex = nir_instr_as_tex(instr);
            if (nir_tex_instr_is_query(tex))
            if (tex->sampler_dim == GLSL_SAMPLER_DIM_BUF)
            /* Don't lower twice */
   if (nir_tex_instr_src_index(tex, nir_tex_src_backend1) >= 0)
            /* Get the coordinates */
   nir_def *coord = nir_steal_tex_src(tex, nir_tex_src_coord);
            /* It's unclear if mipmapped 1D textures work in the hardware. For now, we
   * always lower to 2D.
   */
   if (tex->sampler_dim == GLSL_SAMPLER_DIM_1D) {
               /* Add a zero Y component to other sources */
   nir_tex_src_type other_srcs[] = {
      nir_tex_src_ddx,
   nir_tex_src_ddy,
               for (unsigned i = 0; i < ARRAY_SIZE(other_srcs); ++i) {
                              assert(src->num_components == 1);
   src = nir_vec2(b, src, nir_imm_intN_t(b, 0, src->bit_size));
               tex->sampler_dim = GLSL_SAMPLER_DIM_2D;
               /* The layer is always the last component of the NIR coordinate, split it off
   * because we'll need to swizzle.
   */
            if (tex->is_array) {
      unsigned lidx = coord->num_components - 1;
   nir_def *unclamped_layer = nir_channel(b, coord, lidx);
            /* Round layer to nearest even */
   if (tex->op != nir_texop_txf && tex->op != nir_texop_txf_ms)
            /* For a cube array, the layer is zero-indexed component 3 of the
   * coordinate but the number of layers is component 2 of the txs result.
   */
   if (tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE) {
      assert(lidx == 3 && "4 components");
               /* Clamp to max layer = (# of layers - 1) for out-of-bounds handling.
   * Layer must be 16-bits for the hardware, drop top bits after clamping.
   */
   if (!(tex->backend_flags & AGX_TEXTURE_FLAG_NO_CLAMP)) {
      nir_def *txs = nir_get_texture_size(b, tex);
   nir_def *nr_layers = nir_channel(b, txs, lidx);
   nir_def *max_layer = nir_iadd_imm(b, nr_layers, -1);
      } else {
                              /* Combine layer and multisample index into 32-bit so we don't need a vec5 or
   * vec6 16-bit coordinate tuple, which would be inconvenient in NIR for
   * little benefit (a minor optimization, I guess).
   */
   nir_def *sample_array = (ms_idx && layer)
                              /* Combine into the final 32-bit tuple */
   if (sample_array != NULL) {
      unsigned end = coord->num_components;
   coord = nir_pad_vector(b, coord, end + 1);
                        /* Furthermore, if there is an offset vector, it must be packed */
            if (offset != NULL) {
               for (unsigned c = 0; c < offset->num_components; ++c) {
                     if (packed != NULL)
         else
                              }
      static nir_def *
   bias_for_tex(nir_builder *b, nir_tex_instr *tex)
   {
      nir_instr *instr = nir_get_texture_size(b, tex)->parent_instr;
            query->op = nir_texop_lod_bias_agx;
            nir_def_init(instr, &query->def, 1, 16);
      }
      static bool
   lower_sampler_bias(nir_builder *b, nir_instr *instr, UNUSED void *data)
   {
      if (instr->type != nir_instr_type_tex)
            nir_tex_instr *tex = nir_instr_as_tex(instr);
            switch (tex->op) {
   case nir_texop_tex: {
      tex->op = nir_texop_txb;
   nir_tex_instr_add_src(tex, nir_tex_src_bias, bias_for_tex(b, tex));
               case nir_texop_txb:
   case nir_texop_txl: {
      nir_tex_src_type src =
            nir_def *orig = nir_steal_tex_src(tex, src);
            if (orig->bit_size != 16)
            nir_tex_instr_add_src(tex, src, nir_fadd(b, orig, bias_for_tex(b, tex)));
               case nir_texop_txd: {
      /* For txd, the computed level-of-detail is log2(rho)
   * where rho should scale proportionally to all
   * derivatives. So scale derivatives by exp2(bias) to
   * get level-of-detail log2(exp2(bias) * rho) = bias + log2(rho).
   */
   nir_def *scale = nir_fexp2(b, nir_f2f32(b, bias_for_tex(b, tex)));
            for (unsigned s = 0; s < ARRAY_SIZE(src); ++s) {
                     nir_def *scaled = nir_fmul(b, nir_f2f32(b, orig), scale);
                           case nir_texop_txf:
   case nir_texop_txf_ms:
   case nir_texop_txs:
   case nir_texop_tg4:
   case nir_texop_texture_samples:
   case nir_texop_samples_identical:
      /* These operations do not use a sampler */
         default:
            }
      static bool
   legalize_image_lod(nir_builder *b, nir_intrinsic_instr *intr, UNUSED void *data)
   {
            #define CASE(op, idx)                                                          \
      case nir_intrinsic_##op:                                                    \
   case nir_intrinsic_bindless_##op:                                           \
      src = &intr->src[idx];                                                   \
         switch (intr->intrinsic) {
      CASE(image_load, 3)
   CASE(image_store, 4)
      default:
               #undef CASE
         if (src->ssa->bit_size == 16)
            b->cursor = nir_before_instr(&intr->instr);
   nir_src_rewrite(src, nir_i2i16(b, src->ssa));
      }
      static nir_def *
   txs_for_image(nir_builder *b, nir_intrinsic_instr *intr,
         {
      nir_tex_instr *tex = nir_tex_instr_create(b->shader, 2);
   tex->op = nir_texop_txs;
   tex->is_array = nir_intrinsic_image_array(intr);
   tex->dest_type = nir_type_uint32;
            tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_lod, intr->src[1].ssa);
   tex->src[1] =
            nir_def_init(&tex->instr, &tex->def, num_components, bit_size);
   nir_builder_instr_insert(b, &tex->instr);
      }
      static nir_def *
   nir_bitfield_mask(nir_builder *b, nir_def *x)
   {
      nir_def *one = nir_imm_intN_t(b, 1, x->bit_size);
      }
      static nir_def *
   calculate_twiddled_coordinates(nir_builder *b, nir_def *coord,
               {
      /* SIMD-within-a-register */
   nir_def *coord_px = nir_pack_32_2x16(b, nir_u2u16(b, coord));
   nir_def *tile_mask =
      nir_pack_32_2x16_split(b, nir_bitfield_mask(b, tile_w_px_log2),
         /* Modulo by the tile width/height to get the offsets within the tile */
            /* Get the coordinates of the corner of the tile */
            /* Unpack SIMD-within-a-register */
   nir_def *offs_x_px = nir_unpack_32_2x16_split_x(b, offs_xy_px);
   nir_def *offs_y_px = nir_unpack_32_2x16_split_y(b, offs_xy_px);
   nir_def *tile_x_px = nir_u2u32(b, nir_unpack_32_2x16_split_x(b, tile_xy_px));
            /* Get the tile size */
   nir_def *one_32 = nir_imm_int(b, 1);
   nir_def *tile_w_px = nir_ishl(b, one_32, nir_u2u32(b, tile_w_px_log2));
            /* tile row start (px) =
   *   (y // tile height) * (# of tiles/row) * (# of pix/tile) =
   *   align_down(y, tile height) / tile height * width_tl *tile width *
   *        tile height =
   *   align_down(y, tile height) * width_tl * tile width
   */
   nir_def *tile_row_start_px =
            /* tile column start (px) =
   *   (x // tile width) * (# of pix/tile) =
   *   align(x, tile width) / tile width * tile width * tile height =
   *   align(x, tile width) * tile height
   */
            /* The pixel at which the tile starts is thus... */
            /* Get the total offset */
   nir_def *offs_px = nir_interleave_agx(b, offs_x_px, offs_y_px);
            if (layer_stride_px) {
      nir_def *layer = nir_channel(b, coord, 2);
   nir_def *layer_offset_px = nir_imul(b, layer, layer_stride_px);
                  }
      static nir_def *
   image_texel_address(nir_builder *b, nir_intrinsic_instr *intr,
         {
      /* First, calculate the address of the PBE descriptor */
   nir_def *desc_address =
                     enum glsl_sampler_dim dim = nir_intrinsic_image_dim(intr);
   bool layered = nir_intrinsic_image_array(intr) ||
                  /* The last 8 bytes of the 24-byte PBE descriptor contain either the
   * software-defined atomic descriptor, or (if array image) a pointer to the
   * descriptor. Grab it.
   */
   nir_def *meta_ptr = nir_iadd_imm(b, desc_address, 16);
   nir_def *meta = nir_load_global_constant(b, meta_ptr, 8, 1, 64);
            if (layered) {
      nir_def *desc = nir_load_global_constant(b, meta, 8, 3, 32);
   meta = nir_pack_64_2x32(b, nir_trim_vector(b, desc, 2));
                        /* See the GenXML definitions of the software-defined atomic descriptors */
            if (dim == GLSL_SAMPLER_DIM_BUF)
         else
            nir_def *tile_w_px_log2 =
         nir_def *tile_h_px_log2 =
                  /* We do not allow atomics on linear 2D or linear 2D arrays, as there are no
   * known use cases. So, we're linear if buffer or 1D, and twiddled otherwise.
   */
   nir_def *total_px;
   if (dim == GLSL_SAMPLER_DIM_BUF || dim == GLSL_SAMPLER_DIM_1D) {
      /* 1D linear is indexed directly */
      } else {
      total_px = calculate_twiddled_coordinates(
                        if (dim == GLSL_SAMPLER_DIM_MS) {
      nir_def *sample_idx = intr->src[2].ssa;
               } else {
                  /* Early return if we just want a linearized texel index */
   if (return_index)
            /* Calculate the full texel address. This sequence is written carefully to
   * ensure it will be entirely folded into the atomic's addressing arithmetic.
   */
   enum pipe_format format = nir_intrinsic_format(intr);
            nir_def *total_B = nir_imul_imm(b, total_sa, bytes_per_sample_B);
      }
      static void
   lower_buffer_image(nir_builder *b, nir_intrinsic_instr *intr)
   {
      nir_def *coord_vector = intr->src[1].ssa;
            /* Lower the buffer load/store to a 2D image load/store, matching the 2D
   * texture/PBE descriptor the driver supplies for buffer images.
   */
   nir_def *coord2d = coords_for_buffer_texture(b, coord);
   nir_src_rewrite(&intr->src[1], nir_pad_vector(b, coord2d, 4));
      }
      static void
   lower_1d_image(nir_builder *b, nir_intrinsic_instr *intr)
   {
      nir_def *coord = intr->src[1].ssa;
   bool is_array = nir_intrinsic_image_array(intr);
            nir_src_rewrite(&intr->src[1], nir_pad_vector(b, coord2d, 4));
      }
      /*
   * AGX needs the face and the layer specified separately. This matches how NIR
   * texture instructions work, but not how NIR image intrinsics work. Here we
   * lower by dividing the combined layer-face into separate components which the
   * compiler can consume.
   */
   static void
   lower_cube_array_image(nir_builder *b, nir_intrinsic_instr *intr)
   {
      nir_def *x = nir_channel(b, intr->src[1].ssa, 0);
   nir_def *y = nir_channel(b, intr->src[1].ssa, 1);
            nir_def *face = nir_umod_imm(b, z, 6);
               }
      static bool
   lower_images(nir_builder *b, nir_intrinsic_instr *intr, UNUSED void *data)
   {
               switch (intr->intrinsic) {
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_store: {
      switch (nir_intrinsic_image_dim(intr)) {
   case GLSL_SAMPLER_DIM_1D:
                  case GLSL_SAMPLER_DIM_BUF:
                  case GLSL_SAMPLER_DIM_CUBE:
      if (nir_intrinsic_image_array(intr)) {
      lower_cube_array_image(b, intr);
                     default:
                     case nir_intrinsic_bindless_image_size:
      nir_def_rewrite_uses(
      &intr->def,
            case nir_intrinsic_bindless_image_texel_address:
      nir_def_rewrite_uses(&intr->def, image_texel_address(b, intr, false));
         case nir_intrinsic_image_size:
   case nir_intrinsic_image_texel_address:
            default:
            }
      /*
   * Early texture lowering passes, called by the driver before lowering
   * descriptor bindings. That means these passes operate on texture derefs. The
   * purpose is to make descriptor crawls explicit in the NIR, so that the driver
   * can accurately lower descriptors after this pass but before calling
   * agx_preprocess_nir (and hence the full agx_nir_lower_texture).
   */
   bool
   agx_nir_lower_texture_early(nir_shader *s)
   {
               nir_lower_tex_options lower_tex_options = {
      .lower_txp = ~0,
   .lower_invalid_implicit_lod = true,
   .lower_tg4_offsets = true,
            /* XXX: Metal seems to handle just like 3D txd, so why doesn't it work?
   * TODO: Stop using this lowering
   */
                           }
      bool
   agx_nir_lower_texture(nir_shader *s, bool support_lod_bias)
   {
               nir_tex_src_type_constraints tex_constraints = {
      [nir_tex_src_lod] = {true, 16},
   [nir_tex_src_bias] = {true, 16},
   [nir_tex_src_ms_index] = {true, 16},
   [nir_tex_src_texture_offset] = {true, 16},
               /* Insert fences before lowering image atomics, since image atomics need
   * different fencing than other image operations.
   */
                     /* Lower bias after nir_lower_tex (to get rid of txd) but before
   * lower_regular_texture (which will shuffle around the sources)
   */
   if (support_lod_bias) {
      NIR_PASS(progress, s, nir_shader_instructions_pass, lower_sampler_bias,
               NIR_PASS(progress, s, nir_shader_intrinsics_pass, legalize_image_lod,
         NIR_PASS(progress, s, nir_shader_intrinsics_pass, lower_images,
                  /* Lower texture sources after legalizing types (as the lowering depends on
   * 16-bit multisample indices) but before lowering queries (as the lowering
   * generates txs for array textures).
   */
   NIR_PASS(progress, s, nir_shader_instructions_pass, lower_regular_texture,
         NIR_PASS(progress, s, nir_shader_instructions_pass, lower_txs,
               }
      static bool
   lower_multisampled_store(nir_builder *b, nir_intrinsic_instr *intr,
         {
               if (intr->intrinsic != nir_intrinsic_bindless_image_store)
            if (nir_intrinsic_image_dim(intr) != GLSL_SAMPLER_DIM_MS)
            nir_def *index_px = image_texel_address(b, intr, true);
            nir_src_rewrite(&intr->src[1], nir_pad_vector(b, coord2d, 4));
   nir_src_rewrite(&intr->src[2], nir_imm_int(b, 0));
   nir_intrinsic_set_image_dim(intr, GLSL_SAMPLER_DIM_2D);
   nir_intrinsic_set_image_array(intr, false);
      }
      bool
   agx_nir_lower_multisampled_image_store(nir_shader *s)
   {
      return nir_shader_intrinsics_pass(
      s, lower_multisampled_store,
   }
      /*
   * Given a non-bindless instruction, return whether agx_nir_lower_texture will
   * lower it to something involving a descriptor crawl. This requires the driver
   * to lower the instruction to bindless before calling agx_nir_lower_texture.
   * The implementation just enumerates the cases handled in this file.
   */
   bool
   agx_nir_needs_texture_crawl(nir_instr *instr)
   {
      if (instr->type == nir_instr_type_intrinsic) {
               switch (intr->intrinsic) {
   /* Queries, atomics always become a crawl */
   case nir_intrinsic_image_size:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_image_deref_atomic_swap:
            /* Multisampled stores need a crawl, others do not */
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_deref_store:
            /* Loads do not need a crawl, even from buffers */
   default:
            } else if (instr->type == nir_instr_type_tex) {
               /* Array textures get clamped to their size via txs */
   if (tex->is_array)
            switch (tex->op) {
   /* Queries always become a crawl */
   case nir_texop_txs:
            /* Buffer textures need their format read */
   default:
                        }
