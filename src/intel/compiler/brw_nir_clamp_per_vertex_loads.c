   /*
   * Copyright (c) 2022 Intel Corporation
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
      /*
   * Limit input per vertex input accesses. This is useful for the tesselation stages.
   * On Gfx12.5+ out of bound accesses generate hangs.
   *
   * This pass operates on derefs, it must be called before shader inputs are
   * lowered.
   */
      #include "brw_nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_deref.h"
      static bool
   clamp_per_vertex_loads_instr(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (intrin->intrinsic != nir_intrinsic_load_deref)
            nir_deref_instr *deref = nir_instr_as_deref(intrin->src[0].ssa->parent_instr);
   nir_variable *var = nir_deref_instr_get_variable(deref);
   if (var == NULL || (var->data.mode & nir_var_shader_in) == 0)
            nir_deref_path path;
            bool progress = false;
   for (unsigned i = 0; path.path[i]; i++) {
      if (path.path[i]->deref_type != nir_deref_type_array)
                     nir_src_rewrite(&path.path[i]->arr.index,
            progress = true;
                           }
      bool
   brw_nir_clamp_per_vertex_loads(nir_shader *shader)
   {
               bool ret = nir_shader_intrinsics_pass(shader, clamp_per_vertex_loads_instr,
                                    }
      static bool
   lower_patch_vertices_instr(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (intrin->intrinsic != nir_intrinsic_load_patch_vertices_in)
                                          }
      bool
   brw_nir_lower_patch_vertices_in(nir_shader *shader, unsigned input_vertices)
   {
      return nir_shader_intrinsics_pass(shader, lower_patch_vertices_instr,
                  }
