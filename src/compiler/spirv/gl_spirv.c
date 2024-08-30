   /*
   * Copyright © 2017 Intel Corporation
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
   *
   *
   */
      #include "nir_spirv.h"
      #include "vtn_private.h"
   #include "spirv_info.h"
      static bool
   vtn_validate_preamble_instruction(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpString:
   case SpvOpSource:
   case SpvOpSourceExtension:
   case SpvOpSourceContinued:
   case SpvOpModuleProcessed:
      /* We need this since vtn_foreach_instruction automatically handles
   * OpLine / OpNoLine and relies on the SpvOpString from preamble being
   * handled.
   */
   vtn_handle_debug_text(b, opcode, w, count);
         case SpvOpExtension:
   case SpvOpCapability:
   case SpvOpExtInstImport:
   case SpvOpMemoryModel:
   case SpvOpName:
   case SpvOpMemberName:
   case SpvOpExecutionMode:
   case SpvOpDecorationGroup:
   case SpvOpMemberDecorate:
   case SpvOpGroupDecorate:
   case SpvOpGroupMemberDecorate:
            case SpvOpEntryPoint:
      vtn_handle_entry_point(b, w, count);
         case SpvOpDecorate:
      vtn_handle_decoration(b, opcode, w, count);
         default:
                     }
      static void
   spec_constant_decoration_cb(struct vtn_builder *b, struct vtn_value *v,
               {
      vtn_assert(member == -1);
   if (dec->decoration != SpvDecorationSpecId)
            for (unsigned i = 0; i < b->num_specializations; i++) {
      if (b->specializations[i].id == dec->operands[0]) {
      b->specializations[i].defined_on_module = true;
            }
      static void
   vtn_validate_handle_constant(struct vtn_builder *b, SpvOp opcode,
         {
               switch (opcode) {
   case SpvOpConstant:
   case SpvOpConstantNull:
   case SpvOpSpecConstantComposite:
   case SpvOpConstantComposite:
      /* Nothing to do here for gl_spirv needs */
         case SpvOpConstantTrue:
   case SpvOpConstantFalse:
   case SpvOpSpecConstantTrue:
   case SpvOpSpecConstantFalse:
   case SpvOpSpecConstant:
   case SpvOpSpecConstantOp:
      vtn_foreach_decoration(b, val, spec_constant_decoration_cb, NULL);
         case SpvOpConstantSampler:
      vtn_fail("OpConstantSampler requires Kernel Capability");
         default:
            }
      static bool
   vtn_validate_handle_constant_instruction(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpSource:
   case SpvOpSourceContinued:
   case SpvOpSourceExtension:
   case SpvOpExtension:
   case SpvOpCapability:
   case SpvOpExtInstImport:
   case SpvOpMemoryModel:
   case SpvOpEntryPoint:
   case SpvOpExecutionMode:
   case SpvOpString:
   case SpvOpName:
   case SpvOpMemberName:
   case SpvOpDecorationGroup:
   case SpvOpDecorate:
   case SpvOpMemberDecorate:
   case SpvOpGroupDecorate:
   case SpvOpGroupMemberDecorate:
      vtn_fail("Invalid opcode types and variables section");
         case SpvOpTypeVoid:
   case SpvOpTypeBool:
   case SpvOpTypeInt:
   case SpvOpTypeFloat:
   case SpvOpTypeVector:
   case SpvOpTypeMatrix:
   case SpvOpTypeImage:
   case SpvOpTypeSampler:
   case SpvOpTypeSampledImage:
   case SpvOpTypeArray:
   case SpvOpTypeRuntimeArray:
   case SpvOpTypeStruct:
   case SpvOpTypeOpaque:
   case SpvOpTypePointer:
   case SpvOpTypeFunction:
   case SpvOpTypeEvent:
   case SpvOpTypeDeviceEvent:
   case SpvOpTypeReserveId:
   case SpvOpTypeQueue:
   case SpvOpTypePipe:
      /* We don't need to handle types */
         case SpvOpConstantTrue:
   case SpvOpConstantFalse:
   case SpvOpConstant:
   case SpvOpConstantComposite:
   case SpvOpConstantSampler:
   case SpvOpConstantNull:
   case SpvOpSpecConstantTrue:
   case SpvOpSpecConstantFalse:
   case SpvOpSpecConstant:
   case SpvOpSpecConstantComposite:
   case SpvOpSpecConstantOp:
      vtn_validate_handle_constant(b, opcode, w, count);
         case SpvOpUndef:
   case SpvOpVariable:
      /* We don't need to handle them */
         default:
                     }
      /*
   * Since OpenGL 4.6 you can use SPIR-V modules directly on OpenGL. One of the
   * new methods, glSpecializeShader include some possible errors when trying to
   * use it.
   *
   * From OpenGL 4.6, Section 7.2.1, "Shader Specialization":
   *
   * "void SpecializeShaderARB(uint shader,
   *                           const char* pEntryPoint,
   *                           uint numSpecializationConstants,
   *                           const uint* pConstantIndex,
   *                           const uint* pConstantValue);
   * <skip>
   *
   * INVALID_VALUE is generated if <pEntryPoint> does not name a valid
   * entry point for <shader>.
   *
   * An INVALID_VALUE error is generated if any element of pConstantIndex refers
   * to a specialization constant that does not exist in the shader module
   * contained in shader."
   *
   * We could do those checks on spirv_to_nir, but we are only interested on the
   * full translation later, during linking. This method is a simplified version
   * of spirv_to_nir, looking for only the checks needed by SpecializeShader.
   *
   * This method returns NULL if no entry point was found, and fill the
   * nir_spirv_specialization field "defined_on_module" accordingly. Caller
   * would need to trigger the specific errors.
   *
   */
   enum spirv_verify_result
   spirv_verify_gl_specialization_constants(
      const uint32_t *words, size_t word_count,
   struct nir_spirv_specialization *spec, unsigned num_spec,
      {
      /* vtn_warn/vtn_log uses debug.func. Setting a null to prevent crash. Not
   * need to print the warnings now, would be done later, on the real
   * spirv_to_nir
   */
   const struct spirv_to_nir_options options = { .debug.func = NULL};
            struct vtn_builder *b = vtn_create_builder(words, word_count,
                  if (b == NULL)
            /* See also _vtn_fail() */
   if (vtn_setjmp(b->fail_jump)) {
      ralloc_free(b);
               /* Skip the SPIR-V header, handled at vtn_create_builder */
            /* Search entry point from preamble */
   words = vtn_foreach_instruction(b, words, word_end,
            if (b->entry_point == NULL) {
      ralloc_free(b);
               b->specializations = spec;
            /* Handle constant instructions (we don't need to handle
   * variables or types for gl_spirv)
   */
   words = vtn_foreach_instruction(b, words, word_end,
                     for (unsigned i = 0; i < num_spec; i++) {
      if (!spec[i].defined_on_module)
                  }
   