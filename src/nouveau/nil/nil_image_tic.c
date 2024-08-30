   /*
   * Copyright © 2022 Collabora Ltd.
   * SPDX-License-Identifier: MIT
   */
   #include "nil_image.h"
      #include "nil_format.h"
   #include "util/bitpack_helpers.h"
      #include "nouveau_device.h"
      #include "cl9097.h"
   #include "cl9097tex.h"
   #include "clb097.h"
   #include "clb097tex.h"
   #include "drf.h"
      ALWAYS_INLINE static void
   __set_u32(uint32_t *o, uint32_t v, unsigned lo, unsigned hi)
   {
      assert(lo <= hi && (lo / 32) == (hi / 32));
      }
      #define FIXED_FRAC_BITS 8
      ALWAYS_INLINE static void
   __set_ufixed(uint32_t *o, float v, unsigned lo, unsigned hi)
   {
      assert(lo <= hi && (lo / 32) == (hi / 32));
   o[lo / 32] |= util_bitpack_ufixed_clamp(v, lo % 32, hi % 32,
      }
      ALWAYS_INLINE static void
   __set_i32(uint32_t *o, int32_t v, unsigned lo, unsigned hi)
   {
      assert(lo <= hi && (lo / 32) == (hi / 32));
      }
      ALWAYS_INLINE static void
   __set_bool(uint32_t *o, bool b, unsigned lo, unsigned hi)
   {
      assert(lo == hi);
      }
      #define MW(x) x
      #define TH_SET_U(o, NV, VER, FIELD, val) \
      __set_u32((o), (val), DRF_LO(NV##_TEXHEAD##VER##_##FIELD),\
         #define TH_SET_UF(o, NV, VER, FIELD, val) \
      __set_ufixed((o), (val), DRF_LO(NV##_TEXHEAD##VER##_##FIELD),\
         #define TH_SET_I(o, NV, VER, FIELD, val) \
      __set_i32((o), (val), DRF_LO(NV##_TEXHEAD##VER##_##FIELD),\
         #define TH_SET_B(o, NV, VER, FIELD, b) \
      __set_bool((o), (b), DRF_LO(NV##_TEXHEAD##VER##_##FIELD),\
         #define TH_SET_E(o, NV, VER, FIELD, E) \
            #define TH_NV9097_SET_U(o, IDX, FIELD, val) \
         #define TH_NV9097_SET_UF(o, IDX, FIELD, val) \
         #define TH_NV9097_SET_I(o, IDX, FIELD, val) \
         #define TH_NV9097_SET_B(o, IDX, FIELD, b) \
         #define TH_NV9097_SET_E(o, IDX, FIELD, E) \
            #define TH_NVB097_SET_U(o, VER, FIELD, val) \
         #define TH_NVB097_SET_UF(o, VER, FIELD, val) \
         #define TH_NVB097_SET_I(o, VER, FIELD, val) \
         #define TH_NVB097_SET_B(o, VER, FIELD, b) \
         #define TH_NVB097_SET_E(o, VER, FIELD, E) \
            static inline uint32_t
   nv9097_th_bl_source(const struct nil_tic_format *fmt,
         {
      switch (swz) {
   case PIPE_SWIZZLE_X: return fmt->src_x;
   case PIPE_SWIZZLE_Y: return fmt->src_y;
   case PIPE_SWIZZLE_Z: return fmt->src_z;
   case PIPE_SWIZZLE_W: return fmt->src_w;
   case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
      return is_int ? NV9097_TEXHEADV2_0_X_SOURCE_IN_ONE_INT :
      default:
            }
      static inline uint32_t
   nvb097_th_bl_source(const struct nil_tic_format *fmt,
         {
      switch (swz) {
   case PIPE_SWIZZLE_X: return fmt->src_x;
   case PIPE_SWIZZLE_Y: return fmt->src_y;
   case PIPE_SWIZZLE_Z: return fmt->src_z;
   case PIPE_SWIZZLE_W: return fmt->src_w;
   case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
      return is_int ? NVB097_TEXHEAD_BL_X_SOURCE_IN_ONE_INT :
      default:
            }
      static uint32_t
   nv9097_th_bl_0(enum pipe_format format, const enum pipe_swizzle swizzle[4])
   {
      const struct nil_tic_format *fmt = nil_tic_format_for_pipe(format);
            uint32_t source[4];
   for (unsigned i = 0; i < 4; i++)
            uint32_t th_0 = 0;
   TH_NV9097_SET_U(&th_0, 0, COMPONENT_SIZES, fmt->comp_sizes);
   TH_NV9097_SET_U(&th_0, 0, R_DATA_TYPE, fmt->type_r);
   TH_NV9097_SET_U(&th_0, 0, G_DATA_TYPE, fmt->type_g);
   TH_NV9097_SET_U(&th_0, 0, B_DATA_TYPE, fmt->type_b);
   TH_NV9097_SET_U(&th_0, 0, A_DATA_TYPE, fmt->type_a);
   TH_NV9097_SET_U(&th_0, 0, X_SOURCE, source[0]);
   TH_NV9097_SET_U(&th_0, 0, Y_SOURCE, source[1]);
   TH_NV9097_SET_U(&th_0, 0, Z_SOURCE, source[2]);
               }
      static uint32_t
   nvb097_th_bl_0(enum pipe_format format, const enum pipe_swizzle swizzle[4])
   {
      const struct nil_tic_format *fmt = nil_tic_format_for_pipe(format);
            uint32_t source[4];
   for (unsigned i = 0; i < 4; i++)
            uint32_t th_0 = 0;
   TH_NVB097_SET_U(&th_0, BL, COMPONENTS, fmt->comp_sizes);
   TH_NVB097_SET_U(&th_0, BL, R_DATA_TYPE, fmt->type_r);
   TH_NVB097_SET_U(&th_0, BL, G_DATA_TYPE, fmt->type_g);
   TH_NVB097_SET_U(&th_0, BL, B_DATA_TYPE, fmt->type_b);
   TH_NVB097_SET_U(&th_0, BL, A_DATA_TYPE, fmt->type_a);
   TH_NVB097_SET_U(&th_0, BL, X_SOURCE, source[0]);
   TH_NVB097_SET_U(&th_0, BL, Y_SOURCE, source[1]);
   TH_NVB097_SET_U(&th_0, BL, Z_SOURCE, source[2]);
               }
      static uint32_t
   pipe_to_nv_texture_type(enum nil_view_type type)
   {
   #define CASE(NIL, NV) \
      case NIL_VIEW_TYPE_##NIL: return NVB097_TEXHEAD_BL_TEXTURE_TYPE_##NV; \
            switch (type) {
   CASE(1D,             ONE_D);
   CASE(2D,             TWO_D);
   CASE(3D,             THREE_D);
   CASE(CUBE,           CUBEMAP);
   CASE(1D_ARRAY,       ONE_D_ARRAY);
   CASE(2D_ARRAY,       TWO_D_ARRAY);
   CASE(CUBE_ARRAY,     CUBEMAP_ARRAY);
   default: unreachable("Invalid image view type");
         #undef CASE
   }
      static uint32_t
   nil_to_nv9097_multi_sample_count(enum nil_sample_layout sample_layout)
   {
   #define CASE(SIZE) \
      case NIL_SAMPLE_LAYOUT_##SIZE: \
            switch (sample_layout) {
   CASE(1X1);
   CASE(2X1);
   CASE(2X2);
   CASE(4X2);
   CASE(4X4);
   default:
               #undef CASE
   }
      static uint32_t
   nil_to_nvb097_multi_sample_count(enum nil_sample_layout sample_layout)
   {
   #define CASE(SIZE) \
      case NIL_SAMPLE_LAYOUT_##SIZE: \
            switch (sample_layout) {
   CASE(1X1);
   CASE(2X1);
   CASE(2X2);
   CASE(4X2);
   CASE(4X4);
   default:
               #undef CASE
   }
      static inline uint32_t
   nil_max_mip_level(const struct nil_image *image,
         {
      if (view->type != NIL_VIEW_TYPE_3D && view->array_len == 1 &&
      view->base_level == 0 && view->num_levels == 1) {
   /* The Unnormalized coordinates bit in the sampler gets ignored if the
   * referenced image has more than one miplevel.  Fortunately, Vulkan has
   * restrictions requiring the view to be a single-layer single-LOD view
   * in order to use nonnormalizedCoordinates = VK_TRUE in the sampler.
   * From the Vulkan 1.3.255 spec:
   *
   *    "When unnormalizedCoordinates is VK_TRUE, images the sampler is
   *    used with in the shader have the following requirements:
   *
   *     - The viewType must be either VK_IMAGE_VIEW_TYPE_1D or
   *       VK_IMAGE_VIEW_TYPE_2D.
   *     - The image view must have a single layer and a single mip
   *       level."
   *
   * Under these conditions, the view is simply LOD 0 of a single array
   * slice so we don't need to care about aray stride between slices so
   * it's safe to set the number of miplevels to 0 regardless of how many
   * the image actually has.
   */
      } else {
            }
      static struct nil_extent4d
   nil_normalize_extent(const struct nil_image *image,
         {
               extent.width = image->extent_px.width;
   extent.height = image->extent_px.height;
            switch (view->type) {
   case NIL_VIEW_TYPE_1D:
   case NIL_VIEW_TYPE_1D_ARRAY:
   case NIL_VIEW_TYPE_2D:
   case NIL_VIEW_TYPE_2D_ARRAY:
      assert(image->extent_px.depth == 1);
   extent.depth = view->array_len;
      case NIL_VIEW_TYPE_CUBE:
   case NIL_VIEW_TYPE_CUBE_ARRAY:
      assert(image->dim == NIL_IMAGE_DIM_2D);
   assert(view->array_len % 6 == 0);
   extent.depth = view->array_len / 6;
      case NIL_VIEW_TYPE_3D:
      assert(image->dim == NIL_IMAGE_DIM_3D);
   extent.depth = image->extent_px.depth;
      default:
                     }
      static void
   nv9097_nil_image_fill_tic(const struct nil_image *image,
                     {
      assert(util_format_get_blocksize(image->format) ==
         assert(view->base_level + view->num_levels <= image->num_levels);
                                       /* There's no base layer field in the texture header */
   const uint64_t layer_address =
         assert((layer_address & BITFIELD_MASK(9)) == 0);
   TH_NV9097_SET_U(th, 1, OFFSET_LOWER, layer_address & 0xffffffff);
                     const struct nil_tiling *tiling = &image->levels[0].tiling;
   assert(tiling->is_tiled);
            TH_NV9097_SET_E(th, 2, GOBS_PER_BLOCK_WIDTH, ONE_GOB);
   TH_NV9097_SET_U(th, 2, GOBS_PER_BLOCK_HEIGHT, tiling->y_log2);
            TH_NV9097_SET_E(th, 3, LOD_ANISO_QUALITY, LOD_QUALITY_HIGH);
   TH_NV9097_SET_E(th, 3, LOD_ISO_QUALITY, LOD_QUALITY_HIGH);
            const struct nil_extent4d extent = nil_normalize_extent(image, view);
   TH_NV9097_SET_U(th, 4, WIDTH, extent.width);
   TH_NV9097_SET_U(th, 5, HEIGHT, extent.height);
                              TH_NV9097_SET_B(th, 2, S_R_G_B_CONVERSION,
                     /* In the sampler, the two options for FLOAT_COORD_NORMALIZATION are:
      *
   *  - FORCE_UNNORMALIZED_COORDS
   *  - USE_HEADER_SETTING
   *
   * So we set it to normalized in the header and let the sampler select
   * that or force non-normalized.
               TH_NV9097_SET_E(th, 6, ANISO_FINE_SPREAD_FUNC, SPREAD_FUNC_TWO);
            TH_NV9097_SET_U(th, 7, RES_VIEW_MIN_MIP_LEVEL, view->base_level);
   TH_NV9097_SET_U(th, 7, RES_VIEW_MAX_MIP_LEVEL,
            TH_NV9097_SET_U(th, 7, MULTI_SAMPLE_COUNT,
            TH_NV9097_SET_UF(th, 7, MIN_LOD_CLAMP,
               }
      static void
   nvb097_nil_image_fill_tic(const struct nil_image *image,
                     {
      assert(util_format_get_blocksize(image->format) ==
         assert(view->base_level + view->num_levels <= image->num_levels);
                              /* There's no base layer field in the texture header */
   const uint64_t layer_address =
         assert((layer_address & BITFIELD_MASK(9)) == 0);
   TH_NVB097_SET_U(th, BL, ADDRESS_BITS31TO9, (uint32_t)layer_address >> 9);
                     const struct nil_tiling *tiling = &image->levels[0].tiling;
   assert(tiling->is_tiled);
   assert(tiling->gob_height_8);
   TH_NVB097_SET_E(th, BL, GOBS_PER_BLOCK_WIDTH, ONE_GOB);
   TH_NVB097_SET_U(th, BL, GOBS_PER_BLOCK_HEIGHT, tiling->y_log2);
            TH_NVB097_SET_B(th, BL, LOD_ANISO_QUALITY2, true);
   TH_NVB097_SET_E(th, BL, LOD_ANISO_QUALITY, LOD_QUALITY_HIGH);
   TH_NVB097_SET_E(th, BL, LOD_ISO_QUALITY, LOD_QUALITY_HIGH);
            const struct nil_extent4d extent = nil_normalize_extent(image, view);
   TH_NVB097_SET_U(th, BL, WIDTH_MINUS_ONE, extent.width - 1);
   TH_NVB097_SET_U(th, BL, HEIGHT_MINUS_ONE, extent.height - 1);
                              TH_NVB097_SET_B(th, BL, S_R_G_B_CONVERSION,
            TH_NVB097_SET_E(th, BL, SECTOR_PROMOTION, PROMOTE_TO_2_V);
            /* In the sampler, the two options for FLOAT_COORD_NORMALIZATION are:
      *
   *  - FORCE_UNNORMALIZED_COORDS
   *  - USE_HEADER_SETTING
   *
   * So we set it to normalized in the header and let the sampler select
   * that or force non-normalized.
               TH_NVB097_SET_E(th, BL, ANISO_FINE_SPREAD_FUNC, SPREAD_FUNC_TWO);
            TH_NVB097_SET_U(th, BL, RES_VIEW_MIN_MIP_LEVEL, view->base_level);
   TH_NVB097_SET_U(th, BL, RES_VIEW_MAX_MIP_LEVEL,
            TH_NVB097_SET_U(th, BL, MULTI_SAMPLE_COUNT,
            TH_NVB097_SET_UF(th, BL, MIN_LOD_CLAMP,
               }
      static const enum pipe_swizzle IDENTITY_SWIZZLE[4] = {
      PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_Y,
   PIPE_SWIZZLE_Z,
      };
      static void
   nv9097_nil_buffer_fill_tic(uint64_t base_address,
                     {
                        assert(!util_format_is_compressed(format));
            TH_NV9097_SET_U(th, 1, OFFSET_LOWER, base_address);
   TH_NV9097_SET_U(th, 2, OFFSET_UPPER, base_address >> 32);
            TH_NV9097_SET_U(th, 4, WIDTH, num_elements);
                  }
      static void
   nvb097_nil_buffer_fill_tic(uint64_t base_address,
                     {
               assert(!util_format_is_compressed(format));
            TH_NVB097_SET_U(th, 1D, ADDRESS_BITS31TO0, base_address);
   TH_NVB097_SET_U(th, 1D, ADDRESS_BITS47TO32, base_address >> 32);
            TH_NVB097_SET_U(th, 1D, WIDTH_MINUS_ONE_BITS15TO0,
         TH_NVB097_SET_U(th, 1D, WIDTH_MINUS_ONE_BITS31TO16,
                     /* TODO: Do we need this? */
               }
      void
   nil_image_fill_tic(struct nv_device_info *dev,
                     const struct nil_image *image,
   {
      if (dev->cls_eng3d >= MAXWELL_A) {
         } else if (dev->cls_eng3d >= FERMI_A) {
         } else {
            }
      void
   nil_buffer_fill_tic(struct nv_device_info *dev,
                     uint64_t base_address,
   {
      if (dev->cls_eng3d >= MAXWELL_A) {
         } else if (dev->cls_eng3d >= FERMI_A) {
         } else {
            }
