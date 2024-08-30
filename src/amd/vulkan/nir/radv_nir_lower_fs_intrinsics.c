   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
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
   #include "radv_private.h"
      bool
   radv_nir_lower_fs_intrinsics(nir_shader *nir, const struct radv_shader_stage *fs_stage,
         {
      const struct radv_shader_info *info = &fs_stage->info;
   const struct radv_shader_args *args = &fs_stage->args;
   nir_function_impl *impl = nir_shader_get_entrypoint(nir);
                     nir_foreach_block (block, impl) {
      nir_foreach_instr_safe (instr, block) {
                                    switch (intrin->intrinsic) {
                     nir_def *def = NULL;
   if (info->ps.uses_sample_shading || key->ps.sample_shading_enable) {
      /* gl_SampleMaskIn[0] = (SampleCoverage & (PsIterMask << gl_SampleID)). */
   nir_def *ps_state = nir_load_scalar_arg_amd(&b, 1, .base = args->ps_state.arg_index);
   nir_def *ps_iter_mask =
         nir_def *sample_id = nir_load_sample_id(&b);
      } else {
                           nir_instr_remove(instr);
   progress = true;
      }
   case nir_intrinsic_load_frag_coord: {
                                             /* adjusted_frag_z = fddx_fine(frag_z) * 0.0625 + frag_z */
                  /* VRS Rate X = Ancillary[2:3] */
                  /* xRate = xRate == 0x1 ? adjusted_frag_z : frag_z. */
                                 progress = true;
      }
   case nir_intrinsic_load_barycentric_at_sample: {
                                       nir_push_if(&b, nir_ieq_imm(&b, num_samples, 1));
   {
         }
                                          res2 = nir_load_barycentric_at_offset(&b, 32, sample_pos,
                        } else {
      if (!key->ps.num_samples) {
                                          new_dest = nir_load_barycentric_at_offset(&b, 32, sample_pos,
                                 progress = true;
      }
   default:
                        if (progress)
         else
               }
