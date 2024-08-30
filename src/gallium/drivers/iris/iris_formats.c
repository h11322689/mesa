   /*
   * Copyright © 2017 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * @file iris_formats.c
   *
   * Converts Gallium formats (PIPE_FORMAT_*) to hardware ones (ISL_FORMAT_*).
   * Provides information about which formats support what features.
   */
      #include "util/bitscan.h"
   #include "util/macros.h"
   #include "util/format/u_format.h"
      #include "iris_resource.h"
   #include "iris_screen.h"
      struct iris_format_info
   iris_format_for_usage(const struct intel_device_info *devinfo,
               {
      enum isl_format format = isl_format_for_pipe_format(pformat);
            if (format == ISL_FORMAT_UNSUPPORTED)
                     if (!util_format_is_srgb(pformat)) {
      if (util_format_is_intensity(pformat)) {
         } else if (util_format_is_luminance(pformat)) {
         } else if (util_format_is_luminance_alpha(pformat)) {
         } else if (util_format_is_alpha(pformat)) {
                     /* When faking RGBX pipe formats with RGBA ISL formats, override alpha. */
   if (!util_format_has_alpha(pformat) && fmtl->channels.a.type != ISL_VOID) {
                  if ((usage & ISL_SURF_USAGE_RENDER_TARGET_BIT) &&
      pformat == PIPE_FORMAT_A8_UNORM) {
   /* Most of the hardware A/LA formats are not renderable, except
   * for A8_UNORM.  SURFACE_STATE's shader channel select fields
   * cannot be used to swap RGB and A channels when rendering (as
   * it could impact alpha blending), so we have to use the actual
   * A8_UNORM format when rendering.
   */
   format = ISL_FORMAT_A8_UNORM;
               /* We choose RGBA over RGBX for rendering the hardware doesn't support
   * rendering to RGBX. However, when this internal override is used on Gfx9+,
   * fast clears don't work correctly.
   *
   * i965 fixes this by pretending to not support RGBX formats, and the higher
   * layers of Mesa pick the RGBA format instead. Gallium doesn't work that
   * way, and might choose a different format, like BGRX instead of RGBX,
   * which will also cause problems when sampling from a surface fast cleared
   * as RGBX. So we always choose RGBA instead of RGBX explicitly
   * here.
   */
   if (isl_format_is_rgbx(format) &&
      !isl_format_supports_rendering(devinfo, format)) {
   format = isl_format_rgbx_to_rgba(format);
                  }
      /**
   * The pscreen->is_format_supported() driver hook.
   *
   * Returns true if the given format is supported for the given usage
   * (PIPE_BIND_*) and sample count.
   */
   bool
   iris_is_format_supported(struct pipe_screen *pscreen,
                           enum pipe_format pformat,
   {
      struct iris_screen *screen = (struct iris_screen *) pscreen;
   const struct intel_device_info *devinfo = screen->devinfo;
            if (sample_count > max_samples ||
      !util_is_power_of_two_or_zero(sample_count))
         if (pformat == PIPE_FORMAT_NONE)
            /* Rely on gallium fallbacks for better YUV format support. */
   if (util_format_is_yuv(pformat))
                     if (format == ISL_FORMAT_UNSUPPORTED)
            const struct isl_format_layout *fmtl = isl_format_get_layout(format);
   const bool is_integer = isl_format_has_int_channel(format);
            if (sample_count > 1)
            if (usage & PIPE_BIND_DEPTH_STENCIL) {
      supported &= format == ISL_FORMAT_R32_FLOAT_X8X24_TYPELESS ||
               format == ISL_FORMAT_R32_FLOAT ||
               if (usage & PIPE_BIND_RENDER_TARGET) {
      /* Alpha and luminance-alpha formats other than A8_UNORM are not
   * renderable.  For texturing, we can use R or RG formats with
   * shader channel selects (SCS) to swizzle the data into the correct
   * channels.  But for render targets, the hardware prohibits using
   * SCS to move shader outputs between the RGB and A channels, as it
   * would alter what data is used for alpha blending.
   *
   * For BLORP, we can apply the swizzle in the shader.  But for
   * general rendering, this would mean recompiling the shader, which
   * we'd like to avoid doing.  So we mark these formats non-renderable.
   *
   * We do support A8_UNORM as it's required and is renderable.
   */
   if (pformat != PIPE_FORMAT_A8_UNORM &&
      (util_format_is_alpha(pformat) ||
                        if (isl_format_is_rgbx(format) &&
                           if (!is_integer)
               if (usage & PIPE_BIND_SHADER_IMAGE) {
      /* Dataport doesn't support compression, and we can't resolve an MCS
   * compressed surface.  (Buffer images may have sample count of 0.)
   */
            supported &= isl_format_supports_typed_writes(devinfo, format);
               if (usage & PIPE_BIND_SAMPLER_VIEW) {
      supported &= isl_format_supports_sampling(devinfo, format);
   if (!is_integer)
            /* Don't advertise 3-component RGB formats for non-buffer textures.
   * This ensures that they are renderable from an API perspective since
   * gallium frontends will fall back to RGBA or RGBX, which are
   * renderable.  We want to render internally for copies and blits,
   * even if the application doesn't.
   *
   * Buffer textures don't need to be renderable, so we support real RGB.
   * This is useful for PBO upload, and 32-bit RGB support is mandatory.
   */
   if (target != PIPE_BUFFER)
               if (usage & PIPE_BIND_VERTEX_BUFFER)
            if (usage & PIPE_BIND_INDEX_BUFFER) {
      supported &= format == ISL_FORMAT_R8_UINT ||
                     /* TODO: Support ASTC 5x5 on Gfx9 properly.  This means implementing
   * a complex sampler workaround (see i965's gfx9_apply_astc5x5_wa_flush).
   * Without it, st/mesa will emulate ASTC 5x5 via uncompressed textures.
   */
   if (devinfo->ver == 9 && (format == ISL_FORMAT_ASTC_LDR_2D_5X5_FLT16 ||
                     }
   