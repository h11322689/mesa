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
      #include "util/format/u_formats.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
      #include "util/format/u_format.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
      #include <GL/gl.h>
   #include "stw_gdishim.h"
   #include "gldrv.h"
   #include "stw_device.h"
   #include "stw_framebuffer.h"
   #include "stw_pixelformat.h"
   #include "stw_tls.h"
   #include "stw_winsys.h"
         struct stw_pf_color_info
   {
      enum pipe_format format;
   struct {
      unsigned char red;
   unsigned char green;
   unsigned char blue;
      } bits;
   struct {
      unsigned char red;
   unsigned char green;
   unsigned char blue;
         };
      struct stw_pf_depth_info
   {
      enum pipe_format format;
   struct {
      unsigned char depth;
         };
         /* NOTE: order matters, since in otherwise equal circumstances the first
   * format listed will get chosen */
      static const struct stw_pf_color_info
   stw_pf_color[] = {
      /* no-alpha */
   { PIPE_FORMAT_B8G8R8X8_UNORM,    { 8,  8,  8,  0}, {16,  8,  0,  0} },
   { PIPE_FORMAT_X8R8G8B8_UNORM,    { 8,  8,  8,  0}, { 8, 16, 24,  0} },
   /* alpha */
   { PIPE_FORMAT_B8G8R8A8_UNORM,    { 8,  8,  8,  8}, {16,  8,  0, 24} },
   { PIPE_FORMAT_A8R8G8B8_UNORM,    { 8,  8,  8,  8}, { 8, 16, 24,  0} },
   /* shallow bit depths */
      #if 0
         #endif
      { PIPE_FORMAT_B5G5R5A1_UNORM,    { 5,  5,  5,  1}, {10,  5,  0, 15} },
      };
      static const struct stw_pf_color_info
   stw_pf_color_extended[] = {
         };
      static const struct stw_pf_depth_info
   stw_pf_depth_stencil[] = {
      /* pure depth */
   { PIPE_FORMAT_Z32_UNORM,   {32, 0} },
   { PIPE_FORMAT_X8Z24_UNORM, {24, 0} },
   { PIPE_FORMAT_Z24X8_UNORM, {24, 0} },
   { PIPE_FORMAT_Z16_UNORM,   {16, 0} },
   /* combined depth-stencil */
   { PIPE_FORMAT_Z24_UNORM_S8_UINT, {24, 8} },
      };
         static const bool
   stw_pf_doublebuffer[] = {
      false,
      };
         static const stw_pfd_flag
   stw_pf_flag[] = {
      stw_pfd_double_buffer,
      };
         const unsigned
   stw_pf_multisample[] = {
      0,
   4,
   8,
      };
         static void
   stw_pixelformat_add(struct stw_device *stw_dev,
                     bool extended,
   const struct stw_pf_color_info *color,
   const struct stw_pf_depth_info *depth,
   unsigned accum,
   {
               assert(util_format_get_component_bits(color->format, UTIL_FORMAT_COLORSPACE_RGB, 0) == color->bits.red);
   assert(util_format_get_component_bits(color->format, UTIL_FORMAT_COLORSPACE_RGB, 1) == color->bits.green);
   assert(util_format_get_component_bits(color->format, UTIL_FORMAT_COLORSPACE_RGB, 2) == color->bits.blue);
   assert(util_format_get_component_bits(color->format, UTIL_FORMAT_COLORSPACE_RGB, 3) == color->bits.alpha);
   assert(util_format_get_component_bits(depth->format, UTIL_FORMAT_COLORSPACE_ZS, 0) == depth->bits.depth);
            pfi = util_dynarray_grow(&stw_dev->pixelformats,
                           pfi->iPixelFormat = util_dynarray_num_elements(&stw_dev->pixelformats, struct stw_pixelformat_info);
   pfi->pfd.nSize = sizeof pfi->pfd;
                     /* TODO: also support non-native pixel formats */
   if (!extended) {
                  /* See http://www.opengl.org/pipeline/article/vol003_7/ */
            if (doublebuffer)
            if (gdi)
                     pfi->pfd.cColorBits =
         pfi->pfd.cRedBits = color->bits.red;
   pfi->pfd.cRedShift = color->shift.red;
   pfi->pfd.cGreenBits = color->bits.green;
   pfi->pfd.cGreenShift = color->shift.green;
   pfi->pfd.cBlueBits = color->bits.blue;
   pfi->pfd.cBlueShift = color->shift.blue;
   pfi->pfd.cAlphaBits = color->bits.alpha;
   pfi->pfd.cAlphaShift = color->shift.alpha;
   pfi->pfd.cAccumBits = 4*accum;
   pfi->pfd.cAccumRedBits = accum;
   pfi->pfd.cAccumGreenBits = accum;
   pfi->pfd.cAccumBlueBits = accum;
   pfi->pfd.cAccumAlphaBits = accum;
   pfi->pfd.cDepthBits = depth->bits.depth;
   pfi->pfd.cStencilBits = depth->bits.stencil;
   pfi->pfd.cAuxBuffers = 0;
   pfi->pfd.iLayerType = 0;
   pfi->pfd.bReserved = 0;
   pfi->pfd.dwLayerMask = 0;
   pfi->pfd.dwVisibleMask = 0;
            /*
   * since gallium frontend can allocate depth/stencil/accum buffers, we provide
   * only color buffers here in the non-zink case, however in the zink case
   * kopper requires that we allocate depth/stencil through the winsys
   */
   pfi->stvis.buffer_mask = ST_ATTACHMENT_FRONT_LEFT_MASK;
   if (doublebuffer)
            pfi->stvis.color_format = color->format;
         #ifdef GALLIUM_ZINK
      if (stw_dev->zink && (depth->bits.depth > 0 || depth->bits.stencil > 0))
      #endif
         pfi->stvis.accum_format = (accum) ?
                     /* WGL_ARB_render_texture */
   if (color->bits.alpha)
                     if (!extended) {
      ++stw_dev->pixelformat_count;
   assert(stw_dev->pixelformat_count ==
               }
         /**
   * Add the depth/stencil/accum/ms variants for a list of color formats.
   */
   static unsigned
   add_color_format_variants(const struct stw_pf_color_info *color_formats,
         {
      struct pipe_screen *screen = stw_dev->screen;
   unsigned cfmt, ms, db, ds, acc, f;
   unsigned bind_flags = PIPE_BIND_RENDER_TARGET;
   unsigned num_added = 0;
            unsigned supported_flags = 0;
   if (stw_dev->stw_winsys && stw_dev->stw_winsys->get_pfd_flags)
            /* Since GLUT for Windows doesn't support MSAA we have an env var
   * to force all pixel formats to have a particular number of samples.
   */
   {
      const char *samples = getenv("WGL_FORCE_MSAA");
   if (!samples) {
      static bool warned = false;
   samples = getenv("SVGA_FORCE_MSAA");
   if (samples && !warned) {
      fprintf(stderr, "*** SVGA_FORCE_MSAA is deprecated; "
                        if (samples)
               if (!extended) {
                  for (ms = 0; ms < ARRAY_SIZE(stw_pf_multisample); ms++) {
               if (force_samples && samples != force_samples)
            for (cfmt = 0; cfmt < num_color_formats; cfmt++) {
      if (!screen->is_format_supported(screen, color_formats[cfmt].format,
                                                                  if (!screen->is_format_supported(screen, depth->format,
                                    for (f = 0; f < ARRAY_SIZE(stw_pf_flag); f++) {
      stw_pfd_flag flag = stw_pf_flag[f];
   if (!(supported_flags & flag) || (flag == stw_pfd_double_buffer && !doublebuffer))
         for (acc = 0; acc < 2; acc++) {
      stw_pixelformat_add(stw_dev, extended, &color_formats[cfmt],
                                             }
         void
   stw_pixelformat_init(void)
   {
                                 /* normal, displayable formats */
   num_formats = add_color_format_variants(stw_pf_color,
                  /* extended, pbuffer-only formats */
   add_color_format_variants(stw_pf_color_extended,
            assert(stw_dev->pixelformat_count <=
            }
         uint
   stw_pixelformat_get_count(HDC hdc)
   {
      if (!stw_init_screen(hdc))
               }
         uint
   stw_pixelformat_get_extended_count(HDC hdc)
   {
      if (!stw_init_screen(hdc))
            return util_dynarray_num_elements(&stw_dev->pixelformats,
      }
         const struct stw_pixelformat_info *
   stw_pixelformat_get_info(int iPixelFormat)
   {
               if (iPixelFormat <= 0) {
                  index = iPixelFormat - 1;
   if (index >= util_dynarray_num_elements(&stw_dev->pixelformats,
                        return util_dynarray_element(&stw_dev->pixelformats,
            }
      /**
   * Return the stw pixel format that most closely matches the pixel format
   * on HDC.
   * Used to get a pixel format when SetPixelFormat() hasn't been called before.
   */
   int
   stw_pixelformat_guess(HDC hdc)
   {
      int iPixelFormat = GetPixelFormat(hdc);
            if (!iPixelFormat)
         if (!DescribePixelFormat(hdc, iPixelFormat, sizeof(pfd), &pfd))
            }
      const struct stw_pixelformat_info *
   stw_pixelformat_get_info_from_hdc(HDC hdc)
   {
      /*
   * GDI only knows about displayable pixel formats, so determine the pixel
   * format from the framebuffer.
   *
   * This also allows to use a OpenGL DLL / ICD without installing.
   */
   struct stw_framebuffer *fb;
   fb = stw_framebuffer_from_hdc(hdc);
   if (fb) {
      const struct stw_pixelformat_info *pfi = fb->pfi;
   stw_framebuffer_unlock(fb);
               /* Applications should call SetPixelFormat before creating a context,
   * but not all do, and the opengl32 runtime seems to use a default
   * pixel format in some cases, so use that.
   */
   int iPixelFormat = stw_pixelformat_guess(hdc);
   if (!iPixelFormat)
               }
         LONG APIENTRY
   DrvDescribePixelFormat(HDC hdc, INT iPixelFormat, ULONG cjpfd,
         {
      uint count;
            if (!stw_dev)
                     if (ppfd == NULL)
            if (cjpfd != sizeof(PIXELFORMATDESCRIPTOR))
            pfi = stw_pixelformat_get_info(iPixelFormat);
   if (!pfi) {
                              }
         BOOL APIENTRY
   DrvDescribeLayerPlane(HDC hdc, INT iPixelFormat, INT iLayerPlane,
         {
      assert(0);
      }
         int APIENTRY
   DrvGetLayerPaletteEntries(HDC hdc, INT iLayerPlane, INT iStart,
         {
      assert(0);
      }
         int APIENTRY
   DrvSetLayerPaletteEntries(HDC hdc, INT iLayerPlane, INT iStart,
         {
      assert(0);
      }
         BOOL APIENTRY
   DrvRealizeLayerPalette(HDC hdc, INT iLayerPlane, BOOL bRealize)
   {
      assert(0);
      }
         /* Only used by the wgl code, but have it here to avoid exporting the
   * pixelformat.h functionality.
   */
   int
   stw_pixelformat_choose(HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd)
   {
      uint count;
   uint index;
   uint bestindex;
            count = stw_pixelformat_get_extended_count(hdc);
   bestindex = 0;
            for (index = 1; index <= count; index++) {
      uint delta = 0;
            if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE) &&
      !!(ppfd->dwFlags & PFD_DOUBLEBUFFER) !=
               /* Selection logic:
   * - Enabling a feature (depth, stencil...) is given highest priority.
   * - Giving as many bits as requested is given medium priority.
   * - Giving no more bits than requested is given lowest priority.
            /* FIXME: Take in account individual channel bits */
   if (ppfd->cColorBits && !pfi->pfd.cColorBits)
         else if (ppfd->cColorBits > pfi->pfd.cColorBits)
         else if (ppfd->cColorBits < pfi->pfd.cColorBits)
            if (ppfd->cDepthBits && !pfi->pfd.cDepthBits)
         else if (ppfd->cDepthBits > pfi->pfd.cDepthBits)
         else if (ppfd->cDepthBits < pfi->pfd.cDepthBits)
            if (ppfd->cStencilBits && !pfi->pfd.cStencilBits)
         else if (ppfd->cStencilBits > pfi->pfd.cStencilBits)
         else if (ppfd->cStencilBits < pfi->pfd.cStencilBits)
            if (ppfd->cAlphaBits && !pfi->pfd.cAlphaBits)
         else if (ppfd->cAlphaBits > pfi->pfd.cAlphaBits)
         else if (ppfd->cAlphaBits < pfi->pfd.cAlphaBits)
            if (delta < bestdelta) {
      bestindex = index;
   bestdelta = delta;
   if (bestdelta == 0)
                     }
