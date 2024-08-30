   /*
   * Copyright 2017 Advanced Micro Devices, Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "glspirv.h"
   #include "errors.h"
   #include "shaderobj.h"
   #include "mtypes.h"
      #include "compiler/nir/nir.h"
   #include "compiler/spirv/nir_spirv.h"
      #include "program/program.h"
      #include "util/u_atomic.h"
   #include "api_exec_decl.h"
      void
   _mesa_spirv_module_reference(struct gl_spirv_module **dest,
         {
               if (old && p_atomic_dec_zero(&old->RefCount))
                     if (src)
      }
      void
   _mesa_shader_spirv_data_reference(struct gl_shader_spirv_data **dest,
         {
               if (old && p_atomic_dec_zero(&old->RefCount)) {
      _mesa_spirv_module_reference(&(*dest)->SpirVModule, NULL);
                        if (src)
      }
      void
   _mesa_spirv_shader_binary(struct gl_context *ctx,
               {
      struct gl_spirv_module *module;
            /* From OpenGL 4.6 Core spec, "7.2 Shader Binaries" :
   *
   * "An INVALID_VALUE error is generated if the data pointed to by binary
   *  does not match the specified binaryformat."
   *
   * However, the ARB_gl_spirv spec, under issue #16 says:
   *
   * "ShaderBinary is expected to form an association between the SPIR-V
   *  module and likely would not parse the module as would be required to
   *  detect unsupported capabilities or other validation failures."
   *
   * Which specifies little to no validation requirements. Nevertheless, the
   * two small checks below seem reasonable.
   */
   if (!binary || (length % 4) != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glShaderBinary");
               module = malloc(sizeof(*module) + length);
   if (!module) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glShaderBinary");
               p_atomic_set(&module->RefCount, 0);
   module->Length = length;
            for (int i = 0; i < n; ++i) {
               spirv_data = rzalloc(NULL, struct gl_shader_spirv_data);
   _mesa_shader_spirv_data_reference(&sh->spirv_data, spirv_data);
                     free((void *)sh->Source);
   sh->Source = NULL;
   free((void *)sh->FallbackSource);
            ralloc_free(sh->ir);
   sh->ir = NULL;
   ralloc_free(sh->symbols);
         }
      /**
   * This is the equivalent to compiler/glsl/linker.cpp::link_shaders()
   * but for SPIR-V programs.
   *
   * This method just creates the gl_linked_shader structs with a reference to
   * the SPIR-V data collected during previous steps.
   *
   * The real linking happens later in the driver-specifc call LinkShader().
   * This is so backends can implement different linking strategies for
   * SPIR-V programs.
   */
   void
   _mesa_spirv_link_shaders(struct gl_context *ctx, struct gl_shader_program *prog)
   {
      prog->data->LinkStatus = LINKING_SUCCESS;
            for (unsigned i = 0; i < prog->NumShaders; i++) {
      struct gl_shader *shader = prog->Shaders[i];
            /* We only support one shader per stage. The gl_spirv spec doesn't seem
   * to prevent this, but the way the API is designed, requiring all shaders
   * to be specialized with an entry point, makes supporting this quite
   * undefined.
   *
   * TODO: Turn this into a proper error once the spec bug
   * <https://gitlab.khronos.org/opengl/API/issues/58> is resolved.
   */
   if (prog->_LinkedShaders[shader_type]) {
      ralloc_strcat(&prog->data->InfoLog,
               prog->data->LinkStatus = LINKING_FAILURE;
                        struct gl_linked_shader *linked = rzalloc(NULL, struct gl_linked_shader);
            /* Create program and attach it to the linked shader */
   struct gl_program *gl_prog =
         if (!gl_prog) {
      prog->data->LinkStatus = LINKING_FAILURE;
   _mesa_delete_linked_shader(ctx, linked);
               _mesa_reference_shader_program_data(&gl_prog->sh.data,
            /* Don't use _mesa_reference_program() just take ownership */
            /* Reference the SPIR-V data from shader to the linked shader */
   _mesa_shader_spirv_data_reference(&linked->spirv_data,
            prog->_LinkedShaders[shader_type] = linked;
               int last_vert_stage =
      util_last_bit(prog->data->linked_stages &
         if (last_vert_stage)
            /* Some shaders have to be linked with some other shaders present. */
   if (!prog->SeparateShader) {
      static const struct {
         } stage_pairs[] = {
      { MESA_SHADER_GEOMETRY, MESA_SHADER_VERTEX },
   { MESA_SHADER_TESS_EVAL, MESA_SHADER_VERTEX },
   { MESA_SHADER_TESS_CTRL, MESA_SHADER_VERTEX },
               for (unsigned i = 0; i < ARRAY_SIZE(stage_pairs); i++) {
      gl_shader_stage a = stage_pairs[i].a;
   gl_shader_stage b = stage_pairs[i].b;
   if ((prog->data->linked_stages & ((1 << a) | (1 << b))) == (1 << a)) {
      ralloc_asprintf_append(&prog->data->InfoLog,
                     prog->data->LinkStatus = LINKING_FAILURE;
                     /* Compute shaders have additional restrictions. */
   if ((prog->data->linked_stages & (1 << MESA_SHADER_COMPUTE)) &&
      (prog->data->linked_stages & ~(1 << MESA_SHADER_COMPUTE))) {
   ralloc_asprintf_append(&prog->data->InfoLog,
               prog->data->LinkStatus = LINKING_FAILURE;
         }
      nir_shader *
   _mesa_spirv_to_nir(struct gl_context *ctx,
                     {
      struct gl_linked_shader *linked_shader = prog->_LinkedShaders[stage];
            struct gl_shader_spirv_data *spirv_data = linked_shader->spirv_data;
            struct gl_spirv_module *spirv_module = spirv_data->SpirVModule;
            const char *entry_point_name = spirv_data->SpirVEntryPoint;
            struct nir_spirv_specialization *spec_entries =
      calloc(sizeof(*spec_entries),
         for (unsigned i = 0; i < spirv_data->NumSpecializationConstants; ++i) {
      spec_entries[i].id = spirv_data->SpecializationConstantsIndex[i];
   spec_entries[i].value.u32 = spirv_data->SpecializationConstantsValue[i];
               const struct spirv_to_nir_options spirv_options = {
      .environment = NIR_SPIRV_OPENGL,
   .subgroup_size = SUBGROUP_SIZE_UNIFORM,
   .caps = ctx->Const.SpirVCapabilities,
   .ubo_addr_format = nir_address_format_32bit_index_offset,
            /* TODO: Consider changing this to an address format that has the NULL
   * pointer equals to 0.  That might be a better format to play nice
   * with certain code / code generators.
   */
                  nir_shader *nir =
      spirv_to_nir((const uint32_t *) &spirv_module->Binary[0],
               spirv_module->Length / 4,
   spec_entries, spirv_data->NumSpecializationConstants,
               assert(nir);
                     nir->info.name =
      ralloc_asprintf(nir, "SPIRV:%s:%d",
                              /* Convert some sysvals to input varyings. */
   const struct nir_lower_sysvals_to_varyings_options sysvals_to_varyings = {
      .frag_coord = !ctx->Const.GLSLFragCoordIsSysVal,
   .point_coord = !ctx->Const.GLSLPointCoordIsSysVal,
      };
            /* We have to lower away local constant initializers right before we
   * inline functions.  That way they get properly initialized at the top
   * of the function and not at the top of its caller.
   */
   NIR_PASS_V(nir, nir_lower_variable_initializers, nir_var_function_temp);
   NIR_PASS_V(nir, nir_lower_returns);
   NIR_PASS_V(nir, nir_inline_functions);
   NIR_PASS_V(nir, nir_copy_prop);
            /* Pick off the single entrypoint that we want */
            /* Now that we've deleted all but the main function, we can go ahead and
   * lower the rest of the constant initializers.  We do this here so that
   * nir_remove_dead_variables and split_per_member_structs below see the
   * corresponding stores.
   */
            /* Split member structs.  We do this before lower_io_to_temporaries so that
   * it doesn't lower system values to temporaries by accident.
   */
   NIR_PASS_V(nir, nir_split_var_copies);
            if (nir->info.stage == MESA_SHADER_VERTEX)
                        }
      void GLAPIENTRY
   _mesa_SpecializeShaderARB(GLuint shader,
                           {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_shader *sh;
            if (!ctx->Extensions.ARB_gl_spirv) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glSpecializeShaderARB");
               sh = _mesa_lookup_shader_err(ctx, shader, "glSpecializeShaderARB");
   if (!sh)
            if (!sh->spirv_data) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (sh->CompileStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                              /* From the GL_ARB_gl_spirv spec:
   *
   *    "The OpenGL API expects the SPIR-V module to have already been
   *     validated, and can return an error if it discovers anything invalid
   *     in the module. An invalid SPIR-V module is allowed to result in
   *     undefined behavior."
   *
   * However, the following errors still need to be detected (from the same
   * spec):
   *
   *    "INVALID_VALUE is generated if <pEntryPoint> does not name a valid
   *     entry point for <shader>.
   *
   *     INVALID_VALUE is generated if any element of <pConstantIndex>
   *     refers to a specialization constant that does not exist in the
   *     shader module contained in <shader>."
   *
   * We cannot flag those errors a-priori because detecting them requires
   * parsing the module. However, flagging them during specialization is okay,
   * since it makes no difference in terms of application-visible state.
   */
            for (unsigned i = 0; i < numSpecializationConstants; ++i) {
      spec_entries[i].id = pConstantIndex[i];
   spec_entries[i].value.u32 = pConstantValue[i];
               enum spirv_verify_result r = spirv_verify_gl_specialization_constants(
      (uint32_t *)&spirv_data->SpirVModule->Binary[0],
   spirv_data->SpirVModule->Length / 4,
   spec_entries, numSpecializationConstants,
         switch (r) {
   case SPIRV_VERIFY_OK:
         case SPIRV_VERIFY_PARSER_ERROR:
      _mesa_error(ctx, GL_INVALID_VALUE,
                  case SPIRV_VERIFY_ENTRY_POINT_NOT_FOUND:
      _mesa_error(ctx, GL_INVALID_VALUE,
                  case SPIRV_VERIFY_UNKNOWN_SPEC_INDEX:
      for (unsigned i = 0; i < numSpecializationConstants; ++i) {
      if (spec_entries[i].defined_on_module == false) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     }
                        /* Note that we didn't make a real compilation of the module (spirv_to_nir),
   * but just checked some error conditions. Real "compilation" will be done
   * later, upon linking.
   */
            spirv_data->NumSpecializationConstants = numSpecializationConstants;
   spirv_data->SpecializationConstantsIndex =
      rzalloc_array_size(spirv_data, sizeof(GLuint),
      spirv_data->SpecializationConstantsValue =
      rzalloc_array_size(spirv_data, sizeof(GLuint),
      for (unsigned i = 0; i < numSpecializationConstants; ++i) {
      spirv_data->SpecializationConstantsIndex[i] = pConstantIndex[i];
            end:
         }
