   /*
   * Copyright © 2008, 2009 Intel Corporation
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
   #include <getopt.h>
      /** @file standalone.cpp
   *
   * Standalone compiler helper lib.  Used by standalone glsl_compiler and
   * also available to drivers to implement their own standalone compiler
   * with driver backend.
   */
      #include "ast.h"
   #include "glsl_parser_extras.h"
   #include "ir_optimization.h"
   #include "program.h"
   #include "standalone_scaffolding.h"
   #include "standalone.h"
   #include "util/set.h"
   #include "linker.h"
   #include "glsl_parser_extras.h"
   #include "ir_builder_print_visitor.h"
   #include "builtin_functions.h"
   #include "opt_add_neg_to_sub.h"
   #include "main/mtypes.h"
   #include "program/program.h"
      class dead_variable_visitor : public ir_hierarchical_visitor {
   public:
      dead_variable_visitor()
   {
                  virtual ~dead_variable_visitor()
   {
                  virtual ir_visitor_status visit(ir_variable *ir)
   {
      /* If the variable is auto or temp, add it to the set of variables that
   * are candidates for removal.
   */
   if (ir->data.mode != ir_var_auto && ir->data.mode != ir_var_temporary)
                                 virtual ir_visitor_status visit(ir_dereference_variable *ir)
   {
               /* If a variable is dereferenced at all, remove it from the set of
   * variables that are candidates for removal.
   */
   if (entry != NULL)
                        void remove_dead_variables()
   {
      set_foreach(variables, entry) {
               assert(ir->ir_type == ir_type_variable);
               private:
         };
      static const struct standalone_options *options;
      static void
   initialize_context(struct gl_context *ctx, gl_api api)
   {
      initialize_context_to_defaults(ctx, api);
            /* The standalone compiler needs to claim support for almost
   * everything in order to compile the built-in functions.
   */
   ctx->Const.GLSLVersion = options->glsl_version;
   ctx->Extensions.ARB_ES3_compatibility = true;
   ctx->Extensions.ARB_ES3_1_compatibility = true;
   ctx->Extensions.ARB_ES3_2_compatibility = true;
   ctx->Const.MaxComputeWorkGroupCount[0] = 65535;
   ctx->Const.MaxComputeWorkGroupCount[1] = 65535;
   ctx->Const.MaxComputeWorkGroupCount[2] = 65535;
   ctx->Const.MaxComputeWorkGroupSize[0] = 1024;
   ctx->Const.MaxComputeWorkGroupSize[1] = 1024;
   ctx->Const.MaxComputeWorkGroupSize[2] = 64;
   ctx->Const.MaxComputeWorkGroupInvocations = 1024;
   ctx->Const.MaxComputeSharedMemorySize = 32768;
   ctx->Const.MaxComputeVariableGroupSize[0] = 512;
   ctx->Const.MaxComputeVariableGroupSize[1] = 512;
   ctx->Const.MaxComputeVariableGroupSize[2] = 64;
   ctx->Const.MaxComputeVariableGroupInvocations = 512;
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxCombinedUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxInputComponents = 0; /* not used */
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxOutputComponents = 0; /* not used */
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxAtomicBuffers = 8;
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxAtomicCounters = 8;
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxImageUniforms = 8;
            switch (ctx->Const.GLSLVersion) {
   case 100:
      ctx->Const.MaxClipPlanes = 0;
   ctx->Const.MaxCombinedTextureImageUnits = 8;
   ctx->Const.MaxDrawBuffers = 2;
   ctx->Const.MinProgramTexelOffset = 0;
   ctx->Const.MaxProgramTexelOffset = 0;
   ctx->Const.MaxLights = 0;
   ctx->Const.MaxTextureCoordUnits = 0;
            ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs = 8;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxUniformComponents = 128 * 4;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxCombinedUniformComponents = 128 * 4;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxInputComponents = 0; /* not used */
            ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits =
         ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxUniformComponents = 16 * 4;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxCombinedUniformComponents = 16 * 4;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxInputComponents =
                  ctx->Const.MaxVarying = ctx->Const.Program[MESA_SHADER_VERTEX].MaxOutputComponents / 4;
      case 110:
   case 120:
      ctx->Const.MaxClipPlanes = 6;
   ctx->Const.MaxCombinedTextureImageUnits = 2;
   ctx->Const.MaxDrawBuffers = 1;
   ctx->Const.MinProgramTexelOffset = 0;
   ctx->Const.MaxProgramTexelOffset = 0;
   ctx->Const.MaxLights = 8;
   ctx->Const.MaxTextureCoordUnits = 2;
            ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxUniformComponents = 512;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxCombinedUniformComponents = 512;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxInputComponents = 0; /* not used */
            ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits =
         ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxUniformComponents = 64;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxCombinedUniformComponents = 64;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxInputComponents =
                  ctx->Const.MaxVarying = ctx->Const.Program[MESA_SHADER_VERTEX].MaxOutputComponents / 4;
      case 130:
   case 140:
      ctx->Const.MaxClipPlanes = 8;
   ctx->Const.MaxCombinedTextureImageUnits = 16;
   ctx->Const.MaxDrawBuffers = 8;
   ctx->Const.MinProgramTexelOffset = -8;
   ctx->Const.MaxProgramTexelOffset = 7;
   ctx->Const.MaxLights = 8;
   ctx->Const.MaxTextureCoordUnits = 8;
   ctx->Const.MaxTextureUnits = 2;
   ctx->Const.MaxUniformBufferBindings = 84;
   ctx->Const.MaxVertexStreams = 4;
            ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxCombinedUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxInputComponents = 0; /* not used */
            ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxCombinedUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxInputComponents =
                  ctx->Const.MaxVarying = ctx->Const.Program[MESA_SHADER_VERTEX].MaxOutputComponents / 4;
      case 150:
   case 330:
   case 400:
   case 410:
   case 420:
   case 430:
   case 440:
   case 450:
   case 460:
      ctx->Const.MaxClipPlanes = 8;
   ctx->Const.MaxDrawBuffers = 8;
   ctx->Const.MinProgramTexelOffset = -8;
   ctx->Const.MaxProgramTexelOffset = 7;
   ctx->Const.MaxLights = 8;
   ctx->Const.MaxTextureCoordUnits = 8;
   ctx->Const.MaxTextureUnits = 2;
   ctx->Const.MaxUniformBufferBindings = 84;
   ctx->Const.MaxVertexStreams = 4;
   ctx->Const.MaxTransformFeedbackBuffers = 4;
   ctx->Const.MaxShaderStorageBufferBindings = 4;
   ctx->Const.MaxShaderStorageBlockSize = 4096;
            ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxCombinedUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxInputComponents = 0; /* not used */
            ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxCombinedUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxInputComponents =
                  ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxCombinedUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxInputComponents =
                  ctx->Const.MaxCombinedTextureImageUnits =
      ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits
               ctx->Const.MaxGeometryOutputVertices = 256;
            ctx->Const.MaxVarying = 60 / 4;
      case 300:
      ctx->Const.MaxClipPlanes = 8;
   ctx->Const.MaxCombinedTextureImageUnits = 32;
   ctx->Const.MaxDrawBuffers = 4;
   ctx->Const.MinProgramTexelOffset = -8;
   ctx->Const.MaxProgramTexelOffset = 7;
   ctx->Const.MaxLights = 0;
   ctx->Const.MaxTextureCoordUnits = 0;
   ctx->Const.MaxTextureUnits = 0;
   ctx->Const.MaxUniformBufferBindings = 84;
   ctx->Const.MaxVertexStreams = 4;
            ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxCombinedUniformComponents = 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxInputComponents = 0; /* not used */
            ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxUniformComponents = 224;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxCombinedUniformComponents = 224;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxInputComponents = 15 * 4;
            ctx->Const.MaxVarying = ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxInputComponents / 4;
               ctx->Const.GenerateTemporaryNames = true;
            /* GL_ARB_explicit_uniform_location, GL_MAX_UNIFORM_LOCATIONS */
   ctx->Const.MaxUserAssignableUniformLocations =
      }
      /* Returned string will have 'ctx' as its ralloc owner. */
   static char *
   load_text_file(void *ctx, const char *file_name)
   {
      char *text = NULL;
   size_t size;
   size_t total_read = 0;
            if (!fp) {
                  fseek(fp, 0L, SEEK_END);
   size = ftell(fp);
            text = (char *) ralloc_size(ctx, size + 1);
   if (text != NULL) {
      do {
      size_t bytes = fread(text + total_read,
         if (bytes < size - total_read) {
      text = NULL;
               if (bytes == 0) {
                              text[total_read] = '\0';
                           }
      static void
   compile_shader(struct gl_context *ctx, struct gl_shader *shader)
   {
      _mesa_glsl_compile_shader(ctx, shader, options->dump_ast,
            /* Print out the resulting IR */
   if (shader->CompileStatus == COMPILE_SUCCESS && options->dump_lir) {
                     }
      extern "C" struct gl_shader_program *
   standalone_compile_shader(const struct standalone_options *_options,
         {
      int status = EXIT_SUCCESS;
                     switch (options->glsl_version) {
   case 100:
   case 300:
      glsl_es = true;
      case 110:
   case 120:
   case 130:
   case 140:
   case 150:
   case 330:
   case 400:
   case 410:
   case 420:
   case 430:
   case 440:
   case 450:
   case 460:
      glsl_es = false;
      default:
      fprintf(stderr, "Unrecognized GLSL version `%d'\n", options->glsl_version);
               if (glsl_es) {
         } else {
                  if (options->lower_precision) {
      for (unsigned i = MESA_SHADER_VERTEX; i <= MESA_SHADER_COMPUTE; i++) {
      struct gl_shader_compiler_options *options =
         options->LowerPrecisionFloat16 = true;
   options->LowerPrecisionInt16 = true;
   options->LowerPrecisionDerivatives = true;
   options->LowerPrecisionConstants = true;
                           for (unsigned i = 0; i < num_files; i++) {
      const unsigned len = strlen(files[i]);
   if (len < 6)
            const char *const ext = & files[i][len - 5];
   /* TODO add support to read a .shader_test */
   GLenum type;
   if (strncmp(".vert", ext, 5) == 0 || strncmp(".glsl", ext, 5) == 0)
         else if (strncmp(".tesc", ext, 5) == 0)
         else if (strncmp(".tese", ext, 5) == 0)
         else if (strncmp(".geom", ext, 5) == 0)
         else if (strncmp(".frag", ext, 5) == 0)
         else if (strncmp(".comp", ext, 5) == 0)
         else
            const char *source = load_text_file(whole_program, files[i]);
   if (source == NULL) {
      printf("File \"%s\" does not exist.\n", files[i]);
                                 if (strlen(shader->InfoLog) > 0) {
                     printf("%s", shader->InfoLog);
   if (!options->just_log)
               if (!shader->CompileStatus) {
      status = EXIT_FAILURE;
                  if (status == EXIT_SUCCESS) {
               if (options->do_link)  {
         } else {
               whole_program->data->LinkStatus = LINKING_SUCCESS;
   whole_program->_LinkedShaders[stage] =
      link_intrastage_shaders(whole_program /* mem_ctx */,
                                 /* Par-linking can fail, for example, if there are undefined external
   * references.
   */
                                                   bool progress;
                     progress = do_common_optimization(ir,
                                                if (strlen(whole_program->data->InfoLog) > 0) {
      printf("\n");
   if (!options->just_log)
         printf("%s", whole_program->data->InfoLog);
   if (!options->just_log)
               for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
                                             dead_variable_visitor dv;
   visit_list_elements(&dv, shader->ir);
               if (options->dump_builder) {
                                                               fail:
      for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (whole_program->_LinkedShaders[i])
               ralloc_free(whole_program);
      }
      extern "C" void
   standalone_compiler_cleanup(struct gl_shader_program *whole_program)
   {
                  }
