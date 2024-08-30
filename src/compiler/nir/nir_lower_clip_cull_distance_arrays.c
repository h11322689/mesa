   /*
   * Copyright © 2015 Intel Corporation
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
      /**
   * @file
   *
   * This pass combines clip and cull distance arrays in separate locations and
   * colocates them both in VARYING_SLOT_CLIP_DIST0.  It does so by maintaining
   * two arrays but making them compact and using location_frac to stack them on
   * top of each other.
   */
      /**
   * Get the length of the clip/cull distance array, looking past
   * any interface block arrays.
   */
   static unsigned
   get_unwrapped_array_length(nir_shader *nir, nir_variable *var)
   {
      if (!var)
            /* Unwrap GS input and TCS input/output interfaces.  We want the
   * underlying clip/cull distance array length, not the per-vertex
   * array length.
   */
   const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, nir->info.stage))
            if (var->data.per_view) {
      assert(glsl_type_is_array(type));
                           }
      static bool
   combine_clip_cull(nir_shader *nir,
               {
      nir_variable *cull = NULL;
            nir_foreach_variable_with_modes(var, nir, mode) {
      if (var->data.location == VARYING_SLOT_CLIP_DIST0)
            if (var->data.location == VARYING_SLOT_CULL_DIST0)
               if (!cull && !clip) {
      /* If this is run after optimizations and the variables have been
   * eliminated, we should update the shader info, because no other
   * place does that.
   */
   if (store_info) {
      nir->info.clip_distance_array_size = 0;
      }
               if (!cull && clip) {
      /* The GLSL IR lowering pass must have converted these to vectors */
   if (!clip->data.compact)
            /* If this pass has already run, don't repeat.  We would think that
   * the combined clip/cull distance array was clip-only and mess up.
   */
   if (clip->data.how_declared == nir_var_hidden)
               const unsigned clip_array_size = get_unwrapped_array_length(nir, clip);
            if (store_info) {
      nir->info.clip_distance_array_size = clip_array_size;
               if (clip) {
      assert(clip->data.compact);
               if (cull) {
      assert(cull->data.compact);
   cull->data.how_declared = nir_var_hidden;
   cull->data.location = VARYING_SLOT_CLIP_DIST0 + clip_array_size / 4;
                  }
      bool
   nir_lower_clip_cull_distance_arrays(nir_shader *nir)
   {
               if (nir->info.stage <= MESA_SHADER_GEOMETRY ||
      nir->info.stage == MESA_SHADER_MESH)
         if (nir->info.stage > MESA_SHADER_VERTEX &&
      nir->info.stage <= MESA_SHADER_FRAGMENT) {
   progress |= combine_clip_cull(nir, nir_var_shader_in,
               nir_foreach_function_impl(impl, nir) {
      if (progress) {
      nir_metadata_preserve(impl,
                        } else {
                        }
