   /*
   * Copyright © 2020 Google LLC
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
      /* Lowers nir_intrinsic_load_ubo() to nir_intrinsic_load_ubo_vec4() taking an
   * offset in vec4 units.  This is a fairly common mode of UBO addressing for
   * hardware to have, and it gives NIR a chance to optimize the addressing math
   * and CSE the loads.
   *
   * This pass handles lowering for loads that straddle a vec4 alignment
   * boundary.  We try to minimize the extra loads we generate for that case,
   * and are ensured non-straddling loads with:
   *
   * - std140 (GLSL 1.40, GLSL ES)
   * - Vulkan "Extended Layout" (the baseline for UBOs)
   *
   * but not:
   *
   * - GLSL 4.30's new packed mode (enabled by PIPE_CAP_LOAD_CONSTBUF) where
   *   vec3 arrays are packed tightly.
   *
   * - PackedDriverUniformStorage in GL (enabled by PIPE_CAP_PACKED_UNIFORMS)
   *   combined with nir_lower_uniforms_to_ubo, where values in the default
   *   uniform block are packed tightly.
   *
   * - Vulkan's scalarBlockLayout optional feature:
   *
   *   "A member is defined to improperly straddle if either of the following are
   *    true:
   *
   *    • It is a vector with total size less than or equal to 16 bytes, and has
   *      Offset decorations placing its first byte at F and its last byte at L
   *      where floor(F / 16) != floor(L / 16).
   *    • It is a vector with total size greater than 16 bytes and has its Offset
   *      decorations placing its first byte at a non-integer multiple of 16.
   *
   *    [...]
   *
   *    Unless the scalarBlockLayout feature is enabled on the device:
   *
   *    • Vectors must not improperly straddle, as defined above."
   */
      #include "nir.h"
   #include "nir_builder.h"
      static bool
   nir_lower_ubo_vec4_filter(const nir_instr *instr, const void *data)
   {
      if (instr->type != nir_instr_type_intrinsic)
               }
      static nir_intrinsic_instr *
   create_load(nir_builder *b, nir_def *block, nir_def *offset,
         {
      nir_def *def = nir_load_ubo_vec4(b, num_components, bit_size, block, offset);
      }
      static nir_def *
   nir_lower_ubo_vec4_lower(nir_builder *b, nir_instr *instr, void *data)
   {
                        nir_def *byte_offset = intr->src[1].ssa;
            unsigned align_mul = nir_intrinsic_align_mul(intr);
            int chan_size_bytes = intr->def.bit_size / 8;
            /* We don't care if someone figured out that things are aligned beyond
   * vec4.
   */
   align_mul = MIN2(align_mul, 16);
   align_offset &= 15;
            unsigned num_components = intr->num_components;
   bool aligned_mul = (align_mul == 16 &&
         if (!aligned_mul)
            nir_intrinsic_instr *load = create_load(b, intr->src[0].ssa, vec4_offset,
                                    int align_chan_offset = align_offset / chan_size_bytes;
   if (aligned_mul) {
      /* For an aligned load, just ask the backend to load from the known
   * offset's component.
   */
      } else if (intr->num_components == 1) {
      /* If we're loading a single component, that component alone won't
   * straddle a vec4 boundary so we can do this with a single UBO load.
   */
   nir_def *component =
      nir_iand_imm(b,
                  } else if (align_mul == 8 &&
            /* Special case: Loading small vectors from offset % 8 == 0 can be done
   * with just one load and one bcsel.
   */
   nir_component_mask_t low_channels =
         nir_component_mask_t high_channels =
         result = nir_bcsel(b, nir_test_mask(b, byte_offset, 8),
            } else {
      /* General fallback case: Per-result-channel bcsel-based extraction
   * from two separate vec4 loads.
   */
   nir_def *next_vec4_offset = nir_iadd_imm(b, vec4_offset, 1);
   nir_intrinsic_instr *next_load = create_load(b, intr->src[0].ssa, next_vec4_offset,
                  nir_def *channels[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < intr->num_components; i++) {
                        nir_def *component =
      nir_iand_imm(b,
               channels[i] = nir_vector_extract(b,
                                    nir_bcsel(b,
                           }
      bool
   nir_lower_ubo_vec4(nir_shader *shader)
   {
      return nir_shader_lower_instructions(shader,
                  }
