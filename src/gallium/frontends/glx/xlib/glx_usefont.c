   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1995 Thorsten.Ohl @ Physik.TH-Darmstadt.de
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
         /**
   * Fake implementation of glXUseXFont().
   */
      #include <stdlib.h>
   #include <string.h>
   #include <stdio.h>
   #include <GL/glx.h>
   #include "main/errors.h"
         /* Some debugging info.  */
      #ifdef DEBUG
   #include <ctype.h>
      int debug_xfonts = 0;
      static void
   dump_char_struct(XCharStruct * ch, char *prefix)
   {
      printf("%slbearing = %d, rbearing = %d, width = %d\n",
   prefix, ch->lbearing, ch->rbearing, ch->width);
   printf("%sascent = %d, descent = %d, attributes = %u\n",
      }
      static void
   dump_font_struct(XFontStruct * font)
   {
      printf("ascent = %d, descent = %d\n", font->ascent, font->descent);
   printf("char_or_byte2 = (%u,%u)\n",
   font->min_char_or_byte2, font->max_char_or_byte2);
   printf("byte1 = (%u,%u)\n", font->min_byte1, font->max_byte1);
   printf("all_chars_exist = %s\n", font->all_chars_exist ? "True" : "False");
   printf("default_char = %c (\\%03o)\n",
   (char) (isprint(font->default_char) ? font->default_char : ' '),
   font->default_char);
   dump_char_struct(&font->min_bounds, "min> ");
      #if 0
      for (c = font->min_char_or_byte2; c <= font->max_char_or_byte2; c++) {
      char prefix[8];
   sprintf(prefix, "%d> ", c);
         #endif
   }
      static void
   dump_bitmap(unsigned int width, unsigned int height, GLubyte * bitmap)
   {
               printf("    ");
   for (x = 0; x < 8 * width; x++)
         putchar('\n');
   for (y = 0; y < height; y++) {
      printf("%3o:", y);
   putchar((bitmap[width * (height - y - 1) + x / 8] & (1 << (7 - (x %
            ? '*' : '.');
      printf("   ");
   printf("0x%02x, ", bitmap[width * (height - y - 1) + x]);
               }
   #endif /* DEBUG */
         /* Implementation.  */
      /* Fill a BITMAP with a character C from thew current font
      in the graphics context GC.  WIDTH is the width in bytes
                        glPixelStorei (GL_UNPACK_SWAP_BYTES, GL_FALSE);
   glPixelStorei (GL_UNPACK_LSB_FIRST, GL_FALSE);
   glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
   glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
   glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
                  * use only one reusable pixmap with the maximum dimensions.
   * draw the entire font into a single pixmap (careful with
      */
         /*
   * Generate OpenGL-compatible bitmap.
   */
   static void
   fill_bitmap(Display * dpy, Window win, GC gc,
      unsigned int width, unsigned int height,
      {
      XImage *image;
   unsigned int x, y;
   Pixmap pixmap;
            pixmap = XCreatePixmap(dpy, win, 8 * width, height, 1);
   XSetForeground(dpy, gc, 0);
   XFillRectangle(dpy, pixmap, gc, 0, 0, 8 * width, height);
            char2b.byte1 = (c >> 8) & 0xff;
                     image = XGetImage(dpy, pixmap, 0, 0, 8 * width, height, 1, XYPixmap);
   if (image) {
      /* Fill the bitmap (X11 and OpenGL are upside down wrt each other).  */
   for (x = 0; x < 8 * width; x++)
      if (XGetPixel(image, x, y))
         (1 << (7 - (x % 8)));
                     }
      /*
   * determine if a given glyph is valid and return the
   * corresponding XCharStruct.
   */
   static XCharStruct *
   isvalid(XFontStruct * fs, unsigned int which)
   {
      unsigned int rows, pages;
   unsigned int byte1 = 0, byte2 = 0;
            rows = fs->max_byte1 - fs->min_byte1 + 1;
            if (rows == 1) {
      /* "linear" fonts */
   valid = 0;
      }
   else {
      /* "matrix" fonts */
   byte2 = which & 0xff;
   byte1 = which >> 8;
      (fs->max_char_or_byte2 < byte2) ||
      valid = 0;
               if (valid) {
      if (rows == 1) {
      /* "linear" fonts */
      }
   else {
      /* "matrix" fonts */
   i = ((byte1 - fs->min_byte1) * pages) +
            }
         }
   return (&fs->min_bounds);
            }
      }
         PUBLIC void
   glXUseXFont(Font font, int first, int count, int listbase)
   {
      Display *dpy;
   Window win;
   Pixmap pixmap;
   GC gc;
   XGCValues values;
   unsigned long valuemask;
   XFontStruct *fs;
   GLint swapbytes, lsbfirst, rowlength;
   GLint skiprows, skippixels, alignment;
   unsigned int max_width, max_height, max_bm_width, max_bm_height;
   GLubyte *bm;
            dpy = glXGetCurrentDisplay();
   if (!dpy)
         i = DefaultScreen(dpy);
            fs = XQueryFont(dpy, font);
   if (!fs) {
         "Couldn't get font structure information");
                  /* Allocate a bitmap that can fit all characters.  */
   max_width = fs->max_bounds.rbearing - fs->min_bounds.lbearing;
   max_height = fs->max_bounds.ascent + fs->max_bounds.descent;
   max_bm_width = (max_width + 7) / 8;
            bm = malloc((max_bm_width * max_bm_height) * sizeof(GLubyte));
   if (!bm) {
      XFreeFontInfo(NULL, fs, 1);
      "Couldn't allocate bitmap in glXUseXFont()");
               #if 0
      /* get the page info */
   pages = fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1;
   firstchar = (fs->min_byte1 << 8) + fs->min_char_or_byte2;
   lastchar = (fs->max_byte1 << 8) + fs->max_char_or_byte2;
   rows = fs->max_byte1 - fs->min_byte1 + 1;
      #endif
         /* Save the current packing mode for bitmaps.  */
   glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapbytes);
   glGetIntegerv(GL_UNPACK_LSB_FIRST, &lsbfirst);
   glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowlength);
   glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skiprows);
   glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skippixels);
            /* Enforce a standard packing mode which is compatible with
      fill_bitmap() from above.  This is actually the default mode,
      glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
   glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
   glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
            pixmap = XCreatePixmap(dpy, win, 10, 10, 1);
   values.foreground = BlackPixel(dpy, DefaultScreen(dpy));
   values.background = WhitePixel(dpy, DefaultScreen(dpy));
   values.font = fs->fid;
   valuemask = GCForeground | GCBackground | GCFont;
   gc = XCreateGC(dpy, pixmap, valuemask, &values);
         #ifdef DEBUG
      if (debug_xfonts)
      #endif
         for (i = 0; i < count; i++) {
      unsigned int width, height, bm_width, bm_height;
   GLfloat x0, y0, dx, dy;
   XCharStruct *ch;
   int x, y;
   unsigned int c = first + i;
   int list = listbase + i;
            /* check on index validity and get the bounds */
   ch = isvalid(fs, c);
   ch = &fs->max_bounds;
   valid = 0;
         }
   valid = 1;
            #ifdef DEBUG
         char s[7];
   sprintf(s, isprint(c) ? "%c> " : "\\%03o> ", c);
   dump_char_struct(ch, s);
         #endif
            /* glBitmap()' parameters:
         width = ch->rbearing - ch->lbearing;
   height = ch->ascent + ch->descent;
   x0 = -ch->lbearing;
   y0 = ch->descent - 0;	/* XXX used to subtract 1 here */
   /* but that caused a conformace failure */
   dx = ch->width;
            /* X11's starting point.  */
   x = -ch->lbearing;
            /* Round the width to a multiple of eight.  We will use this also
      for the pixmap for capturing the X11 font.  This is slightly
      bm_width = (width + 7) / 8;
            glNewList(list, GL_COMPILE);
      memset(bm, '\0', bm_width * bm_height);
   fill_bitmap(dpy, win, gc, bm_width, bm_height, x, y, c, bm);
      glBitmap(width, height, x0, y0, dx, dy, bm);
   #ifdef DEBUG
   if (debug_xfonts) {
      printf("width/height = %u/%u\n", width, height);
   printf("bm_width/bm_height = %u/%u\n", bm_width, bm_height);
      }
   #endif
         }
   glBitmap(0, 0, 0.0, 0.0, dx, dy, NULL);
         }
               free(bm);
   XFreeFontInfo(NULL, fs, 1);
            /* Restore saved packing modes.  */
   glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbytes);
   glPixelStorei(GL_UNPACK_LSB_FIRST, lsbfirst);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, rowlength);
   glPixelStorei(GL_UNPACK_SKIP_ROWS, skiprows);
   glPixelStorei(GL_UNPACK_SKIP_PIXELS, skippixels);
      }
