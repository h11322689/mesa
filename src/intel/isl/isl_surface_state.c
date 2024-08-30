   /*
   * Copyright 2016 Intel Corporation
   *
   *  Permission is hereby granted, free of charge, to any person obtaining a
   *  copy of this software and associated documentation files (the "Software"),
   *  to deal in the Software without restriction, including without limitation
   *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
   *  and/or sell copies of the Software, and to permit persons to whom the
   *  Software is furnished to do so, subject to the following conditions:
   *
   *  The above copyright notice and this permission notice (including the next
   *  paragraph) shall be included in all copies or substantial portions of the
   *  Software.
   *
   *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   *  IN THE SOFTWARE.
   */
      #include <stdint.h>
      #define __gen_address_type uint64_t
   #define __gen_user_data void
      static uint64_t
   __gen_combine_address(__attribute__((unused)) void *data,
               {
         }
      #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
      #include "isl_priv.h"
   #include "isl_genX_helpers.h"
      #if GFX_VER >= 8
   static const uint8_t isl_encode_tiling[] = {
      [ISL_TILING_LINEAR]  = LINEAR,
      #if GFX_VERx10 >= 125
      [ISL_TILING_4]       = TILE4,
      #else
      [ISL_TILING_Y0]      = YMAJOR,
   [ISL_TILING_ICL_Yf]  = YMAJOR,
   [ISL_TILING_ICL_Ys]  = YMAJOR,
   [ISL_TILING_SKL_Yf]  = YMAJOR,
      #endif /* GFX_VERx10 < 125 */
   #if GFX_VER <= 11
         #endif
   };
   #endif
      #if GFX_VER >= 9 && GFX_VERx10 <= 120
   static const uint8_t isl_tiling_encode_trmode[] = {
      [ISL_TILING_Y0]         = NONE,
   [ISL_TILING_SKL_Yf]     = TILEYF,
   [ISL_TILING_SKL_Ys]     = TILEYS,
   [ISL_TILING_ICL_Yf]     = TILEYF,
      };
   #endif
      #if GFX_VER >= 7
   static const uint32_t isl_encode_multisample_layout[] = {
      [ISL_MSAA_LAYOUT_NONE]           = MSFMT_MSS,
   [ISL_MSAA_LAYOUT_INTERLEAVED]    = MSFMT_DEPTH_STENCIL,
      };
   #endif
      #if GFX_VER >= 12
   static const uint32_t isl_encode_aux_mode[] = {
      [ISL_AUX_USAGE_NONE] = AUX_NONE,
   [ISL_AUX_USAGE_MC] = AUX_NONE,
   [ISL_AUX_USAGE_MCS] = AUX_CCS_E,
   [ISL_AUX_USAGE_FCV_CCS_E] = AUX_CCS_E,
   [ISL_AUX_USAGE_CCS_E] = AUX_CCS_E,
   [ISL_AUX_USAGE_HIZ_CCS_WT] = AUX_CCS_E,
   [ISL_AUX_USAGE_MCS_CCS] = AUX_MCS_LCE,
      };
   #elif GFX_VER >= 9
   static const uint32_t isl_encode_aux_mode[] = {
      [ISL_AUX_USAGE_NONE] = AUX_NONE,
   [ISL_AUX_USAGE_HIZ] = AUX_HIZ,
   [ISL_AUX_USAGE_MCS] = AUX_CCS_D,
   [ISL_AUX_USAGE_CCS_D] = AUX_CCS_D,
      };
   #elif GFX_VER >= 8
   static const uint32_t isl_encode_aux_mode[] = {
      [ISL_AUX_USAGE_NONE] = AUX_NONE,
   [ISL_AUX_USAGE_HIZ] = AUX_HIZ,
   [ISL_AUX_USAGE_MCS] = AUX_MCS,
      };
   #endif
      static uint8_t
   get_surftype(enum isl_surf_dim dim, isl_surf_usage_flags_t usage)
   {
      switch (dim) {
   default:
         case ISL_SURF_DIM_1D:
      assert(!(usage & ISL_SURF_USAGE_CUBE_BIT));
      case ISL_SURF_DIM_2D:
      if ((usage & ISL_SURF_USAGE_CUBE_BIT) &&
      (usage & ISL_SURF_USAGE_TEXTURE_BIT)) {
   /* We need SURFTYPE_CUBE to make cube sampling work */
      } else {
      /* Everything else (render and storage) treat cubes as plain
   * 2D array textures
   */
         case ISL_SURF_DIM_3D:
      assert(!(usage & ISL_SURF_USAGE_CUBE_BIT));
         }
      #if GFX_VERx10 >= 125
   static uint8_t
   get_media_compression_format(enum isl_format format,
         {
               /* From Bspec 43868, Enumeration_MediaCompressionFormat:
   *
   *    Luma P010 has MSB of 0 while chroma P010 has MSB of 1.
   *    Luma P016 has MSB of 0 while chroma P016 has MSB of 1.
   *    Luma NV12 has MSB of 0 while chroma NV12 has MSB of 1.
   */
   switch (format) {
   case ISL_FORMAT_PLANAR_420_8: /* NV12 */
      assert(plane_bpb == 8 || plane_bpb == 16);
            /* drm_fourcc.h defines the chroma plane of NV12 as 16-bpb */
      case ISL_FORMAT_PLANAR_420_10:
   case ISL_FORMAT_PLANAR_420_12:
   case ISL_FORMAT_PLANAR_420_16:
      assert(plane_bpb == 16 || plane_bpb == 32);
            /* drm_fourcc.h defines the chroma plane of P01X as 32-bpb */
      default:
            }
   #endif
      void
   isl_genX(surf_fill_state_s)(const struct isl_device *dev, void *state,
         {
   #ifndef NDEBUG
      isl_surf_usage_flags_t _base_usage =
      info->view->usage & (ISL_SURF_USAGE_RENDER_TARGET_BIT |
            /* They may only specify one of the above bits at a time */
   assert(__builtin_popcount(_base_usage) == 1);
   /* The only other allowed bit is ISL_SURF_USAGE_CUBE_BIT */
      #endif
         if (info->surf->dim == ISL_SURF_DIM_3D) {
      assert(info->view->base_array_layer + info->view->array_len <=
      } else {
      assert(info->view->base_array_layer + info->view->array_len <=
                                 if (info->view->usage & ISL_SURF_USAGE_RENDER_TARGET_BIT)
         else if (info->view->usage & ISL_SURF_USAGE_TEXTURE_BIT)
            /* From the Sky Lake PRM Vol. 2d, RENDER_SURFACE_STATE::SurfaceFormat
   *
   *    This field cannot be a compressed (BC*, DXT*, FXT*, ETC*, EAC*)
   *    format if the Surface Type is SURFTYPE_1D
   */
   if (info->surf->dim == ISL_SURF_DIM_1D)
            if (isl_format_is_compressed(info->surf->format)) {
      /* You're not allowed to make a view of a compressed format with any
   * format other than the surface format.  None of the userspace APIs
   * allow for this directly and doing so would mess up a number of
   * surface parameters such as Width, Height, and alignments.  Ideally,
   * we'd like to assert that the two formats match.  However, we have an
   * S3TC workaround that requires us to do reinterpretation.  So assert
   * that they're at least the same bpb and block size.
   */
   ASSERTED const struct isl_format_layout *surf_fmtl =
         ASSERTED const struct isl_format_layout *view_fmtl =
                     /* We could be attempting to upload blocks of compressed data via an
   * uncompressed view, blocksize will not match there.
   */
   if (isl_format_is_compressed(info->view->format)) {
      assert(surf_fmtl->bw == view_fmtl->bw);
                        #if GFX_VER >= 12
      /* The BSpec description of this field says:
   *
   *    "This bit field, when set, indicates if the resource is created as
   *    Depth/Stencil resource."
   *
   *    "SW must set this bit for any resource that was created with
   *    Depth/Stencil resource flag. Setting this bit allows HW to properly
   *    interpret the data-layout for various cases. For any resource that's
   *    created without Depth/Stencil resource flag, it must be reset."
   *
   * Even though the docs for this bit seem to imply that it's required for
   * anything which might have been used for depth/stencil, empirical
   * evidence suggests that it only affects CCS compression usage.  There are
   * a few things which back this up:
   *
   *  1. The docs are also pretty clear that this bit was added as part
   *     of enabling Gfx12 depth/stencil lossless compression.
   *
   *  2. The only new difference between depth/stencil and color images on
   *     Gfx12 (where the bit was added) is how they treat CCS compression.
   *     All other differences such as alignment requirements and MSAA layout
   *     are already covered by other bits.
   *
   * Under these assumptions, it makes sense for ISL to model this bit as
   * being an extension of AuxiliarySurfaceMode where STC_CCS and HIZ_CCS_WT
   * are indicated by AuxiliarySurfaceMode == CCS_E and DepthStencilResource
   * == true.
   */
   s.DepthStencilResource = info->aux_usage == ISL_AUX_USAGE_HIZ_CCS_WT ||
      #endif
      #if GFX_VER <= 5
      s.ColorBufferComponentWriteDisables = info->write_disables;
      #else
         #endif
      #if GFX_VERx10 == 75
      s.IntegerSurfaceFormat =
      #endif
         assert(info->surf->logical_level0_px.width > 0 &&
            s.Width = info->surf->logical_level0_px.width - 1;
            /* In the gfx6 PRM Volume 1 Part 1: Graphics Core, Section 7.18.3.7.1
   * (Surface Arrays For all surfaces other than separate stencil buffer):
   *
   * "[DevSNB] Errata: Sampler MSAA Qpitch will be 4 greater than the value
   *  calculated in the equation above , for every other odd Surface Height
   *  starting from 1 i.e. 1,5,9,13"
   *
   * Since this Qpitch errata only impacts the sampler, we have to adjust the
   * input for the rendering surface to achieve the same qpitch. For the
   * affected heights, we increment the height by 1 for the rendering
   * surface.
   */
   if (GFX_VER == 6 && (info->view->usage & ISL_SURF_USAGE_RENDER_TARGET_BIT) &&
      info->surf->samples > 1 &&
   (info->surf->logical_level0_px.height % 4) == 1)
         switch (s.SurfaceType) {
   case SURFTYPE_1D:
   case SURFTYPE_2D:
      /* From the Ivy Bridge PRM >> RENDER_SURFACE_STATE::MinimumArrayElement:
   *
   *    "If Number of Multisamples is not MULTISAMPLECOUNT_1, this field
   *    must be set to zero if this surface is used with sampling engine
   *    messages."
   *
   * This restriction appears to exist only on Ivy Bridge.
   */
   if (GFX_VERx10 == 70 && !ISL_DEV_IS_BAYTRAIL(dev) &&
      (info->view->usage & ISL_SURF_USAGE_TEXTURE_BIT) &&
                        /* From the Broadwell PRM >> RENDER_SURFACE_STATE::Depth:
   *
   *    For SURFTYPE_1D, 2D, and CUBE: The range of this field is reduced
   *    by one for each increase from zero of Minimum Array Element. For
   *    example, if Minimum Array Element is set to 1024 on a 2D surface,
   *    the range of this field is reduced to [0,1023].
   *
   * In other words, 'Depth' is the number of array layers.
   */
            /* From the Broadwell PRM >> RENDER_SURFACE_STATE::RenderTargetViewExtent:
   *
   *    For Render Target and Typed Dataport 1D and 2D Surfaces:
   *    This field must be set to the same value as the Depth field.
   */
   if (info->view->usage & (ISL_SURF_USAGE_RENDER_TARGET_BIT |
                  case SURFTYPE_CUBE:
      s.MinimumArrayElement = info->view->base_array_layer;
   /* Same as SURFTYPE_2D, but divided by 6 */
   s.Depth = info->view->array_len / 6 - 1;
   if (info->view->usage & (ISL_SURF_USAGE_RENDER_TARGET_BIT |
                  case SURFTYPE_3D:
      /* From the Broadwell PRM >> RENDER_SURFACE_STATE::Depth:
   *
   *    If the volume texture is MIP-mapped, this field specifies the
   *    depth of the base MIP level.
   */
            /* From the Broadwell PRM >> RENDER_SURFACE_STATE::RenderTargetViewExtent:
   *
   *    For Render Target and Typed Dataport 3D Surfaces: This field
   *    indicates the extent of the accessible 'R' coordinates minus 1 on
   *    the LOD currently being rendered to.
   *
   * The docs specify that this only matters for render targets and
   * surfaces used with typed dataport messages.  Prior to Ivy Bridge, the
   * Depth field has more bits than RenderTargetViewExtent so we can have
   * textures with more levels than we can render to.  In order to prevent
   * assert-failures in the packing function below, we only set the field
   * when it's actually going to be used by the hardware.
   *
   * The MinimumArrayElement field is ignored by all hardware
   * prior to Sky Lake when texturing, and drivers are responsible
   * for validating the correctness of this parameter.
   * KHR_gl_texture_3D_image requires this functionality.
   */
   s.MinimumArrayElement = info->view->base_array_layer;
   s.RenderTargetViewExtent = info->view->array_len - 1;
      default:
               #if GFX_VER >= 7
      if (INTEL_NEEDS_WA_1806565034) {
      /* Wa_1806565034:
   *
   *    "Only set SurfaceArray if arrayed surface is > 1."
   *
   * Since this is a performance workaround, we only enable it when robust
   * image access is disabled. Otherwise layered robust access is not
   * specification compliant.
   */
   s.SurfaceArray = info->surf->dim != ISL_SURF_DIM_3D &&
      } else {
            #endif
         if (info->view->usage & ISL_SURF_USAGE_RENDER_TARGET_BIT) {
      /* For render target surfaces, the hardware interprets field
   * MIPCount/LOD as LOD. The Broadwell PRM says:
   *
   *    MIPCountLOD defines the LOD that will be rendered into.
   *    SurfaceMinLOD is ignored.
   */
   s.MIPCountLOD = info->view->base_level;
      } else {
      /* For non render target surfaces, the hardware interprets field
   * MIPCount/LOD as MIPCount.  The range of levels accessible by the
   * sampler engine is [SurfaceMinLOD, SurfaceMinLOD + MIPCountLOD].
   */
   s.SurfaceMinLOD = info->view->base_level;
            #if GFX_VER >= 9
         #endif
      #if GFX_VERx10 >= 125
      /* Setting L1 caching policy to Write-back mode. */
      #endif
      #if GFX_VER >= 6
      const struct isl_extent3d image_align =
            #if GFX_VER >= 7
         #endif
   #endif
         if (info->surf->dim_layout == ISL_DIM_LAYOUT_GFX9_1D) {
      /* For gfx9 1-D textures, surface pitch is ignored */
      } else {
               #if GFX_VER >= 8
         #elif GFX_VER == 7
      s.SurfaceArraySpacing = info->surf->array_pitch_span ==
      #endif
      #if GFX_VER >= 9 && GFX_VERx10 <= 120
               /* From the SKL+ PRMs, RENDER_SURFACE_STATE:TileMode,
   *
   *    If Surface Format is ASTC*, this field must be TILEMODE_YMAJOR.
   */
   if (isl_format_get_layout(info->view->format)->txc == ISL_TXC_ASTC)
            s.TileMode = isl_encode_tiling[info->surf->tiling];
   if (isl_tiling_is_std_y(info->surf->tiling))
      #elif GFX_VER >= 8
      assert(isl_format_get_layout(info->view->format)->txc != ISL_TXC_ASTC);
   assert(!isl_tiling_is_std_y(info->surf->tiling));
      #else
      s.TiledSurface = info->surf->tiling != ISL_TILING_LINEAR,
   s.TileWalk = info->surf->tiling == ISL_TILING_Y0 ? TILEWALK_YMAJOR :
      #endif
      #if GFX_VER >= 8
         #else
         #endif
      #if GFX_VER >= 11 && GFX_VERx10 < 125
      /* We've seen dEQP failures when enabling this bit with UINT formats,
   * which particularly affects blorp_copy() operations.  It shouldn't
   * have any effect on UINT textures anyway, so disable it for them.
   */
   s.EnableUnormPathInColorPipe =
      #endif
         s.CubeFaceEnablePositiveZ = 1;
   s.CubeFaceEnableNegativeZ = 1;
   s.CubeFaceEnablePositiveY = 1;
   s.CubeFaceEnableNegativeY = 1;
   s.CubeFaceEnablePositiveX = 1;
         #if GFX_VER >= 6
      /* From the Broadwell PRM for "Number of Multisamples":
   *
   *    "If this field is any value other than MULTISAMPLECOUNT_1, Surface
   *    Min LOD, Mip Count / LOD, and Resource Min LOD must be set to zero."
   *
   * This is fine because no 3D API allows multisampling and mipmapping at
   * the same time.
   */
   if (info->surf->samples > 1) {
      assert(info->view->min_lod_clamp == 0);
   assert(info->view->base_level == 0);
      }
      #if GFX_VER >= 7
      s.MultisampledSurfaceStorageFormat =
      #endif
   #endif
      #if GFX_VER >= 7
         #else
         #endif
      #if (GFX_VERx10 >= 75)
      if (info->view->usage & ISL_SURF_USAGE_RENDER_TARGET_BIT)
            s.ShaderChannelSelectRed = (enum GENX(ShaderChannelSelect)) info->view->swizzle.r;
   s.ShaderChannelSelectGreen = (enum GENX(ShaderChannelSelect)) info->view->swizzle.g;
   s.ShaderChannelSelectBlue = (enum GENX(ShaderChannelSelect)) info->view->swizzle.b;
      #else
         #endif
               #if GFX_VER >= 6
         #endif
      #if GFX_VERx10 >= 45
      if (info->x_offset_sa != 0 || info->y_offset_sa != 0) {
      /* There are fairly strict rules about when the offsets can be used.
   * These are mostly taken from the Sky Lake PRM documentation for
   * RENDER_SURFACE_STATE.
   */
   assert(info->surf->tiling != ISL_TILING_LINEAR);
   assert(info->surf->dim == ISL_SURF_DIM_2D);
   assert(isl_is_pow2(isl_format_get_layout(info->view->format)->bpb));
   assert(info->surf->levels == 1);
   assert(info->surf->logical_level0_px.array_len == 1);
            if (GFX_VER >= 8) {
      /* Broadwell added more rules. */
   assert(info->surf->samples == 1);
   if (isl_format_get_layout(info->view->format)->bpb == 8)
         if (isl_format_get_layout(info->view->format)->bpb == 16)
         #if GFX_VER >= 7
         #endif
               const unsigned x_div = 4;
   const unsigned y_div = GFX_VER >= 8 ? 4 : 2;
   assert(info->x_offset_sa % x_div == 0);
   assert(info->y_offset_sa % y_div == 0);
   s.XOffset = info->x_offset_sa / x_div;
      #else
      assert(info->x_offset_sa == 0);
      #endif
      #if GFX_VER >= 7
      if (info->aux_usage != ISL_AUX_USAGE_NONE) {
      /* Check valid aux usages per-gen */
   if (GFX_VER >= 12) {
      assert(info->aux_usage == ISL_AUX_USAGE_MCS ||
         info->aux_usage == ISL_AUX_USAGE_CCS_E ||
   info->aux_usage == ISL_AUX_USAGE_FCV_CCS_E ||
   info->aux_usage == ISL_AUX_USAGE_MC ||
   info->aux_usage == ISL_AUX_USAGE_HIZ_CCS_WT ||
      } else if (GFX_VER >= 9) {
      assert(info->aux_usage == ISL_AUX_USAGE_HIZ ||
         info->aux_usage == ISL_AUX_USAGE_MCS ||
      } else if (GFX_VER >= 8) {
      assert(info->aux_usage == ISL_AUX_USAGE_HIZ ||
            } else if (GFX_VER >= 7) {
      assert(info->aux_usage == ISL_AUX_USAGE_MCS ||
               /* The docs don't appear to say anything whatsoever about compression
   * and the data port.  Testing seems to indicate that the data port
   * completely ignores the AuxiliarySurfaceMode field.
   *
   * On gfx12 HDC supports compression.
   */
   if (GFX_VER < 12)
            if (isl_surf_usage_is_depth(info->surf->usage))
            if (isl_surf_usage_is_stencil(info->surf->usage))
            if (isl_aux_usage_has_hiz(info->aux_usage)) {
      /* For Gfx8-10, there are some restrictions around sampling from HiZ.
   * The Skylake PRM docs for RENDER_SURFACE_STATE::AuxiliarySurfaceMode
   * say:
   *
   *    "If this field is set to AUX_HIZ, Number of Multisamples must
   *    be MULTISAMPLECOUNT_1, and Surface Type cannot be SURFTYPE_3D."
   *
   * On Gfx12, the docs are a bit less obvious but the restriction is
   * the same.  The limitation isn't called out explicitly but the docs
   * for the CCS_E value of RENDER_SURFACE_STATE::AuxiliarySurfaceMode
   * say:
   *
   *    "If Number of multisamples > 1, programming this value means
   *    MSAA compression is enabled for that surface. Auxiliary surface
   *    is MSC with tile y."
   *
   * Since this interpretation ignores whether the surface is
   * depth/stencil or not and since multisampled depth buffers use
   * ISL_MSAA_LAYOUT_INTERLEAVED which is incompatible with MCS
   * compression, this means that we can't even specify MSAA depth CCS
   * in RENDER_SURFACE_STATE::AuxiliarySurfaceMode.
                                 /* The format must be one of the following: */
   switch (info->view->format) {
   case ISL_FORMAT_R32_FLOAT:
   case ISL_FORMAT_R24_UNORM_X8_TYPELESS:
   case ISL_FORMAT_R16_UNORM:
         default:
      assert(!"Incompatible HiZ Sampling format");
            #if GFX_VERx10 >= 125
         if (info->aux_usage == ISL_AUX_USAGE_MC) {
      s.CompressionFormat =
      } else {
      s.CompressionFormat =
      #endif
   #if GFX_VER >= 12
                  /* The Tiger Lake PRM for RENDER_SURFACE_STATE::DecompressInL3 says:
   *
   *    When this field is set to 1h, the associated compressible surface,
   *    when accessed by sampler and data-port, will be uncompressed in
   *    L3. If the surface is not compressible, this bit field is ignored.
   *
   * The sampler's decompressor seems to lack support for some types of
   * format re-interpretation. Use the more capable decompressor for these
   * cases.
   */
   s.DecompressInL3 =
         #endif
   #if GFX_VER >= 9
         /* Some CCS aux usages have format restrictions. The Skylake PRM doc for
   * RENDER_SURFACE_STATE::AuxiliarySurfaceMode says:
   *
   *    If Number of Multisamples is MULTISAMPLECOUNT_1, AUX_CCS_E setting
   *    is only allowed if Surface Format is supported for Render Target
   *    Compression. This setting enables render target compression.
   *
   * If CCS_E is in use, the format must support it.
   */
   if (isl_aux_usage_has_ccs_e(info->aux_usage))
            /* It also says:
   *
   *    If Number of Multisamples is MULTISAMPLECOUNT_1, AUX_CCS_D setting
   *    is only allowed if Surface Format supported for Fast Clear. In
   *    addition, if the surface is bound to the sampling engine, Surface
   *    Format must be supported for Render Target Compression for
   *    surfaces bound to the sampling engine. For render target surfaces,
   *    this setting disables render target compression. For sampling
   *    engine surfaces, this mode behaves the same as AUX_CCS_E.
   *
   * If CCS_D is in use while rendering, the format must support it. If
   * it's in use while sampling, the format must support CCS_E.
   */
   if (info->aux_usage == ISL_AUX_USAGE_CCS_D) {
      if (info->view->usage & ISL_SURF_USAGE_RENDER_TARGET_BIT) {
         } else {
      assert(info->view->usage & ISL_SURF_USAGE_TEXTURE_BIT);
         #endif
   #if GFX_VER >= 8
         #else
         #endif
               /* The auxiliary buffer info is filled when it's usable by the HW.
   *
   * Starting with Gfx12, the only form of compression that can be used
   * with RENDER_SURFACE_STATE which requires an aux surface is MCS.
   * HiZ still requires a surface but the HiZ surface can only be
   * accessed through 3DSTATE_HIER_DEPTH_BUFFER.
   *
   * On all earlier hardware, an aux surface is required for all forms
   * of compression.
   */
   if ((GFX_VER < 12 && info->aux_usage != ISL_AUX_USAGE_NONE) ||
                        struct isl_tile_info tile_info;
   isl_surf_get_tile_info(info->aux_surf, &tile_info);
   uint32_t pitch_in_tiles =
            s.AuxiliarySurfaceBaseAddress = info->aux_address;
      #if GFX_VER >= 8
         /* Auxiliary surfaces in ISL have compressed formats but the hardware
   * doesn't expect our definition of the compression, it expects qpitch
   * in units of samples on the main surface.
   */
   s.AuxiliarySurfaceQPitch =
   #endif
         #endif
      #if GFX_VER >= 8 && GFX_VER < 11
      /* From the CHV PRM, Volume 2d, page 321 (RENDER_SURFACE_STATE dword 0
   * bit 9 "Sampler L2 Bypass Mode Disable" Programming Notes):
   *
   *    This bit must be set for the following surface types: BC2_UNORM
   *    BC3_UNORM BC5_UNORM BC5_SNORM BC7_UNORM
   */
   if (GFX_VER >= 9 || dev->info->platform == INTEL_PLATFORM_CHV) {
      switch (info->view->format) {
   case ISL_FORMAT_BC2_UNORM:
   case ISL_FORMAT_BC3_UNORM:
   case ISL_FORMAT_BC5_UNORM:
   case ISL_FORMAT_BC5_SNORM:
   case ISL_FORMAT_BC7_UNORM:
      s.SamplerL2BypassModeDisable = true;
      default:
      /* From the SKL PRM, Programming Note under Sampler Output Channel
   * Mapping:
   *
   *    If a surface has an associated HiZ Auxiliary surface, the
   *    Sampler L2 Bypass Mode Disable field in the RENDER_SURFACE_STATE
   *    must be set.
   */
   if (GFX_VER >= 9 && info->aux_usage == ISL_AUX_USAGE_HIZ)
                  #endif
         if (isl_aux_usage_has_fast_clears(info->aux_usage)) {
      #if GFX_VER >= 10
               #else
         #endif
            #if GFX_VER == 11
         /*
   * From BXML > GT > Shared Functions > vol5c Shared Functions >
   * [Structure] RENDER_SURFACE_STATE [BDW+] > ClearColorConversionEnable:
   *
   *   Project: Gfx11
   *
   *   "Enables Pixel backend hw to convert clear values into native format
   *    and write back to clear address, so that display and sampler can use
   *    the converted value for resolving fast cleared RTs."
   *
   * Summary:
   *   Clear color conversion must be enabled if the clear color is stored
   *   indirectly and fast color clears are enabled.
   */
   if (info->use_clear_address) {
         #endif
      #if GFX_VER >= 12
         #elif GFX_VER >= 9
         if (!info->use_clear_address) {
      s.RedClearColor = info->clear_color.u32[0];
   s.GreenClearColor = info->clear_color.u32[1];
   s.BlueClearColor = info->clear_color.u32[2];
      #elif GFX_VER >= 7
         /* Prior to Sky Lake, we only have one bit for the clear color which
   * gives us 0 or 1 in whatever the surface's format happens to be.
   */
   if (isl_format_has_int_channel(info->view->format)) {
      for (unsigned i = 0; i < 4; i++) {
      assert(info->clear_color.u32[i] == 0 ||
      }
   s.RedClearColor = info->clear_color.u32[0] != 0;
   s.GreenClearColor = info->clear_color.u32[1] != 0;
   s.BlueClearColor = info->clear_color.u32[2] != 0;
      } else {
      for (unsigned i = 0; i < 4; i++) {
      assert(info->clear_color.f32[i] == 0.0f ||
      }
   s.RedClearColor = info->clear_color.f32[0] != 0.0f;
   s.GreenClearColor = info->clear_color.f32[1] != 0.0f;
   s.BlueClearColor = info->clear_color.f32[2] != 0.0f;
      #endif
                  }
      void
   isl_genX(buffer_fill_state_s)(const struct isl_device *dev, void *state,
         {
               /* Uniform and Storage buffers need to have surface size not less that the
   * aligned 32-bit size of the buffer. To calculate the array length on
   * unsized arrays in StorageBuffer the last 2 bits store the padding size
   * added to the surface, so we can calculate latter the original buffer
   * size to know the number of elements.
   *
   *  surface_size = isl_align(buffer_size, 4) +
   *                 (isl_align(buffer_size) - buffer_size)
   *
   *  buffer_size = (surface_size & ~3) - (surface_size & 3)
   */
   if ((info->format == ISL_FORMAT_RAW  ||
      info->stride_B < isl_format_get_layout(info->format)->bpb / 8) &&
   !info->is_scratch) {
   assert(info->stride_B == 1);
   uint64_t aligned_size = isl_align(buffer_size, 4);
                        assert(num_elements > 0);
   if (info->format == ISL_FORMAT_RAW) {
         } else {
      /* From the IVB PRM, SURFACE_STATE::Height,
   *
   *    For typed buffer and structured buffer surfaces, the number
   *    of entries in the buffer ranges from 1 to 2^27.
   */
                                    #if GFX_VERx10 >= 125
      if (info->is_scratch) {
      /* From the BSpec:
   *
   *    "For surfaces of type SURFTYPE_SCRATCH, valid range of pitch is:
   *    [63,262143] -> [64B, 256KB].  Also, for SURFTYPE_SCRATCH, the
   *    pitch must be a multiple of 64bytes."
   */
   assert(info->format == ISL_FORMAT_RAW);
   assert(info->stride_B % 64 == 0);
   assert(info->stride_B <= 256 * 1024);
         #else
         #endif
               #if GFX_VER >= 6
         #if GFX_VERx10 >= 125
         #elif GFX_VER >= 7
      s.SurfaceHorizontalAlignment = isl_encode_halign(4);
      #endif
   #endif
      #if GFX_VER >= 9
      s.Height = ((num_elements - 1) >> 7) & 0x3fff;
   s.Width = (num_elements - 1) & 0x7f;
      #elif GFX_VER >= 7
      s.Height = ((num_elements - 1) >> 7) & 0x3fff;
   s.Width = (num_elements - 1) & 0x7f;
      #else
      s.Height = ((num_elements - 1) >> 7) & 0x1fff;
   s.Width = (num_elements - 1) & 0x7f;
      #endif
      #if GFX_VER >= 6
         #endif
      #if (GFX_VER >= 8)
         #else
         #endif
      #if (GFX_VER >= 8)
         #else
         #endif
            #if GFX_VER >= 6
         #endif
      #if GFX_VER >= 9
      /* Store the buffer size in the upper dword of the AUX surface base
   * address. Only enabled on Gfx9+ since Gfx8 has an Atom version with only
   * 32bits of address space.
   */
   if (dev->buffer_length_in_aux_addr)
      #else
         #endif
      #if GFX_VERx10 >= 125
      /* Setting L1 caching policy to Write-back mode. */
      #endif
      #if (GFX_VERx10 >= 75)
      s.ShaderChannelSelectRed = (enum GENX(ShaderChannelSelect)) info->swizzle.r;
   s.ShaderChannelSelectGreen = (enum GENX(ShaderChannelSelect)) info->swizzle.g;
   s.ShaderChannelSelectBlue = (enum GENX(ShaderChannelSelect)) info->swizzle.b;
      #endif
            }
      void
   isl_genX(null_fill_state_s)(const struct isl_device *dev, void *state,
         {
      struct GENX(RENDER_SURFACE_STATE) s = {
      .SurfaceType = SURFTYPE_NULL,
   /* We previously had this format set to B8G8R8A8_UNORM but ran into
   * hangs on IVB. R32_UINT seems to work for everybody.
   *
   * https://gitlab.freedesktop.org/mesa/mesa/-/issues/1872
   */
   #if GFX_VER >= 7
         #endif
   #if GFX_VERx10 >= 125
         #elif GFX_VER >= 8
         #else
         .TiledSurface = true,
   #endif
   #if GFX_VER >= 6
         #endif
   #if GFX_VER == 7
         /* According to PRMs: "Volume 4 Part 1: Subsystem and Cores – Shared
   * Functions"
   *
   * RENDER_SURFACE_STATE::Surface Vertical Alignment
   *
   *    "This field must be set to VALIGN_4 for all tiled Y Render Target
   *     surfaces."
   *
   * Affect IVB, HSW.
   */
   #endif
         .MIPCountLOD = info->levels,
   .Width = info->size.width - 1,
   .Height = info->size.height - 1,
   .Depth = info->size.depth - 1,
   #if GFX_VER <= 5
         .MinimumArrayElement = info->minimum_array_element,
   #endif
      };
      }
