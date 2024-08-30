   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   *
   **************************************************************************/
      /**
   * @file
   * GDI software rasterizer support.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include <windows.h>
      #include "util/format/u_formats.h"
   #include "pipe/p_context.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "frontend/sw_winsys.h"
   #include "gdi_sw_winsys.h"
   #include "wgl/stw_gdishim.h"
         struct gdi_sw_displaytarget
   {
      enum pipe_format format;
   unsigned width;
   unsigned height;
                                 };
         /** Cast wrapper */
   static inline struct gdi_sw_displaytarget *
   gdi_sw_displaytarget( struct sw_displaytarget *buf )
   {
         }
         static bool
   gdi_sw_is_displaytarget_format_supported( struct sw_winsys *ws,
               {
      switch(format) {
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
            /* TODO: Support other formats possible with BMPs, as described in 
   * http://msdn.microsoft.com/en-us/library/dd183376(VS.85).aspx */
         default:
            }
         static void *
   gdi_sw_displaytarget_map(struct sw_winsys *ws,
               {
                  }
         static void
   gdi_sw_displaytarget_unmap(struct sw_winsys *ws,
         {
      }
         static void
   gdi_sw_displaytarget_destroy(struct sw_winsys *winsys,
         {
               align_free(gdt->data);
      }
         static struct sw_displaytarget *
   gdi_sw_displaytarget_create(struct sw_winsys *winsys,
                                       {
      struct gdi_sw_displaytarget *gdt;
   unsigned cpp;
   unsigned bpp;
      gdt = CALLOC_STRUCT(gdi_sw_displaytarget);
   if(!gdt)
            gdt->format = format;
   gdt->width = width;
            bpp = util_format_get_blocksizebits(format);
   cpp = util_format_get_blocksize(format);
      gdt->stride = align(width * cpp, alignment);
   gdt->size = gdt->stride * height;
      gdt->data = align_malloc(gdt->size, alignment);
   if(!gdt->data)
            gdt->bmi.bV5Size = sizeof(BITMAPV5HEADER);
   gdt->bmi.bV5Width = gdt->stride / cpp;
   gdt->bmi.bV5Height = -(long)height;
   gdt->bmi.bV5Planes = 1;
   gdt->bmi.bV5BitCount = bpp;
   gdt->bmi.bV5Compression = BI_RGB;
   gdt->bmi.bV5SizeImage = 0;
   gdt->bmi.bV5XPelsPerMeter = 0;
   gdt->bmi.bV5YPelsPerMeter = 0;
   gdt->bmi.bV5ClrUsed = 0;
            if (format == PIPE_FORMAT_B5G6R5_UNORM) {
      gdt->bmi.bV5Compression = BI_BITFIELDS;
   gdt->bmi.bV5RedMask = 0xF800;
   gdt->bmi.bV5GreenMask = 0x07E0;
               *stride = gdt->stride;
         no_data:
         no_gdt:
         }
         static struct sw_displaytarget *
   gdi_sw_displaytarget_from_handle(struct sw_winsys *winsys,
                     {
      assert(0);
      }
         static bool
   gdi_sw_displaytarget_get_handle(struct sw_winsys *winsys,
               {
      assert(0);
      }
         void
   gdi_sw_display( struct sw_winsys *winsys,
               {
               StretchDIBits(hDC,
                  }
      static void
   gdi_sw_displaytarget_display(struct sw_winsys *winsys, 
                     {
      /* nasty:
   */
               }
         static void
   gdi_sw_destroy(struct sw_winsys *winsys)
   {
         }
      struct sw_winsys *
   gdi_create_sw_winsys(void)
   {
               winsys = CALLOC_STRUCT(sw_winsys);
   if(!winsys)
            winsys->destroy = gdi_sw_destroy;
   winsys->is_displaytarget_format_supported = gdi_sw_is_displaytarget_format_supported;
   winsys->displaytarget_create = gdi_sw_displaytarget_create;
   winsys->displaytarget_from_handle = gdi_sw_displaytarget_from_handle;
   winsys->displaytarget_get_handle = gdi_sw_displaytarget_get_handle;
   winsys->displaytarget_map = gdi_sw_displaytarget_map;
   winsys->displaytarget_unmap = gdi_sw_displaytarget_unmap;
   winsys->displaytarget_display = gdi_sw_displaytarget_display;
               }
   