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
      #include "u_indices.h"
   #include "u_indices_priv.h"
      static void translate_byte_to_ushort( const void *in,
                                 {
      uint8_t *src = (uint8_t *)in + start;
   uint16_t *dst = out;
   while (out_nr--) {
            }
      enum mesa_prim
   u_index_prim_type_convert(unsigned hw_mask, enum mesa_prim prim, bool pv_matches)
   {
      if ((hw_mask & (1<<prim)) && pv_matches)
            switch (prim) {
   case MESA_PRIM_POINTS:
         case MESA_PRIM_LINES:
   case MESA_PRIM_LINE_STRIP:
   case MESA_PRIM_LINE_LOOP:
         case MESA_PRIM_TRIANGLES:
   case MESA_PRIM_TRIANGLE_STRIP:
   case MESA_PRIM_TRIANGLE_FAN:
   case MESA_PRIM_QUADS:
   case MESA_PRIM_QUAD_STRIP:
      if ((hw_mask & (1<<MESA_PRIM_QUADS)) && pv_matches)
         else
      case MESA_PRIM_POLYGON:
         case MESA_PRIM_LINES_ADJACENCY:
   case MESA_PRIM_LINE_STRIP_ADJACENCY:
         case MESA_PRIM_TRIANGLES_ADJACENCY:
   case MESA_PRIM_TRIANGLE_STRIP_ADJACENCY:
         case MESA_PRIM_PATCHES:
         default:
      assert(0);
      }
      }
      /**
   * Translate indexes when a driver can't support certain types
   * of drawing.  Example include:
   * - Translate 1-byte indexes into 2-byte indexes
   * - Translate MESA_PRIM_QUADS into MESA_PRIM_TRIANGLES when the hardware
   *   doesn't support the former.
   * - Translate from first provoking vertex to last provoking vertex and
   *   vice versa.
   *
   * Note that this function is used for indexed primitives.
   *
   * \param hw_mask  mask of (1 << MESA_PRIM_x) flags indicating which types
   *                 of primitives are supported by the hardware.
   * \param prim  incoming MESA_PRIM_x
   * \param in_index_size  bytes per index value (1, 2 or 4)
   * \param nr  number of incoming vertices
   * \param in_pv  incoming provoking vertex convention (PV_FIRST or PV_LAST)
   * \param out_pv  desired provoking vertex convention (PV_FIRST or PV_LAST)
   * \param prim_restart  whether primitive restart is disable or enabled
   * \param out_prim  returns new MESA_PRIM_x we'll translate to
   * \param out_index_size  returns bytes per new index value (2 or 4)
   * \param out_nr  returns number of new vertices
   * \param out_translate  returns the translation function to use by the caller
   */
   enum indices_mode
   u_index_translator(unsigned hw_mask,
                     enum mesa_prim prim,
   unsigned in_index_size,
   unsigned nr,
   unsigned in_pv,
   unsigned out_pv,
   unsigned prim_restart,
   enum mesa_prim *out_prim,
   {
      unsigned in_idx;
   unsigned out_idx;
            assert(in_index_size == 1 ||
                           in_idx = in_size_idx(in_index_size);
   *out_index_size = u_index_size_convert(in_index_size);
            if ((hw_mask & (1<<prim)) &&
         {
      if (in_index_size == 4)
         else if (in_index_size == 2)
         else
            *out_prim = prim;
               }
   *out_prim = u_index_prim_type_convert(hw_mask, prim, in_pv == out_pv);
   *out_translate = (*out_prim == MESA_PRIM_QUADS ? translate_quads : translate)
                     }
      unsigned
   u_index_count_converted_indices(unsigned hw_mask, bool pv_matches, enum mesa_prim prim, unsigned nr)
   {
      if ((hw_mask & (1<<prim)) && pv_matches)
            switch (prim) {
   case MESA_PRIM_POINTS:
   case MESA_PRIM_PATCHES:
         case MESA_PRIM_LINES:
         case MESA_PRIM_LINE_STRIP:
         case MESA_PRIM_LINE_LOOP:
         case MESA_PRIM_TRIANGLES:
         case MESA_PRIM_TRIANGLE_STRIP:
         case MESA_PRIM_TRIANGLE_FAN:
         case MESA_PRIM_QUADS:
         case MESA_PRIM_QUAD_STRIP:
         case MESA_PRIM_POLYGON:
         case MESA_PRIM_LINES_ADJACENCY:
         case MESA_PRIM_LINE_STRIP_ADJACENCY:
         case MESA_PRIM_TRIANGLES_ADJACENCY:
         case MESA_PRIM_TRIANGLE_STRIP_ADJACENCY:
         default:
      assert(0);
      }
      }
         /**
   * If a driver does not support a particular gallium primitive type
   * (such as MESA_PRIM_QUAD_STRIP) this function can be used to help
   * convert the primitive into a simpler type (like MESA_PRIM_TRIANGLES).
   *
   * The generator functions generates a number of ushort or uint indexes
   * for drawing the new type of primitive.
   *
   * Note that this function is used for non-indexed primitives.
   *
   * \param hw_mask  a bitmask of (1 << MESA_PRIM_x) values that indicates
   *                 kind of primitives are supported by the driver.
   * \param prim  the MESA_PRIM_x that the user wants to draw
   * \param start  index of first vertex to draw
   * \param nr  number of vertices to draw
   * \param in_pv  user's provoking vertex (PV_FIRST/LAST)
   * \param out_pv  desired proking vertex for the hardware (PV_FIRST/LAST)
   * \param out_prim  returns the new primitive type for the driver
   * \param out_index_size  returns OUT_USHORT or OUT_UINT
   * \param out_nr  returns new number of vertices to draw
   * \param out_generate  returns pointer to the generator function
   */
   enum indices_mode
   u_index_generator(unsigned hw_mask,
                     enum mesa_prim prim,
   unsigned start,
   unsigned nr,
   unsigned in_pv,
   unsigned out_pv,
   enum mesa_prim *out_prim,
   {
                        *out_index_size = ((start + nr) > 0xfffe) ? 4 : 2;
   out_idx = out_size_idx(*out_index_size);
   *out_prim = u_index_prim_type_convert(hw_mask, prim, in_pv == out_pv);
            if ((hw_mask & (1<<prim)) &&
               *out_generate = (*out_prim == MESA_PRIM_QUADS ? generate_quads : generate)
            }
   *out_generate = (*out_prim == MESA_PRIM_QUADS ? generate_quads : generate)
            }
