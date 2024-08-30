   /*
   * Copyright © 2014 Jon Turney
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
      #include "windowsgl.h"
   #include "windowsgl_internal.h"
   #include "windowsdriconst.h"
   #include "wgl.h"
      #include <stdio.h>
      /*
   * Window drawable
   */
      static
   HDC window_getdc(windowsDrawable *d)
   {
         }
      static
   void window_releasedc(windowsDrawable *d, HDC dc)
   {
         }
      static struct windowsdrawable_callbacks window_callbacks = {
      .type = WindowsDRIDrawableWindow,
   .getdc = window_getdc,
      };
      /*
   * Pixmap drawable
   */
      static
   HDC pixmap_getdc(windowsDrawable *d)
   {
         }
      static
   void pixmap_releasedc(windowsDrawable *d, HDC dc)
   {
         }
      static struct windowsdrawable_callbacks pixmap_callbacks = {
      .type = WindowsDRIDrawablePixmap,
   .getdc = pixmap_getdc,
      };
      /*
   * Pbuffer drawable
   */
      static
   HDC pbuffer_getdc(windowsDrawable *d)
   {
         }
      static
   void pbuffer_releasedc(windowsDrawable *d, HDC dc)
   {
         }
      static struct windowsdrawable_callbacks pbuffer_callbacks = {
      .type = WindowsDRIDrawablePbuffer,
   .getdc = pbuffer_getdc,
      };
      /*
   *
   */
      windowsDrawable *
   windows_create_drawable(int type, void *handle)
   {
               d = calloc(1, sizeof *d);
   if (d == NULL)
            switch (type)
   {
   case WindowsDRIDrawableWindow:
      d->hWnd = handle;
   d->callbacks = &window_callbacks;
         case WindowsDRIDrawablePixmap:
   {
      BITMAPINFOHEADER *pBmpHeader;
                              // Access file mapping object by a name
   snprintf(name, sizeof(name), "Local\\CYGWINX_WINDOWSDRI_%08x", (unsigned int)(uintptr_t)handle);
   d->hSection = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, name);
   if (!d->hSection)
            // Create a screen-compatible DC
            // Map the shared memory section to access the BITMAPINFOHEADER
   pBmpHeader = (BITMAPINFOHEADER *)MapViewOfFile(d->hSection, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(BITMAPINFOHEADER));
   if (!pBmpHeader)
            // Create a DIB using the file mapping
   d->hDIB = CreateDIBSection(d->dibDC, (BITMAPINFO *) pBmpHeader,
                  // Done with the BITMAPINFOHEADER
            // Select the DIB into the DC
      }
            case WindowsDRIDrawablePbuffer:
      d->hPbuffer = handle;
   d->callbacks = &pbuffer_callbacks;
                  }
      void
   windows_destroy_drawable(windowsDrawable *drawable)
   {
      switch (drawable->callbacks->type)
   {
   case WindowsDRIDrawableWindow:
            case WindowsDRIDrawablePixmap:
   {
      // Select the default DIB into the DC
            // delete the screen-compatible DC
            // Delete the DIB
            // Close the file mapping object
      }
                                       }
