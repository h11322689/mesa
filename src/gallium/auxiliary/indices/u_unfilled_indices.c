   /*
   * Copyright 2009 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   * VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
         /*
   * NOTE: This file is not compiled by itself.  It's actually #included
   * by the generated u_unfilled_gen.c file!
   */
      #include "u_indices.h"
   #include "u_indices_priv.h"
   #include "util/u_prim.h"
         static void translate_ubyte_ushort( const void *in,
                                 {
      const uint8_t *in_ub = (const uint8_t *)in;
   uint16_t *out_us = (uint16_t *)out;
   unsigned i;
   for (i = 0; i < out_nr; i++)
      }
      static void generate_linear_ushort( unsigned start,
               {
      uint16_t *out_us = (uint16_t *)out;
   unsigned i;
   for (i = 0; i < nr; i++)
      }
      static void generate_linear_uint( unsigned start,
               {
      unsigned *out_ui = (unsigned *)out;
   unsigned i;
   for (i = 0; i < nr; i++)
      }
         /**
   * Given a primitive type and number of vertices, return the number of vertices
   * needed to draw the primitive with fill mode = PIPE_POLYGON_MODE_LINE using
   * separate lines (MESA_PRIM_LINES).
   */
   static unsigned
   nr_lines(enum mesa_prim prim, unsigned nr)
   {
      switch (prim) {
   case MESA_PRIM_TRIANGLES:
         case MESA_PRIM_TRIANGLE_STRIP:
         case MESA_PRIM_TRIANGLE_FAN:
         case MESA_PRIM_QUADS:
         case MESA_PRIM_QUAD_STRIP:
         case MESA_PRIM_POLYGON:
         /* Note: these cases can't really be handled since drawing lines instead
   * of triangles would also require changing the GS.  But if there's no GS,
   * this should work.
   */
   case MESA_PRIM_TRIANGLES_ADJACENCY:
         case MESA_PRIM_TRIANGLE_STRIP_ADJACENCY:
         default:
      assert(0);
         }
         enum indices_mode
   u_unfilled_translator(enum mesa_prim prim,
                        unsigned in_index_size,
   unsigned nr,
   unsigned unfilled_mode,
      {
      unsigned in_idx;
                              in_idx = in_size_idx(in_index_size);
   *out_index_size = (in_index_size == 4) ? 4 : 2;
            if (unfilled_mode == PIPE_POLYGON_MODE_POINT) {
      *out_prim = MESA_PRIM_POINTS;
            switch (in_index_size) {
   case 1:
      *out_translate = translate_ubyte_ushort;
      case 2:
      *out_translate = translate_memcpy_uint;
      case 4:
      *out_translate = translate_memcpy_ushort;
      default:
      *out_translate = translate_memcpy_uint;
   *out_nr = 0;
   assert(0);
         }
   else {
      assert(unfilled_mode == PIPE_POLYGON_MODE_LINE);
   *out_prim = MESA_PRIM_LINES;
   *out_translate = translate_line[in_idx][out_idx][prim];
   *out_nr = nr_lines( prim, nr );
         }
         /**
   * Utility for converting unfilled polygons into points, lines, triangles.
   * Few drivers have direct support for OpenGL's glPolygonMode.
   * This function helps with converting triangles into points or lines
   * when the front and back fill modes are the same.  When there's
   * different front/back fill modes, that can be handled with the
   * 'draw' module.
   */
   enum indices_mode
   u_unfilled_generator(enum mesa_prim prim,
                        unsigned start,
   unsigned nr,
   unsigned unfilled_mode,
      {
                                 *out_index_size = ((start + nr) > 0xfffe) ? 4 : 2;
            if (unfilled_mode == PIPE_POLYGON_MODE_POINT) {
      if (*out_index_size == 4)
         else
            *out_prim = MESA_PRIM_POINTS;
   *out_nr = nr;
      }
   else {
      assert(unfilled_mode == PIPE_POLYGON_MODE_LINE);
   *out_prim = MESA_PRIM_LINES;
   *out_generate = generate_line[out_idx][prim];
                  }
