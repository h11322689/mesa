   /*
   * Copyright © 2023 Valve Corporation
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
      #include "nir.h"
   #include "nir_builder.h"
   #include "radv_nir.h"
      bool
   radv_nir_lower_primitive_shading_rate(nir_shader *nir, enum amd_gfx_level gfx_level)
   {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
                     /* Iterate in reverse order since there should be only one deref store to PRIMITIVE_SHADING_RATE
   * after lower_io_to_temporaries for vertex shaders.
   */
   nir_foreach_block_reverse (block, impl) {
      nir_foreach_instr_reverse (instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                  nir_variable *var = nir_intrinsic_get_var(intr, 0);
                                    /* x_rate = (shadingRate & (Horizontal2Pixels | Horizontal4Pixels)) ? 0x1 : 0x0; */
                  /* y_rate = (shadingRate & (Vertical2Pixels | Vertical4Pixels)) ? 0x1 : 0x0; */
                           /* MS:
   * Primitive shading rate is a per-primitive output, it is
   * part of the second channel of the primitive export.
   * Bits [28:31] = VRS rate
   * This will be added to the other bits of that channel in the backend.
   *
   * VS, TES, GS:
   * Primitive shading rate is a per-vertex output pos export.
   * Bits [2:5] = VRS rate
   * HW shading rate = (xRate << 2) | (yRate << 4)
   *
   * GFX11: 4-bit VRS_SHADING_RATE enum
   * GFX10: X = low 2 bits, Y = high 2 bits
   */
                  if (gfx_level >= GFX11) {
      x_rate_shift = 4;
      }
   if (nir->info.stage == MESA_SHADER_MESH) {
      x_rate_shift += 26;
                                 progress = true;
   if (nir->info.stage == MESA_SHADER_VERTEX)
      }
   if (nir->info.stage == MESA_SHADER_VERTEX && progress)
               if (progress)
         else
               }