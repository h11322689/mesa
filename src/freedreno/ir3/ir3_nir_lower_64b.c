   /*
   * Copyright © 2021 Google, Inc.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "ir3_nir.h"
      /*
   * Lowering for 64b intrinsics generated with OpenCL or with
   * VK_KHR_buffer_device_address. All our intrinsics from a hw
   * standpoint are 32b, so we just need to combine in zero for
   * the upper 32bits and let the other nir passes clean up the mess.
   */
      static bool
   lower_64b_intrinsics_filter(const nir_instr *instr, const void *unused)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     if (intr->intrinsic == nir_intrinsic_load_deref ||
      intr->intrinsic == nir_intrinsic_store_deref)
         if (is_intrinsic_store(intr->intrinsic))
            if (nir_intrinsic_dest_components(intr) == 0)
               }
      static nir_def *
   lower_64b_intrinsics(nir_builder *b, nir_instr *instr, void *unused)
   {
                        /* We could be *slightly* more clever and, for ex, turn a 64b vec4
   * load into two 32b vec4 loads, rather than 4 32b vec2 loads.
            if (is_intrinsic_store(intr->intrinsic)) {
      unsigned offset_src_idx;
   switch (intr->intrinsic) {
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_global_ir3:
      offset_src_idx = 2;
      default:
                  unsigned num_comp = nir_intrinsic_src_components(intr, 0);
   unsigned wrmask = nir_intrinsic_has_write_mask(intr) ?
         nir_def *val = intr->src[0].ssa;
            for (unsigned i = 0; i < num_comp; i++) {
                                    nir_intrinsic_instr *store =
         store->num_components = 2;
                  if (nir_intrinsic_has_write_mask(intr))
                                                   nir_def *def = &intr->def;
            /* load_kernel_input is handled specially, lowering to two 32b inputs:
   */
   if (intr->intrinsic == nir_intrinsic_load_kernel_input) {
               nir_def *offset = nir_iadd_imm(b,
                                          if (is_intrinsic_load(intr->intrinsic)) {
      unsigned offset_src_idx;
   switch(intr->intrinsic) {
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_global_ir3:
      offset_src_idx = 1;
      default:
                           for (unsigned i = 0; i < num_comp; i++) {
      nir_intrinsic_instr *load =
                                                      } else {
      /* The remaining (non load/store) intrinsics just get zero-
   * extended from 32b to 64b:
   */
   for (unsigned i = 0; i < num_comp; i++) {
      nir_def *c = nir_channel(b, def, i);
                     }
      bool
   ir3_nir_lower_64b_intrinsics(nir_shader *shader)
   {
      return nir_shader_lower_instructions(
            }
      /*
   * Lowering for 64b undef instructions, splitting into a two 32b undefs
   */
      static nir_def *
   lower_64b_undef(nir_builder *b, nir_instr *instr, void *unused)
   {
               nir_undef_instr *undef = nir_instr_as_undef(instr);
   unsigned num_comp = undef->def.num_components;
            for (unsigned i = 0; i < num_comp; i++) {
               components[i] = nir_pack_64_2x32_split(b,
                        }
      static bool
   lower_64b_undef_filter(const nir_instr *instr, const void *unused)
   {
               return instr->type == nir_instr_type_undef &&
      }
      bool
   ir3_nir_lower_64b_undef(nir_shader *shader)
   {
      return nir_shader_lower_instructions(
            }
      /*
   * Lowering for load_global/store_global with 64b addresses to ir3
   * variants, which instead take a uvec2_32
   */
      static bool
   lower_64b_global_filter(const nir_instr *instr, const void *unused)
   {
               if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_store_global:
   case nir_intrinsic_global_atomic:
   case nir_intrinsic_global_atomic_swap:
         default:
            }
      static nir_def *
   lower_64b_global(nir_builder *b, nir_instr *instr, void *unused)
   {
               nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
            nir_def *addr64 = intr->src[load ? 0 : 1].ssa;
            /*
   * Note that we can get vec8/vec16 with OpenCL.. we need to split
   * those up into max 4 components per load/store.
            if (intr->intrinsic == nir_intrinsic_global_atomic) {
      return nir_global_atomic_ir3(
         b, intr->def.bit_size, addr,
      } else if (intr->intrinsic == nir_intrinsic_global_atomic_swap) {
      return nir_global_atomic_swap_ir3(
      b, intr->def.bit_size, addr,
   intr->src[1].ssa, intr->src[2].ssa,
            if (load) {
      unsigned num_comp = nir_intrinsic_dest_components(intr);
   nir_def *components[num_comp];
   for (unsigned off = 0; off < num_comp;) {
      unsigned c = MIN2(num_comp - off, 4);
   nir_def *val = nir_load_global_ir3(
         b, c, intr->def.bit_size,
   for (unsigned i = 0; i < c; i++) {
            }
      } else {
      unsigned num_comp = nir_intrinsic_src_components(intr, 0);
   nir_def *value = intr->src[0].ssa;
   for (unsigned off = 0; off < num_comp; off += 4) {
      unsigned c = MIN2(num_comp - off, 4);
   nir_def *v = nir_channels(b, value, BITFIELD_MASK(c) << off);
      }
         }
      bool
   ir3_nir_lower_64b_global(nir_shader *shader)
   {
      return nir_shader_lower_instructions(
            }
