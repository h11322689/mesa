   /*
   * Copyright © 2009 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * \file prog_parameter_layout.c
   * \brief Helper functions to layout storage for program parameters
   *
   * \author Ian Romanick <ian.d.romanick@intel.com>
   */
         #include "main/shader_types.h"
   #include "prog_parameter.h"
   #include "prog_parameter_layout.h"
   #include "prog_instruction.h"
   #include "program_parser.h"
      unsigned
   _mesa_combine_swizzles(unsigned base, unsigned applied)
   {
      unsigned swiz = 0;
            for (i = 0; i < 4; i++) {
                              }
         /**
   * Copy indirect access array from one parameter list to another
   *
   * \param src   Parameter array copied from
   * \param dst   Parameter array copied to
   * \param first Index of first element in \c src to copy
   * \param count Number of elements to copy
   *
   * \return
   * The location in \c dst of the first element copied from \c src on
   * success.  -1 on failure.
   *
   * \warning
   * This function assumes that there is already enough space available in
   * \c dst to hold all of the elements that will be copied over.
   */
   static int
   copy_indirect_accessed_array(struct gl_program_parameter_list *src,
         struct gl_program_parameter_list *dst,
   {
      const int base = dst->NumParameters;
            for (i = first; i < (first + count); i++) {
               j = dst->NumParameters;
         for (j = 0; j < dst->NumParameters; j++) {
      if (memcmp(dst->Parameters[j].StateIndexes, curr->StateIndexes,
                  }
                           /* copy src parameter [i] to dest parameter [j] */
   memcpy(&dst->Parameters[j], curr,
                     gl_constant_value *pv_dst =
         gl_constant_value *pv_src =
            memcpy(pv_dst, pv_src, MIN2(src->Parameters[i].Size, 4) *
                     /* Pointer to the string name was copied.  Null-out src param name
   * to prevent double free later.
   */
                           }
         static int compare_state_var(const void *a1, const void *a2)
   {
      const struct gl_program_parameter *p1 =
         const struct gl_program_parameter *p2 =
            for (unsigned i = 0; i < STATE_LENGTH; i++) {
      if (p1->StateIndexes[i] != p2->StateIndexes[i])
      }
      }
         /**
   * Create the final program parameter list in this order:
   * - constants and state variables with variable indexing are first
   * - other constants are next
   * - other state variables are last and sorted
   *
   * \return GL_TRUE for success, GL_FALSE for failure
   */
   GLboolean
   _mesa_layout_parameters(struct asm_parser_state *state)
   {
      struct gl_program_parameter_list *layout;
            layout =
            /* PASS 1:  Move any parameters that are accessed indirectly from the
   * original parameter list to the new parameter list.
   */
   for (inst = state->inst_head; inst != NULL; inst = inst->next) {
      for (unsigned i = 0; i < 3; i++) {
      if (inst->SrcReg[i].Base.RelAddr) {
      /* Only attempt to add the to the new parameter list once.
   */
   if (!inst->SrcReg[i].Symbol->pass1_done) {
      const int new_begin =
                        if (new_begin < 0) {
                                          /* Previously the Index was just the offset from the parameter
   * array.  Now that the base of the parameter array is known, the
   * index can be updated to its actual value.
   */
   inst->Base.SrcReg[i] = inst->SrcReg[i].Base;
   inst->Base.SrcReg[i].Index +=
                     /* PASS 2: Move any constants that are not accessed indirectly from the
   * original parameter list to the new parameter list.
   */
   for (inst = state->inst_head; inst != NULL; inst = inst->next) {
      for (unsigned i = 0; i < 3; i++) {
      const int idx = inst->SrcReg[i].Base.Index;
   const struct gl_program_parameter *const p =
                  if (inst->SrcReg[i].Base.RelAddr ||
      inst->SrcReg[i].Base.File <= PROGRAM_OUTPUT ||
   inst->SrcReg[i].Base.File >= PROGRAM_WRITE_ONLY ||
                        unsigned pvo = state->prog->Parameters->Parameters[idx].ValueOffset;
                                                inst->SrcReg[i].Base.File = p->Type;
                  /* PASS 3: Add sorted state variables.  NOTE: This pass does **not** modify
   * the instruction with the updated index.  The sorting step might
   * invalidate the index that was calculated by _mesa_add_state_reference.
   * Instead, it relies on PASS 4 to do this.
   */
            for (inst = state->inst_head; inst != NULL; inst = inst->next) {
      for (unsigned i = 0; i < 3; i++) {
                              if (inst->SrcReg[i].Base.RelAddr ||
      inst->SrcReg[i].Base.File <= PROGRAM_OUTPUT ||
   inst->SrcReg[i].Base.File >= PROGRAM_WRITE_ONLY ||
                              /* Sort if we have added at least 2 state vars. */
   if (first_state_var + 2 <= layout->NumParameters) {
      /* All state vars should be vec4s. */
   for (unsigned i = first_state_var; i < layout->NumParameters; i++) {
      assert(layout->Parameters[i].Size == 4);
               qsort(layout->Parameters + first_state_var,
                  /* Fix offsets. */
   for (unsigned i = first_state_var; i < layout->NumParameters; i++) {
                     /* PASS 4: Fix up the index and file information for instructions whose
   * parameters were added to the parameter list in PASS 3.
   */
   for (inst = state->inst_head; inst != NULL; inst = inst->next) {
      for (unsigned i = 0; i < 3; i++) {
      const int idx = inst->SrcReg[i].Base.Index;
                  if (inst->SrcReg[i].Base.RelAddr ||
      inst->SrcReg[i].Base.File <= PROGRAM_OUTPUT ||
   inst->SrcReg[i].Base.File >= PROGRAM_WRITE_ONLY ||
                                       inst->SrcReg[i].Base.File = p->Type;
                  assert(layout->NumParameters <= state->prog->Parameters->NumParameters);
            layout->StateFlags = state->prog->Parameters->StateFlags;
   _mesa_free_parameter_list(state->prog->Parameters);
               }
