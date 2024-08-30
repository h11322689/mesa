   /*
   * Copyright © 2020 Intel Corporation
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
   #include "nir_worklist.h"
   #include "util/u_vector.h"
      static bool
   combine_all_barriers(nir_intrinsic_instr *a, nir_intrinsic_instr *b, void *_)
   {
      nir_intrinsic_set_memory_modes(
         nir_intrinsic_set_memory_semantics(
         nir_intrinsic_set_memory_scope(
         nir_intrinsic_set_execution_scope(
            }
      static bool
   nir_opt_combine_barriers_impl(nir_function_impl *impl,
               {
               nir_foreach_block(block, impl) {
               nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic) {
      prev = NULL;
               nir_intrinsic_instr *current = nir_instr_as_intrinsic(instr);
   if (current->intrinsic != nir_intrinsic_barrier) {
      prev = NULL;
               if (prev && combine_cb(prev, current, data)) {
      nir_instr_remove(&current->instr);
      } else {
                        if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
            } else {
                     }
      /* Combine adjacent scoped barriers. */
   bool
   nir_opt_combine_barriers(nir_shader *shader,
               {
      /* Default to combining everything. Only some backends can do better. */
   if (!combine_cb)
                     nir_foreach_function_impl(impl, shader) {
      if (nir_opt_combine_barriers_impl(impl, combine_cb, data)) {
                        }
      static bool
   barrier_happens_before(const nir_instr *a, const nir_instr *b)
   {
      if (a->block == b->block)
               }
      static bool
   nir_opt_barrier_modes_impl(nir_function_impl *impl)
   {
               nir_instr_worklist *barriers = nir_instr_worklist_create();
   if (!barriers)
            struct u_vector mem_derefs;
   if (!u_vector_init(&mem_derefs, 32, sizeof(struct nir_instr *))) {
      nir_instr_worklist_destroy(barriers);
               const unsigned all_memory_modes = nir_var_image |
                        nir_foreach_block_safe(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                                                      if (nir_deref_mode_may_be(deref, all_memory_modes) ||
      glsl_contains_atomic(deref->type)) {
   nir_deref_instr **tail = u_vector_add(&mem_derefs);
                        nir_foreach_instr_in_worklist(instr, barriers) {
               const unsigned barrier_modes = nir_intrinsic_memory_modes(barrier);
            /* If a barrier dominates all memory accesses for a particular mode (or
   * there are none), then the barrier cannot affect those accesses.  We
   * can drop that mode from the barrier.
   *
   * For each barrier, we look at the list of memory derefs, and see if
   * the barrier fails to dominate the deref.  If so, then there's at
   * least one memory access that may happen before the barrier, so we
   * need to keep the mode.  Any modes not kept are discarded.
   */
   nir_deref_instr **p_deref;
   u_vector_foreach(p_deref, &mem_derefs) {
      nir_deref_instr *deref = *p_deref;
   const unsigned atomic_mode =
                        if (deref_modes &&
      !barrier_happens_before(&barrier->instr, &deref->instr))
            /* If we don't need all the modes, update the barrier. */
   if (barrier_modes != new_modes) {
      nir_intrinsic_set_memory_modes(barrier, new_modes);
               /* Shared memory only exists within a workgroup, so synchronizing it
   * beyond workgroup scope is nonsense.
   */
   if (nir_intrinsic_execution_scope(barrier) == SCOPE_NONE &&
      new_modes == nir_var_mem_shared) {
   nir_intrinsic_set_memory_scope(barrier,
                        nir_instr_worklist_destroy(barriers);
               }
      /**
   * Reduce barriers to remove unnecessary modes and scope.
   *
   * This pass must be called before nir_lower_explicit_io lowers derefs!
   *
   * Many shaders issue full memory barriers, which may need to synchronize
   * access to images, SSBOs, shared local memory, or global memory.  However,
   * many of them only use a subset of those memory types - say, only SSBOs.
   *
   * Shaders may also have patterns such as:
   *
   *    1. shared local memory access
   *    2. barrier with full variable modes
   *    3. more shared local memory access
   *    4. image access
   *
   * In this case, the barrier is needed to ensure synchronization between the
   * various shared memory operations.  Image reads and writes do also exist,
   * but they are all on one side of the barrier, so it is a no-op for image
   * access.  We can drop the image mode from the barrier in this case too.
   *
   * In addition, we can reduce the memory scope of shared-only barriers, as
   * shared local memory only exists within a workgroup.
   */
   bool
   nir_opt_barrier_modes(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
      nir_metadata_require(impl, nir_metadata_dominance |
            if (nir_opt_barrier_modes_impl(impl)) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
                  } else {
                        }
