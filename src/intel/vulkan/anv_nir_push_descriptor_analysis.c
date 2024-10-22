   /*
   * Copyright © 2022 Intel Corporation
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
      #include "anv_nir.h"
      #include "compiler/brw_nir.h"
      const struct anv_descriptor_set_layout *
   anv_pipeline_layout_get_push_set(const struct anv_pipeline_sets_layout *layout,
         {
      for (unsigned s = 0; s < ARRAY_SIZE(layout->set); s++) {
               if (!set_layout ||
      !(set_layout->flags &
               if (set_idx)
                           }
      /* This function returns a bitfield of used descriptors in the push descriptor
   * set. You can only call this function before calling
   * anv_nir_apply_pipeline_layout() as information required is lost after
   * applying the pipeline layout.
   */
   uint32_t
   anv_nir_compute_used_push_descriptors(nir_shader *shader,
         {
      uint8_t push_set;
   const struct anv_descriptor_set_layout *push_set_layout =
         if (push_set_layout == NULL)
            uint32_t used_push_bindings = 0;
   nir_foreach_variable_with_modes(var, shader,
                              if (var->data.descriptor_set == push_set) {
      uint32_t desc_idx =
         assert(desc_idx < MAX_PUSH_DESCRIPTORS);
                  nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                  uint8_t set = nir_intrinsic_desc_set(intrin);
                  uint32_t binding = nir_intrinsic_binding(intrin);
   uint32_t desc_idx =
                                       }
      /* This function checks whether the shader accesses the push descriptor
   * buffer. This function must be called after anv_nir_compute_push_layout().
   */
   bool
   anv_nir_loads_push_desc_buffer(nir_shader *nir,
               {
      uint8_t push_set;
   const struct anv_descriptor_set_layout *push_set_layout =
         if (push_set_layout == NULL)
            nir_foreach_function_impl(impl, nir) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                  const nir_const_value *const_bt_idx =
                                 const struct anv_pipeline_binding *binding =
         if (binding->set == ANV_DESCRIPTOR_SET_DESCRIPTORS &&
      binding->index == push_set)
                     }
      /* This function computes a bitfield of all the UBOs bindings in the push
   * descriptor set that are fully promoted to push constants. If a binding's
   * bit in the field is set, the corresponding binding table entry will not be
   * accessed by the shader. This function must be called after
   * anv_nir_compute_push_layout().
   */
   uint32_t
   anv_nir_push_desc_ubo_fully_promoted(nir_shader *nir,
               {
      uint8_t push_set;
   const struct anv_descriptor_set_layout *push_set_layout =
         if (push_set_layout == NULL)
            uint32_t ubos_fully_promoted = 0;
   for (uint32_t b = 0; b < push_set_layout->binding_count; b++) {
      const struct anv_descriptor_set_binding_layout *bind_layout =
         if (bind_layout->type == -1)
            assert(bind_layout->descriptor_index < MAX_PUSH_DESCRIPTORS);
   if (bind_layout->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
               nir_foreach_function_impl(impl, nir) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                                                /* Skip if this isn't a load from push descriptor buffer. */
   const struct anv_pipeline_binding *binding =
                        const uint32_t desc_idx =
                           /* If the offset in the entry is dynamic, we can't tell if
   * promoted or not.
   */
   const nir_const_value *const_load_offset =
         if (const_load_offset != NULL) {
      /* Check if the load was promoted to a push constant. */
                        for (unsigned i = 0; i < ARRAY_SIZE(bind_map->push_ranges); i++) {
      if (bind_map->push_ranges[i].set == binding->set &&
      bind_map->push_ranges[i].index == desc_idx &&
   bind_map->push_ranges[i].start * 32 <= load_offset &&
   (bind_map->push_ranges[i].start +
   bind_map->push_ranges[i].length) * 32 >=
   (load_offset + load_bytes)) {
   promoted = true;
                     if (!promoted)
                        }
