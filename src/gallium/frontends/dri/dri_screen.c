   /**************************************************************************
   *
   * Copyright 2009, VMware, Inc.
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
   /*
   * Author: Keith Whitwell <keithw@vmware.com>
   * Author: Jakob Bornecrantz <wallbraker@gmail.com>
   */
      #include "dri_screen.h"
   #include "dri_context.h"
   #include "dri_helpers.h"
      #include "util/u_inlines.h"
   #include "pipe/p_screen.h"
   #include "util/format/u_formats.h"
   #include "pipe-loader/pipe_loader.h"
   #include "frontend/drm_driver.h"
      #include "util/u_debug.h"
   #include "util/u_driconf.h"
   #include "util/format/u_format_s3tc.h"
      #include "state_tracker/st_context.h"
      #define MSAA_VISUAL_MAX_SAMPLES 32
      #undef false
      const __DRIconfigOptionsExtension gallium_config_options = {
      .base = { __DRI_CONFIG_OPTIONS, 2 },
      };
      #define false 0
      void
   dri_init_options(struct dri_screen *screen)
   {
               struct st_config_options *options = &screen->options;
               }
      static unsigned
   dri_loader_get_cap(struct dri_screen *screen, enum dri_loader_cap cap)
   {
      const __DRIdri2LoaderExtension *dri2_loader = screen->dri2.loader;
            if (dri2_loader && dri2_loader->base.version >= 4 &&
      dri2_loader->getCapability)
         if (image_loader && image_loader->base.version >= 2 &&
      image_loader->getCapability)
            }
      /**
   * Creates a set of \c struct gl_config that a driver will expose.
   *
   * A set of \c struct gl_config will be created based on the supplied
   * parameters.  The number of modes processed will be 2 *
   * \c num_depth_stencil_bits * \c num_db_modes.
   *
   * For the most part, data is just copied from \c depth_bits, \c stencil_bits,
   * \c db_modes, and \c visType into each \c struct gl_config element.
   * However, the meanings of \c fb_format and \c fb_type require further
   * explanation.  The \c fb_format specifies which color components are in
   * each pixel and what the default order is.  For example, \c GL_RGB specifies
   * that red, green, blue are available and red is in the "most significant"
   * position and blue is in the "least significant".  The \c fb_type specifies
   * the bit sizes of each component and the actual ordering.  For example, if
   * \c GL_UNSIGNED_SHORT_5_6_5_REV is specified with \c GL_RGB, bits [15:11]
   * are the blue value, bits [10:5] are the green value, and bits [4:0] are
   * the red value.
   *
   * One sublte issue is the combination of \c GL_RGB  or \c GL_BGR and either
   * of the \c GL_UNSIGNED_INT_8_8_8_8 modes.  The resulting mask values in the
   * \c struct gl_config structure is \b identical to the \c GL_RGBA or
   * \c GL_BGRA case, except the \c alphaMask is zero.  This means that, as
   * far as this routine is concerned, \c GL_RGB with \c GL_UNSIGNED_INT_8_8_8_8
   * still uses 32-bits.
   *
   * If in doubt, look at the tables used in the function.
   *
   * \param ptr_to_modes  Pointer to a pointer to a linked list of
   *                      \c struct gl_config.  Upon completion, a pointer to
   *                      the next element to be process will be stored here.
   *                      If the function fails and returns \c GL_FALSE, this
   *                      value will be unmodified, but some elements in the
   *                      linked list may be modified.
   * \param format        Mesa mesa_format enum describing the pixel format
   * \param depth_bits    Array of depth buffer sizes to be exposed.
   * \param stencil_bits  Array of stencil buffer sizes to be exposed.
   * \param num_depth_stencil_bits  Number of entries in both \c depth_bits and
   *                      \c stencil_bits.
   * \param db_modes      Array of double buffer modes.
   * \param num_db_modes  Number of entries in \c db_modes.
   * \param msaa_samples  Array of msaa sample count. 0 represents a visual
   *                      without a multisample buffer.
   * \param num_msaa_modes Number of entries in \c msaa_samples.
   * \param enable_accum  Add an accum buffer to the configs
   * \param color_depth_match Whether the color depth must match the zs depth
   *                          This forces 32-bit color to have 24-bit depth, and
   *                          16-bit color to have 16-bit depth.
   *
   * \returns
   * Pointer to any array of pointers to the \c __DRIconfig structures created
   * for the specified formats.  If there is an error, \c NULL is returned.
   * Currently the only cause of failure is a bad parameter (i.e., unsupported
   * \c format).
   */
   static __DRIconfig **
   driCreateConfigs(mesa_format format,
                  const uint8_t * depth_bits, const uint8_t * stencil_bits,
   unsigned num_depth_stencil_bits,
      {
      static const struct {
      uint32_t masks[4];
      } format_table[] = {
      /* MESA_FORMAT_B5G6R5_UNORM */
   {{ 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
   { 11, 5, 0, -1 }},
   /* MESA_FORMAT_B8G8R8X8_UNORM */
   {{ 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
   { 16, 8, 0, -1 }},
   /* MESA_FORMAT_B8G8R8A8_UNORM */
   {{ 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 },
   { 16, 8, 0, 24 }},
   /* MESA_FORMAT_B10G10R10X2_UNORM */
   {{ 0x3FF00000, 0x000FFC00, 0x000003FF, 0x00000000 },
   { 20, 10, 0, -1 }},
   /* MESA_FORMAT_B10G10R10A2_UNORM */
   {{ 0x3FF00000, 0x000FFC00, 0x000003FF, 0xC0000000 },
   { 20, 10, 0, 30 }},
   /* MESA_FORMAT_R8G8B8A8_UNORM */
   {{ 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 },
   { 0, 8, 16, 24 }},
   /* MESA_FORMAT_R8G8B8X8_UNORM */
   {{ 0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000 },
   { 0, 8, 16, -1 }},
   /* MESA_FORMAT_R10G10B10X2_UNORM */
   {{ 0x000003FF, 0x000FFC00, 0x3FF00000, 0x00000000 },
   { 0, 10, 20, -1 }},
   /* MESA_FORMAT_R10G10B10A2_UNORM */
   {{ 0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000 },
   { 0, 10, 20, 30 }},
   /* MESA_FORMAT_RGBX_FLOAT16 */
   {{ 0, 0, 0, 0},
   { 0, 16, 32, -1 }},
   /* MESA_FORMAT_RGBA_FLOAT16 */
   {{ 0, 0, 0, 0},
   { 0, 16, 32, 48 }},
   /* MESA_FORMAT_B5G5R5A1_UNORM */
   {{ 0x00007C00, 0x000003E0, 0x0000001F, 0x00008000 },
   { 10, 5, 0, 15 }},
   /* MESA_FORMAT_R5G5B5A1_UNORM */
   {{ 0x0000001F, 0x000003E0, 0x00007C00, 0x00008000 },
   { 0, 5, 10, 15 }},
   /* MESA_FORMAT_B4G4R4A4_UNORM */
   {{ 0x00000F00, 0x000000F0, 0x0000000F, 0x0000F000 },
   { 8, 4, 0, 12 }},
   /* MESA_FORMAT_R4G4B4A4_UNORM */
   {{ 0x0000000F, 0x000000F0, 0x00000F00, 0x0000F000 },
               const uint32_t * masks;
   const int * shifts;
   __DRIconfig **configs, **c;
   struct gl_config *modes;
   unsigned i, j, k, h;
   unsigned num_modes;
   unsigned num_accum_bits = (enable_accum) ? 2 : 1;
   int red_bits;
   int green_bits;
   int blue_bits;
   int alpha_bits;
   bool is_srgb;
            switch (format) {
   case MESA_FORMAT_B5G6R5_UNORM:
      masks = format_table[0].masks;
   shifts = format_table[0].shifts;
      case MESA_FORMAT_B8G8R8X8_UNORM:
   case MESA_FORMAT_B8G8R8X8_SRGB:
      masks = format_table[1].masks;
   shifts = format_table[1].shifts;
      case MESA_FORMAT_B8G8R8A8_UNORM:
   case MESA_FORMAT_B8G8R8A8_SRGB:
      masks = format_table[2].masks;
   shifts = format_table[2].shifts;
      case MESA_FORMAT_R8G8B8A8_UNORM:
   case MESA_FORMAT_R8G8B8A8_SRGB:
      masks = format_table[5].masks;
   shifts = format_table[5].shifts;
      case MESA_FORMAT_R8G8B8X8_UNORM:
   case MESA_FORMAT_R8G8B8X8_SRGB:
      masks = format_table[6].masks;
   shifts = format_table[6].shifts;
      case MESA_FORMAT_B10G10R10X2_UNORM:
      masks = format_table[3].masks;
   shifts = format_table[3].shifts;
      case MESA_FORMAT_B10G10R10A2_UNORM:
      masks = format_table[4].masks;
   shifts = format_table[4].shifts;
      case MESA_FORMAT_RGBX_FLOAT16:
      masks = format_table[9].masks;
   shifts = format_table[9].shifts;
      case MESA_FORMAT_RGBA_FLOAT16:
      masks = format_table[10].masks;
   shifts = format_table[10].shifts;
      case MESA_FORMAT_R10G10B10X2_UNORM:
      masks = format_table[7].masks;
   shifts = format_table[7].shifts;
      case MESA_FORMAT_R10G10B10A2_UNORM:
      masks = format_table[8].masks;
   shifts = format_table[8].shifts;
      case MESA_FORMAT_B5G5R5A1_UNORM:
      masks = format_table[11].masks;
   shifts = format_table[11].shifts;
      case MESA_FORMAT_R5G5B5A1_UNORM:
      masks = format_table[12].masks;
   shifts = format_table[12].shifts;
      case MESA_FORMAT_B4G4R4A4_UNORM:
      masks = format_table[13].masks;
   shifts = format_table[13].shifts;
      case MESA_FORMAT_R4G4B4A4_UNORM:
      masks = format_table[14].masks;
   shifts = format_table[14].shifts;
      default:
      fprintf(stderr, "[%s:%u] Unknown framebuffer type %s (%d).\n",
         __func__, __LINE__,
               red_bits = _mesa_get_format_bits(format, GL_RED_BITS);
   green_bits = _mesa_get_format_bits(format, GL_GREEN_BITS);
   blue_bits = _mesa_get_format_bits(format, GL_BLUE_BITS);
   alpha_bits = _mesa_get_format_bits(format, GL_ALPHA_BITS);
   is_srgb = _mesa_is_format_srgb(format);
            num_modes = num_depth_stencil_bits * num_db_modes * num_accum_bits * num_msaa_modes;
   configs = calloc(num_modes + 1, sizeof *configs);
   if (configs == NULL)
            c = configs;
   for ( k = 0 ; k < num_depth_stencil_bits ; k++ ) {
      for ( i = 0 ; i < num_db_modes ; i++ ) {
         for ( h = 0 ; h < num_msaa_modes; h++ ) {
      for ( j = 0 ; j < num_accum_bits ; j++ ) {
      if (color_depth_match &&
         (depth_bits[k] || stencil_bits[k])) {
   /* Depth can really only be 0, 16, 24, or 32. A 32-bit
   * color format still matches 24-bit depth, as there
   * is an implicit 8-bit stencil. So really we just
   * need to make sure that color/depth are both 16 or
   * both non-16.
   */
                                             memset(modes, 0, sizeof *modes);
   modes->floatMode = is_float;
   modes->redBits   = red_bits;
   modes->greenBits = green_bits;
   modes->blueBits  = blue_bits;
   modes->alphaBits = alpha_bits;
   modes->redMask   = masks[0];
   modes->greenMask = masks[1];
   modes->blueMask  = masks[2];
   modes->alphaMask = masks[3];
   modes->redShift   = shifts[0];
   modes->greenShift = shifts[1];
   modes->blueShift  = shifts[2];
                        modes->accumRedBits   = 16 * j;
                                                                  }
               }
      static __DRIconfig **
   driConcatConfigs(__DRIconfig **a, __DRIconfig **b)
   {
      __DRIconfig **all;
            if (a == NULL || a[0] == NULL)
         else if (b == NULL || b[0] == NULL)
            i = 0;
   while (a[i] != NULL)
         j = 0;
   while (b[j] != NULL)
            all = malloc((i + j + 1) * sizeof *all);
   index = 0;
   for (i = 0; a[i] != NULL; i++)
         for (j = 0; b[j] != NULL; j++)
                  free(a);
               }
         static const __DRIconfig **
   dri_fill_in_modes(struct dri_screen *screen)
   {
      static const mesa_format mesa_formats[] = {
      MESA_FORMAT_B10G10R10A2_UNORM,
   MESA_FORMAT_B10G10R10X2_UNORM,
   MESA_FORMAT_R10G10B10A2_UNORM,
   MESA_FORMAT_R10G10B10X2_UNORM,
   MESA_FORMAT_B8G8R8A8_UNORM,
   MESA_FORMAT_B8G8R8X8_UNORM,
   MESA_FORMAT_B8G8R8A8_SRGB,
   MESA_FORMAT_B8G8R8X8_SRGB,
   MESA_FORMAT_B5G6R5_UNORM,
   MESA_FORMAT_RGBA_FLOAT16,
            /* The 32-bit RGBA format must not precede the 32-bit BGRA format.
   * Likewise for RGBX and BGRX.  Otherwise, the GLX client and the GLX
   * server may disagree on which format the GLXFBConfig represents,
   * resulting in swapped color channels.
   *
   * The problem, as of 2017-05-30:
   * When matching a GLXFBConfig to a __DRIconfig, GLX ignores the channel
   * order and chooses the first __DRIconfig with the expected channel
   * sizes. Specifically, GLX compares the GLXFBConfig's and __DRIconfig's
   * __DRI_ATTRIB_{CHANNEL}_SIZE but ignores __DRI_ATTRIB_{CHANNEL}_MASK.
   *
   * EGL does not suffer from this problem. It correctly compares the
   * channel masks when matching EGLConfig to __DRIconfig.
            /* Required by Android, for HAL_PIXEL_FORMAT_RGBA_8888. */
            /* Required by Android, for HAL_PIXEL_FORMAT_RGBX_8888. */
            /* Required by Android, for HAL_PIXEL_FORMAT_RGBA_8888. */
            /* Required by Android, for HAL_PIXEL_FORMAT_RGBX_8888. */
            MESA_FORMAT_B5G5R5A1_UNORM,
   MESA_FORMAT_R5G5B5A1_UNORM,
   MESA_FORMAT_B4G4R4A4_UNORM,
      };
   __DRIconfig **configs = NULL;
   uint8_t depth_bits_array[5];
   uint8_t stencil_bits_array[5];
   unsigned depth_buffer_factor;
   unsigned i;
   struct pipe_screen *p_screen = screen->base.screen;
   bool pf_z16, pf_x8z24, pf_z24x8, pf_s8z24, pf_z24s8, pf_z32;
   bool mixed_color_depth;
   bool allow_rgba_ordering;
   bool allow_rgb10;
                     if (driQueryOptionb(&screen->dev->option_cache, "always_have_depth_buffer")) {
      /* all visuals will have a depth buffer */
      }
   else {
      depth_bits_array[0] = 0;
   stencil_bits_array[0] = 0;
               allow_rgba_ordering = dri_loader_get_cap(screen, DRI_LOADER_CAP_RGBA_ORDERING);
   allow_rgb10 = driQueryOptionb(&screen->dev->option_cache, "allow_rgb10_configs");
            pf_x8z24 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z24X8_UNORM,
               pf_z24x8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_X8Z24_UNORM,
               pf_s8z24 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z24_UNORM_S8_UINT,
               pf_z24s8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_S8_UINT_Z24_UNORM,
               pf_z16 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z16_UNORM,
               pf_z32 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z32_UNORM,
                  if (pf_z16) {
      depth_bits_array[depth_buffer_factor] = 16;
      }
   if (pf_x8z24 || pf_z24x8) {
      depth_bits_array[depth_buffer_factor] = 24;
   stencil_bits_array[depth_buffer_factor++] = 0;
      }
   if (pf_s8z24 || pf_z24s8) {
      depth_bits_array[depth_buffer_factor] = 24;
   stencil_bits_array[depth_buffer_factor++] = 8;
      }
   if (pf_z32) {
      depth_bits_array[depth_buffer_factor] = 32;
               mixed_color_depth =
            /* Add configs. */
   for (unsigned f = 0; f < ARRAY_SIZE(mesa_formats); f++) {
      mesa_format format = mesa_formats[f];
   __DRIconfig **new_configs = NULL;
   unsigned num_msaa_modes = 0; /* includes a single-sample mode */
            /* Expose only BGRA ordering if the loader doesn't support RGBA ordering. */
   if (!allow_rgba_ordering &&
      (format == MESA_FORMAT_R8G8B8A8_UNORM ||
   format == MESA_FORMAT_R8G8B8X8_UNORM ||
   format == MESA_FORMAT_R8G8B8A8_SRGB  ||
   format == MESA_FORMAT_R8G8B8X8_SRGB  ||
   format == MESA_FORMAT_R5G5B5A1_UNORM ||
               if (!allow_rgb10 &&
      (format == MESA_FORMAT_B10G10R10A2_UNORM ||
   format == MESA_FORMAT_B10G10R10X2_UNORM ||
   format == MESA_FORMAT_R10G10B10A2_UNORM ||
               if (!allow_fp16 &&
      (format == MESA_FORMAT_RGBA_FLOAT16 ||
               if (!p_screen->is_format_supported(p_screen, format,
                              for (i = 1; i <= MSAA_VISUAL_MAX_SAMPLES; i++) {
               if (p_screen->is_format_supported(p_screen, format,
                                 if (num_msaa_modes) {
      /* Single-sample configs with an accumulation buffer. */
   new_configs = driCreateConfigs(format,
                                          /* Multi-sample configs without an accumulation buffer. */
   if (num_msaa_modes > 1) {
      new_configs = driCreateConfigs(format,
                                                   if (configs == NULL) {
      debug_printf("%s: driCreateConfigs failed\n", __func__);
                  }
      /**
   * Roughly the converse of dri_fill_in_modes.
   */
   void
   dri_fill_st_visual(struct st_visual *stvis,
               {
               if (!mode)
            /* Deduce the color format. */
   switch (mode->redMask) {
   case 0:
      /* Formats > 32 bpp */
   assert(mode->floatMode);
   if (mode->alphaShift > -1) {
      assert(mode->alphaShift == 48);
      } else {
         }
         case 0x3FF00000:
      if (mode->alphaMask) {
      assert(mode->alphaMask == 0xC0000000);
      } else {
         }
         case 0x000003FF:
      if (mode->alphaMask) {
      assert(mode->alphaMask == 0xC0000000);
      } else {
         }
         case 0x00FF0000:
      if (mode->alphaMask) {
      assert(mode->alphaMask == 0xFF000000);
   stvis->color_format = mode->sRGBCapable ?
            } else {
      stvis->color_format = mode->sRGBCapable ?
            }
         case 0x000000FF:
      if (mode->alphaMask) {
      assert(mode->alphaMask == 0xFF000000);
   stvis->color_format = mode->sRGBCapable ?
            } else {
      stvis->color_format = mode->sRGBCapable ?
            }
         case 0x0000F800:
      stvis->color_format = PIPE_FORMAT_B5G6R5_UNORM;
         case 0x00007C00:
      assert(!mode->sRGBCapable);
   if (mode->alphaMask)
         else
               case 0x0000001F:
      assert(!mode->sRGBCapable);
   if (mode->alphaMask)
         else
               case 0x00000F00:
      assert(!mode->sRGBCapable);
   if (mode->alphaMask)
         else
               case 0x0000000F:
      assert(!mode->sRGBCapable);
   if (mode->alphaMask)
         else
               default:
      assert(!"unsupported visual: invalid red mask");
               if (mode->samples > 0) {
      if (debug_get_bool_option("DRI_NO_MSAA", false))
         else
               switch (mode->depthBits) {
   default:
   case 0:
      stvis->depth_stencil_format = PIPE_FORMAT_NONE;
      case 16:
      stvis->depth_stencil_format = PIPE_FORMAT_Z16_UNORM;
      case 24:
      if (mode->stencilBits == 0) {
      stvis->depth_stencil_format = (screen->d_depth_bits_last) ?
            } else {
      stvis->depth_stencil_format = (screen->sd_depth_bits_last) ?
            }
      case 32:
      stvis->depth_stencil_format = PIPE_FORMAT_Z32_UNORM;
               stvis->accum_format = (mode->accumRedBits > 0) ?
            stvis->buffer_mask |= ST_ATTACHMENT_FRONT_LEFT_MASK;
   if (mode->doubleBufferMode) {
         }
   if (mode->stereoMode) {
      stvis->buffer_mask |= ST_ATTACHMENT_FRONT_RIGHT_MASK;
   if (mode->doubleBufferMode)
               if (mode->depthBits > 0 || mode->stencilBits > 0)
            }
      static bool
   dri_get_egl_image(struct pipe_frontend_screen *fscreen,
               {
      struct dri_screen *screen = (struct dri_screen *)fscreen;
   __DRIimage *img = NULL;
            if (screen->lookup_egl_image_validated) {
         } else if (screen->lookup_egl_image) {
                  if (!img)
            stimg->texture = NULL;
   pipe_resource_reference(&stimg->texture, img->texture);
   map = dri2_get_mapping_by_fourcc(img->dri_fourcc);
   stimg->format = map ? map->pipe_format : img->texture->format;
   stimg->level = img->level;
   stimg->layer = img->layer;
            if (img->imported_dmabuf && map) {
      /* Guess sized internal format for dma-bufs. Could be used
   * by EXT_EGL_image_storage.
   */
   mesa_format mesa_format = driImageFormatToGLFormat(map->dri_format);
      } else {
                  stimg->yuv_color_space = img->yuv_color_space;
               }
      static bool
   dri_validate_egl_image(struct pipe_frontend_screen *fscreen,
         {
                  }
      static int
   dri_get_param(struct pipe_frontend_screen *fscreen,
         {
         }
      void
   dri_release_screen(struct dri_screen * screen)
   {
               if (screen->base.screen) {
      screen->base.screen->destroy(screen->base.screen);
               if (screen->dev) {
      pipe_loader_release(&screen->dev, 1);
                  }
      void
   dri_destroy_screen(struct dri_screen *screen)
   {
               free(screen->options.force_gl_vendor);
   free(screen->options.force_gl_renderer);
            driDestroyOptionCache(&screen->optionCache);
            /* The caller in dri_util preserves the fd ownership */
      }
      static void
   dri_postprocessing_init(struct dri_screen *screen)
   {
               for (i = 0; i < PP_FILTERS; i++) {
      screen->pp_enabled[i] = driQueryOptioni(&screen->dev->option_cache,
         }
      static void
   dri_set_background_context(struct st_context *st,
         {
      struct dri_context *ctx = (struct dri_context *)st->frontend_context;
   const __DRIbackgroundCallableExtension *backgroundCallable =
            if (backgroundCallable)
            if (ctx->hud)
      }
      const __DRIconfig **
   dri_init_screen(struct dri_screen *screen,
         {
      screen->base.screen = pscreen;
   screen->base.get_egl_image = dri_get_egl_image;
   screen->base.get_param = dri_get_param;
            if (screen->validate_egl_image)
            if (pscreen->get_param(pscreen, PIPE_CAP_NPOT_TEXTURES))
         else
                     st_api_query_versions(&screen->base,
                        &screen->options,
            }
      /* vim: set sw=3 ts=8 sts=3 expandtab: */
