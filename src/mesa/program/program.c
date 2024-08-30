   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      /**
   * \file program.c
   * Vertex and fragment program support functions.
   * \author Brian Paul
   */
         #include "util/glheader.h"
   #include "main/context.h"
   #include "main/framebuffer.h"
   #include "main/hash.h"
   #include "main/macros.h"
   #include "main/shaderobj.h"
   #include "main/state.h"
   #include "program.h"
   #include "prog_cache.h"
   #include "prog_parameter.h"
   #include "prog_instruction.h"
   #include "util/bitscan.h"
   #include "util/ralloc.h"
   #include "util/u_atomic.h"
      #include "state_tracker/st_program.h"
   #include "state_tracker/st_context.h"
      /**
   * A pointer to this dummy program is put into the hash table when
   * glGenPrograms is called.
   */
   struct gl_program _mesa_DummyProgram;
         /**
   * Init context's vertex/fragment program state
   */
   void
   _mesa_init_program(struct gl_context *ctx)
   {
      /*
   * If this assertion fails, we need to increase the field
   * size for register indexes (see INST_INDEX_BITS).
   */
   assert(ctx->Const.Program[MESA_SHADER_VERTEX].MaxUniformComponents / 4
         assert(ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxUniformComponents / 4
            assert(ctx->Const.Program[MESA_SHADER_VERTEX].MaxTemps <= (1 << INST_INDEX_BITS));
   assert(ctx->Const.Program[MESA_SHADER_VERTEX].MaxLocalParams <= (1 << INST_INDEX_BITS));
   assert(ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTemps <= (1 << INST_INDEX_BITS));
            assert(ctx->Const.Program[MESA_SHADER_VERTEX].MaxUniformComponents <= 4 * MAX_UNIFORMS);
            assert(ctx->Const.Program[MESA_SHADER_VERTEX].MaxAddressOffset <= (1 << INST_INDEX_BITS));
            /* If this fails, increase prog_instruction::TexSrcUnit size */
            /* If this fails, increase prog_instruction::TexSrcTarget size */
            ctx->Program.ErrorPos = -1;
            ctx->VertexProgram._VaryingInputs = VERT_BIT_ALL;
   ctx->VertexProgram.Enabled = GL_FALSE;
   ctx->VertexProgram.PointSizeEnabled =
         ctx->VertexProgram.TwoSideEnabled = GL_FALSE;
   _mesa_reference_program(ctx, &ctx->VertexProgram.Current,
         assert(ctx->VertexProgram.Current);
            ctx->FragmentProgram.Enabled = GL_FALSE;
   _mesa_reference_program(ctx, &ctx->FragmentProgram.Current,
         assert(ctx->FragmentProgram.Current);
   ctx->FragmentProgram.Cache = _mesa_new_program_cache();
            /* XXX probably move this stuff */
   ctx->ATIFragmentShader.Enabled = GL_FALSE;
   ctx->ATIFragmentShader.Current = ctx->Shared->DefaultFragmentShader;
   assert(ctx->ATIFragmentShader.Current);
      }
         /**
   * Free a context's vertex/fragment program state
   */
   void
   _mesa_free_program_data(struct gl_context *ctx)
   {
      _mesa_reference_program(ctx, &ctx->VertexProgram.Current, NULL);
   _mesa_delete_program_cache(ctx, ctx->VertexProgram.Cache);
   _mesa_reference_program(ctx, &ctx->FragmentProgram.Current, NULL);
            /* XXX probably move this stuff */
   if (ctx->ATIFragmentShader.Current) {
      ctx->ATIFragmentShader.Current->RefCount--;
   if (ctx->ATIFragmentShader.Current->RefCount <= 0) {
                        }
         /**
   * Update the default program objects in the given context to reference those
   * specified in the shared state and release those referencing the old
   * shared state.
   */
   void
   _mesa_update_default_objects_program(struct gl_context *ctx)
   {
      _mesa_reference_program(ctx, &ctx->VertexProgram.Current,
                  _mesa_reference_program(ctx, &ctx->FragmentProgram.Current,
                  /* XXX probably move this stuff */
   if (ctx->ATIFragmentShader.Current) {
      ctx->ATIFragmentShader.Current->RefCount--;
   if (ctx->ATIFragmentShader.Current->RefCount <= 0) {
            }
   ctx->ATIFragmentShader.Current = (struct ati_fragment_shader *) ctx->Shared->DefaultFragmentShader;
   assert(ctx->ATIFragmentShader.Current);
      }
         /**
   * Set the vertex/fragment program error state (position and error string).
   * This is generally called from within the parsers.
   */
   void
   _mesa_set_program_error(struct gl_context *ctx, GLint pos, const char *string)
   {
      ctx->Program.ErrorPos = pos;
   free((void *) ctx->Program.ErrorString);
   if (!string)
            }
         /**
   * Initialize a new gl_program object.
   */
   struct gl_program *
   _mesa_init_gl_program(struct gl_program *prog, gl_shader_stage stage,
         {
      if (!prog)
            memset(prog, 0, sizeof(*prog));
   prog->Id = id;
   prog->Target = _mesa_shader_stage_to_program(stage);
   prog->RefCount = 1;
   prog->Format = GL_PROGRAM_FORMAT_ASCII_ARB;
   prog->info.stage = stage;
            /* Uniforms that lack an initializer in the shader code have an initial
   * value of zero.  This includes sampler uniforms.
   *
   * Page 24 (page 30 of the PDF) of the GLSL 1.20 spec says:
   *
   *     "The link time initial value is either the value of the variable's
   *     initializer, if present, or 0 if no initializer is present. Sampler
   *     types cannot have initializers."
   *
   * So we only initialise ARB assembly style programs.
   */
   if (is_arb_asm) {
      /* default mapping from samplers to texture units */
   for (unsigned i = 0; i < MAX_SAMPLERS; i++)
                  }
      struct gl_program *
   _mesa_new_program(struct gl_context *ctx, gl_shader_stage stage, GLuint id,
         {
               switch (stage) {
   case MESA_SHADER_VERTEX:
      prog = (struct gl_program*)rzalloc(NULL, struct gl_vertex_program);
      default:
      prog = rzalloc(NULL, struct gl_program);
                  }
      /**
   * Delete a program and remove it from the hash table, ignoring the
   * reference count.
   */
   void
   _mesa_delete_program(struct gl_context *ctx, struct gl_program *prog)
   {
      struct st_context *st = st_context(ctx);
   assert(prog);
                              if (prog == &_mesa_DummyProgram)
            if (prog->Parameters) {
                  if (prog->nir) {
                  if (prog->sh.BindlessSamplers) {
                  if (prog->sh.BindlessImages) {
                  if (prog->driver_cache_blob) {
                     }
         /**
   * Return the gl_program object for a given ID.
   * Basically just a wrapper for _mesa_HashLookup() to avoid a lot of
   * casts elsewhere.
   */
   struct gl_program *
   _mesa_lookup_program(struct gl_context *ctx, GLuint id)
   {
      if (id)
         else
      }
         /**
   * Reference counting for vertex/fragment programs
   * This is normally only called from the _mesa_reference_program() macro
   * when there's a real pointer change.
   */
   void
   _mesa_reference_program_(struct gl_context *ctx,
               {
   #ifndef NDEBUG
      assert(ptr);
   if (*ptr && prog) {
      /* sanity check */
   if ((*ptr)->Target == GL_VERTEX_PROGRAM_ARB)
         else if ((*ptr)->Target == GL_FRAGMENT_PROGRAM_ARB)
      assert(prog->Target == GL_FRAGMENT_PROGRAM_ARB ||
      else if ((*ptr)->Target == GL_GEOMETRY_PROGRAM_NV)
         #endif
         if (*ptr) {
                        if (p_atomic_dec_zero(&oldProg->RefCount)) {
      assert(ctx);
   _mesa_reference_shader_program_data(&oldProg->sh.data, NULL);
                           assert(!*ptr);
   if (prog) {
                     }
      /* Gets the minimum number of shader invocations per fragment.
   * This function is useful to determine if we need to do per
   * sample shading or per fragment shading.
   */
   GLint
   _mesa_get_min_invocations_per_fragment(struct gl_context *ctx,
         {
      /* From ARB_sample_shading specification:
   * "Using gl_SampleID in a fragment shader causes the entire shader
   *  to be evaluated per-sample."
   *
   * "Using gl_SamplePosition in a fragment shader causes the entire
   *  shader to be evaluated per-sample."
   *
   * "If MULTISAMPLE or SAMPLE_SHADING_ARB is disabled, sample shading
   *  has no effect."
   */
   if (ctx->Multisample.Enabled) {
      /* The ARB_gpu_shader5 specification says:
   *
   * "Use of the "sample" qualifier on a fragment shader input
   *  forces per-sample shading"
   */
   if (prog->info.fs.uses_sample_qualifier ||
      BITSET_TEST(prog->info.system_values_read, SYSTEM_VALUE_SAMPLE_ID) ||
   BITSET_TEST(prog->info.system_values_read, SYSTEM_VALUE_SAMPLE_POS))
      else if (ctx->Multisample.SampleShading)
      return MAX2(ceilf(ctx->Multisample.MinSampleShadingValue *
      else
      }
      }
         GLbitfield
   gl_external_samplers(const struct gl_program *prog)
   {
      GLbitfield external_samplers = 0;
            while (mask) {
      int idx = u_bit_scan(&mask);
   if (prog->sh.SamplerTargets[idx] == TEXTURE_EXTERNAL_INDEX)
                  }
      static int compare_state_var(const void *a1, const void *a2)
   {
      const struct gl_program_parameter *p1 =
         const struct gl_program_parameter *p2 =
            for (unsigned i = 0; i < STATE_LENGTH; i++) {
      if (p1->StateIndexes[i] != p2->StateIndexes[i])
      }
      }
      void
   _mesa_add_separate_state_parameters(struct gl_program *prog,
         {
               /* All state parameters should be vec4s. */
   for (unsigned i = 0; i < num_state_params; i++) {
      assert(state_params->Parameters[i].Type == PROGRAM_STATE_VAR);
   assert(state_params->Parameters[i].Size == 4);
               /* Sort state parameters to facilitate better parameter merging. */
   qsort(state_params->Parameters, num_state_params,
                  /* Add state parameters to the end of the parameter list. */
   for (unsigned i = 0; i < num_state_params; i++) {
               remap[old_index] =
      _mesa_add_parameter(prog->Parameters, PROGRAM_STATE_VAR,
                                 prog->Parameters->StateFlags |=
               /* Fix up state parameter offsets in instructions. */
   int num_instr = prog->arb.NumInstructions;
            /* Fix src indices after sorting. */
   for (unsigned i = 0; i < num_instr; i++) {
      struct prog_instruction *inst = instrs + i;
            for (unsigned j = 0; j < num_src; j++) {
      if (inst->SrcReg[j].File == PROGRAM_STATE_VAR)
         }
      }
