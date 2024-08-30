   /*
   * Copyright © 2015 Red Hat
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "st_nir.h"
      #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_context.h"
      #include "program/program.h"
   #include "program/prog_statevars.h"
   #include "program/prog_parameter.h"
   #include "main/context.h"
   #include "main/mtypes.h"
   #include "main/errors.h"
   #include "main/glspirv.h"
   #include "main/shaderapi.h"
   #include "main/uniforms.h"
      #include "main/shaderobj.h"
   #include "st_context.h"
   #include "st_program.h"
   #include "st_shader_cache.h"
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/glsl_types.h"
   #include "compiler/glsl/glsl_to_nir.h"
   #include "compiler/glsl/gl_nir.h"
   #include "compiler/glsl/gl_nir_linker.h"
   #include "compiler/glsl/ir.h"
   #include "compiler/glsl/ir_optimization.h"
   #include "compiler/glsl/linker_util.h"
   #include "compiler/glsl/program.h"
   #include "compiler/glsl/shader_cache.h"
   #include "compiler/glsl/string_to_uint_map.h"
      static int
   type_size(const struct glsl_type *type)
   {
         }
      /* Depending on PIPE_CAP_TGSI_TEXCOORD (st->needs_texcoord_semantic) we
   * may need to fix up varying slots so the glsl->nir path is aligned
   * with the anything->tgsi->nir path.
   */
   static void
   st_nir_fixup_varying_slots(struct st_context *st, nir_shader *shader,
         {
      if (st->needs_texcoord_semantic)
            /* This is called from finalize, but we don't want to do this adjustment twice. */
            nir_foreach_variable_with_modes(var, shader, mode) {
      if (var->data.location >= VARYING_SLOT_VAR0 && var->data.location < VARYING_SLOT_PATCH0) {
         } else if (var->data.location == VARYING_SLOT_PNTC) {
         } else if ((var->data.location >= VARYING_SLOT_TEX0) &&
                     }
      /* input location assignment for VS inputs must be handled specially, so
   * that it is aligned w/ st's vbo state.
   * (This isn't the case with, for ex, FS inputs, which only need to agree
   * on varying-slot w/ the VS outputs)
   */
   void
   st_nir_assign_vs_in_locations(struct nir_shader *nir)
   {
      if (nir->info.stage != MESA_SHADER_VERTEX || nir->info.io_lowered)
                              nir_foreach_shader_in_variable_safe(var, nir) {
      /* NIR already assigns dual-slot inputs to two locations so all we have
   * to do is compact everything down.
   */
   if (nir->info.inputs_read & BITFIELD64_BIT(var->data.location)) {
      var->data.driver_location =
      util_bitcount64(nir->info.inputs_read &
   } else {
      /* Convert unused input variables to shader_temp (with no
   * initialization), to avoid confusing drivers looking through the
   * inputs array and expecting to find inputs with a driver_location
   * set.
   */
   var->data.mode = nir_var_shader_temp;
                  /* Re-lower global vars, to deal with any dead VS inputs. */
   if (removed_inputs)
      }
      static int
   st_nir_lookup_parameter_index(struct gl_program *prog, nir_variable *var)
   {
               /* Lookup the first parameter that the uniform storage that match the
   * variable location.
   */
   for (unsigned i = 0; i < params->NumParameters; i++) {
      int index = params->Parameters[i].MainUniformStorageIndex;
   if (index == var->data.location)
               /* TODO: Handle this fallback for SPIR-V.  We need this for GLSL e.g. in
   * dEQP-GLES2.functional.uniform_api.random.3
            /* is there a better way to do this?  If we have something like:
   *
   *    struct S {
   *           float f;
   *           vec4 v;
   *    };
   *    uniform S color;
   *
   * Then what we get in prog->Parameters looks like:
   *
   *    0: Name=color.f, Type=6, DataType=1406, Size=1
   *    1: Name=color.v, Type=6, DataType=8b52, Size=4
   *
   * So the name doesn't match up and _mesa_lookup_parameter_index()
   * fails.  In this case just find the first matching "color.*"..
   *
   * Note for arrays you could end up w/ color[n].f, for example.
   */
   if (!prog->sh.data->spirv) {
      int namelen = strlen(var->name);
   for (unsigned i = 0; i < params->NumParameters; i++) {
      struct gl_program_parameter *p = &params->Parameters[i];
   if ((strncmp(p->Name, var->name, namelen) == 0) &&
      ((p->Name[namelen] == '.') || (p->Name[namelen] == '['))) {
                        }
      static void
   st_nir_assign_uniform_locations(struct gl_context *ctx,
               {
      int shaderidx = 0;
            nir_foreach_variable_with_modes(uniform, nir, nir_var_uniform |
                     const struct glsl_type *type = glsl_without_array(uniform->type);
   if (!uniform->data.bindless && (type->is_sampler() || type->is_image())) {
      if (type->is_sampler()) {
      loc = shaderidx;
      } else {
      loc = imageidx;
         } else if (uniform->state_slots) {
               unsigned comps;
   if (glsl_type_is_struct_or_ifc(type)) {
         } else {
                  if (ctx->Const.PackedDriverUniformStorage) {
      loc = _mesa_add_sized_state_reference(prog->Parameters,
            } else {
            } else {
               /* We need to check that loc is not -1 here before accessing the
   * array. It can be negative for example when we have a struct that
   * only contains opaque types.
   */
   if (loc >= 0 && ctx->Const.PackedDriverUniformStorage) {
                           }
      static bool
   def_is_64bit(nir_def *def, void *state)
   {
      bool *lower = (bool *)state;
   if (def && (def->bit_size == 64)) {
      *lower = true;
      }
      }
      static bool
   src_is_64bit(nir_src *src, void *state)
   {
      bool *lower = (bool *)state;
   if (src && (nir_src_bit_size(*src) == 64)) {
      *lower = true;
      }
      }
      static bool
   filter_64_bit_instr(const nir_instr *const_instr, UNUSED const void *data)
   {
      bool lower = false;
   /* lower_alu_to_scalar required nir_instr to be const, but nir_foreach_*
   * doesn't have const variants, so do the ugly const_cast here. */
            nir_foreach_def(instr, def_is_64bit, &lower);
   if (lower)
         nir_foreach_src(instr, src_is_64bit, &lower);
      }
      /* Second third of converting glsl_to_nir. This creates uniforms, gathers
   * info on varyings, etc after NIR link time opts have been applied.
   */
   static char *
   st_glsl_to_nir_post_opts(struct st_context *st, struct gl_program *prog,
         {
      nir_shader *nir = prog->nir;
            /* Make a pass over the IR to add state references for any built-in
   * uniforms that are used.  This has to be done now (during linking).
   * Code generation doesn't happen until the first time this shader is
   * used for rendering.  Waiting until then to generate the parameters is
   * too late.  At that point, the values for the built-in uniforms won't
   * get sent to the shader.
   */
   nir_foreach_uniform_variable(var, nir) {
      const nir_state_slot *const slots = var->state_slots;
   if (slots != NULL) {
      const struct glsl_type *type = glsl_without_array(var->type);
   for (unsigned int i = 0; i < var->num_state_slots; i++) {
      unsigned comps;
   if (glsl_type_is_struct_or_ifc(type)) {
         } else {
                  if (st->ctx->Const.PackedDriverUniformStorage) {
      _mesa_add_sized_state_reference(prog->Parameters,
            } else {
      _mesa_add_state_reference(prog->Parameters,
                        /* Avoid reallocation of the program parameter list, because the uniform
   * storage is only associated with the original parameter list.
   * This should be enough for Bitmap and DrawPixels constants.
   */
            /* None of the builtins being lowered here can be produced by SPIR-V.  See
   * _mesa_builtin_uniform_desc. Also drivers that support packed uniform
   * storage don't need to lower builtins.
   */
   if (!shader_program->data->spirv &&
      !st->ctx->Const.PackedDriverUniformStorage)
         if (!screen->get_param(screen, PIPE_CAP_NIR_ATOMICS_AS_DEREF))
            NIR_PASS_V(nir, nir_opt_intrinsics);
            /* Lower 64-bit ops. */
   if (nir->options->lower_int64_options ||
      nir->options->lower_doubles_options) {
   bool lowered_64bit_ops = false;
            if (nir->options->lower_doubles_options) {
      /* nir_lower_doubles is not prepared for vector ops, so if the backend doesn't
   * request lower_alu_to_scalar until now, lower all 64 bit ops, and try to
   * vectorize them afterwards again */
   if (!nir->options->lower_to_scalar) {
      NIR_PASS(revectorize, nir, nir_lower_alu_to_scalar, filter_64_bit_instr, nullptr);
      }
   /* doubles lowering requires frexp to be lowered first if it will be,
   * since the pass generates other 64-bit ops.  Most backends lower
   * frexp, and using doubles is rare, and using frexp is even more rare
   * (no instances in shader-db), so we're not too worried about
   * accidentally lowering a 32-bit frexp here.
                  NIR_PASS(lowered_64bit_ops, nir, nir_lower_doubles,
      }
   if (nir->options->lower_int64_options)
            if (revectorize && !nir->options->vectorize_vec2_16bit)
            if (revectorize || lowered_64bit_ops)
               nir_variable_mode mask =
                  if (!st->has_hw_atomics && !screen->get_param(screen, PIPE_CAP_NIR_ATOMICS_AS_DEREF)) {
      unsigned align_offset_state = 0;
   if (st->ctx->Const.ShaderStorageBufferOffsetAlignment > 4) {
      struct gl_program_parameter_list *params = prog->Parameters;
   for (unsigned i = 0; i < shader_program->data->NumAtomicBuffers; i++) {
      gl_state_index16 state[STATE_LENGTH] = { STATE_ATOMIC_COUNTER_OFFSET, (short)shader_program->data->AtomicBuffers[i].Binding };
      }
      }
                                 char *msg = NULL;
   if (st->allow_st_finalize_nir_twice)
            if (st->ctx->_Shader->Flags & GLSL_DUMP) {
      _mesa_log("\n");
   _mesa_log("NIR IR for linked %s program %d:\n",
         _mesa_shader_stage_to_string(prog->info.stage),
   nir_print_shader(nir, _mesa_get_log_file());
                  }
      static void
   st_nir_vectorize_io(nir_shader *producer, nir_shader *consumer)
   {
      if (consumer)
            if (!producer)
                     if (producer->info.stage == MESA_SHADER_TESS_CTRL &&
      producer->options->vectorize_tess_levels)
                  if ((producer)->info.stage != MESA_SHADER_TESS_CTRL) {
      /* Calling lower_io_to_vector creates output variable writes with
   * write-masks.  We only support these for TCS outputs, so for other
   * stages, we need to call nir_lower_io_to_temporaries to get rid of
   * them.  This, in turn, creates temporary variables and extra
   * copy_deref intrinsics that we need to clean up.
   */
   NIR_PASS_V(producer, nir_lower_io_to_temporaries,
         NIR_PASS_V(producer, nir_lower_global_vars_to_local);
   NIR_PASS_V(producer, nir_split_var_copies);
               /* Undef scalar store_deref intrinsics are not ignored by nir_lower_io,
   * so they must be removed before that. These passes remove them.
   */
   NIR_PASS_V(producer, nir_lower_vars_to_ssa);
   NIR_PASS_V(producer, nir_opt_undef);
      }
      extern "C" {
      void
   st_nir_lower_wpos_ytransform(struct nir_shader *nir,
               {
      if (nir->info.stage != MESA_SHADER_FRAGMENT)
            static const gl_state_index16 wposTransformState[STATE_LENGTH] = {
         };
            memcpy(wpos_options.state_tokens, wposTransformState,
         wpos_options.fs_coord_origin_upper_left =
      pscreen->get_param(pscreen,
      wpos_options.fs_coord_origin_lower_left =
      pscreen->get_param(pscreen,
      wpos_options.fs_coord_pixel_center_integer =
      pscreen->get_param(pscreen,
      wpos_options.fs_coord_pixel_center_half_integer =
      pscreen->get_param(pscreen,
         if (nir_lower_wpos_ytransform(nir, &wpos_options)) {
      nir_validate_shader(nir, "after nir_lower_wpos_ytransform");
               static const gl_state_index16 pntcTransformState[STATE_LENGTH] = {
                  if (nir_lower_pntc_ytransform(nir, &pntcTransformState)) {
            }
      static bool
   st_link_glsl_to_nir(struct gl_context *ctx,
         {
      struct st_context *st = st_context(ctx);
   struct gl_linked_shader *linked_shader[MESA_SHADER_STAGES];
            /* Return early if we are loading the shader from on-disk cache */
   if (st_load_nir_from_disk_cache(ctx, shader_program)) {
                                    /* Skip the GLSL steps when using SPIR-V. */
   if (!shader_program->data->spirv) {
      for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
                                    lower_packing_builtins(ir, ctx->Extensions.ARB_shading_language_packing,
                                                         for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (shader_program->_LinkedShaders[i])
               for (unsigned i = 0; i < num_shaders; i++) {
      struct gl_linked_shader *shader = linked_shader[i];
   const nir_shader_compiler_options *options =
                           assert(!prog->nir);
   prog->shader_program = shader_program;
            /* Parameters will be filled during NIR linking. */
            if (shader_program->data->spirv) {
         } else {
               if (ctx->_Shader->Flags & GLSL_DUMP) {
      _mesa_log("\n");
   _mesa_log("GLSL IR for linked %s program %d:\n",
               _mesa_print_ir(_mesa_get_log_file(), shader->ir, NULL);
                           memcpy(prog->nir->info.source_sha1, shader->linked_source_sha1,
            nir_shader_gather_info(prog->nir, nir_shader_get_entrypoint(prog->nir));
   if (!st->ctx->SoftFP64 && ((prog->nir->info.bit_sizes_int | prog->nir->info.bit_sizes_float) & 64) &&
               /* It's not possible to use float64 on GLSL ES, so don't bother trying to
   * build the support code.  The support code depends on higher versions of
   * desktop GLSL, so it will fail to compile (below) anyway.
   */
   if (_mesa_is_desktop_gl(st->ctx) && st->ctx->Const.GLSLVersion >= 400)
                  if (shader_program->data->spirv) {
      static const gl_nir_linker_options opts = {
         };
   if (!gl_nir_link_spirv(&ctx->Const, &ctx->Extensions, shader_program,
            } else {
      if (!gl_nir_link_glsl(&ctx->Const, &ctx->Extensions, ctx->API,
                     for (unsigned i = 0; i < num_shaders; i++) {
      struct gl_program *prog = linked_shader[i]->Program;
   prog->ExternalSamplersUsed = gl_external_samplers(prog);
               nir_build_program_resource_list(&ctx->Const, shader_program,
            for (unsigned i = 0; i < num_shaders; i++) {
      struct gl_linked_shader *shader = linked_shader[i];
   nir_shader *nir = shader->Program->nir;
   gl_shader_stage stage = shader->Stage;
   const struct gl_shader_compiler_options *options =
            /* If there are forms of indirect addressing that the driver
   * cannot handle, perform the lowering pass.
   */
   if (options->EmitNoIndirectInput || options->EmitNoIndirectOutput ||
      options->EmitNoIndirectTemp || options->EmitNoIndirectUniform) {
   nir_variable_mode mode = options->EmitNoIndirectInput ?
         mode |= options->EmitNoIndirectOutput ?
         mode |= options->EmitNoIndirectTemp ?
         mode |= options->EmitNoIndirectUniform ?
                              /* This needs to run after the initial pass of nir_lower_vars_to_ssa, so
   * that the buffer indices are constants in nir where they where
   * constants in GLSL. */
            /* Remap the locations to slots so those requiring two slots will occupy
   * two locations. For instance, if we have in the IR code a dvec3 attr0 in
   * location 0 and vec4 attr1 in location 1, in NIR attr0 will use
   * locations/slots 0 and 1, and attr1 will use location/slot 2
   */
   if (nir->info.stage == MESA_SHADER_VERTEX && !shader_program->data->spirv)
            NIR_PASS_V(nir, st_nir_lower_wpos_ytransform, shader->Program,
            NIR_PASS_V(nir, nir_lower_system_values);
            if (i >= 1) {
               /* We can't use nir_compact_varyings with transform feedback, since
   * the pipe_stream_output->output_register field is based on the
   * pre-compacted driver_locations.
   */
   if (!(prev_shader->sh.LinkedTransformFeedback &&
                        if (ctx->Const.ShaderCompilerOptions[shader->Stage].NirOptions->vectorize_io)
                  /* If the program is a separate shader program check if we need to vectorise
   * the first and last program interfaces too.
   */
   if (shader_program->SeparateShader && num_shaders > 0) {
      struct gl_linked_shader *first_shader = linked_shader[0];
   struct gl_linked_shader *last_shader = linked_shader[num_shaders - 1];
   if (first_shader->Stage != MESA_SHADER_COMPUTE) {
      if (ctx->Const.ShaderCompilerOptions[first_shader->Stage].NirOptions->vectorize_io &&
                  if (ctx->Const.ShaderCompilerOptions[last_shader->Stage].NirOptions->vectorize_io &&
      last_shader->Stage < MESA_SHADER_FRAGMENT)
                        for (unsigned i = 0; i < num_shaders; i++) {
      struct gl_linked_shader *shader = linked_shader[i];
            char *msg = st_glsl_to_nir_post_opts(st, shader->Program, shader_program);
   if (msg) {
      linker_error(shader_program, msg);
               if (prev_info &&
      ctx->Const.ShaderCompilerOptions[shader->Stage].NirOptions->unify_interfaces) {
   prev_info->outputs_written |= info->inputs_read &
                        prev_info->patch_outputs_written |= info->patch_inputs_read;
      }
               for (unsigned i = 0; i < num_shaders; i++) {
      struct gl_linked_shader *shader = linked_shader[i];
            /* Make sure that prog->info is in sync with nir->info, but st/mesa
   * expects some of the values to be from before lowering.
   */
   shader_info old_info = prog->info;
   prog->info = prog->nir->info;
   prog->info.name = old_info.name;
   prog->info.label = old_info.label;
   prog->info.num_ssbos = old_info.num_ssbos;
   prog->info.num_ubos = old_info.num_ubos;
            if (prog->info.stage == MESA_SHADER_VERTEX) {
      /* NIR expands dual-slot inputs out to two locations.  We need to
   * compact things back down GL-style single-slot inputs to avoid
   * confusing the state tracker.
   */
   prog->info.inputs_read =
                  /* Initialize st_vertex_program members. */
               /* Get pipe_stream_output_info. */
   if (shader->Stage == MESA_SHADER_VERTEX ||
      shader->Stage == MESA_SHADER_TESS_EVAL ||
                        st_release_variants(st, prog);
               struct pipe_context *pctx = st_context(ctx)->pipe;
   if (pctx->link_shader) {
      void *driver_handles[PIPE_SHADER_TYPES];
            for (uint32_t i = 0; i < MESA_SHADER_STAGES; ++i) {
      struct gl_linked_shader *shader = shader_program->_LinkedShaders[i];
   if (shader) {
      struct gl_program *p = shader->Program;
   if (p && p->variants) {
      enum pipe_shader_type type = pipe_shader_type_from_mesa(shader->Stage);
                                    }
      void
   st_nir_assign_varying_locations(struct st_context *st, nir_shader *nir)
   {
      if (nir->info.stage == MESA_SHADER_VERTEX) {
      nir_assign_io_var_locations(nir, nir_var_shader_out,
                  } else if (nir->info.stage == MESA_SHADER_GEOMETRY ||
            nir->info.stage == MESA_SHADER_TESS_CTRL ||
   nir_assign_io_var_locations(nir, nir_var_shader_in,
                        nir_assign_io_var_locations(nir, nir_var_shader_out,
                  } else if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      nir_assign_io_var_locations(nir, nir_var_shader_in,
               st_nir_fixup_varying_slots(st, nir, nir_var_shader_in);
   nir_assign_io_var_locations(nir, nir_var_shader_out,
            } else if (nir->info.stage == MESA_SHADER_COMPUTE) {
         } else {
            }
      void
   st_nir_lower_samplers(struct pipe_screen *screen, nir_shader *nir,
               {
      if (screen->get_param(screen, PIPE_CAP_NIR_SAMPLERS_AS_DEREF))
         else
            if (prog) {
      BITSET_COPY(prog->info.textures_used, nir->info.textures_used);
   BITSET_COPY(prog->info.textures_used_by_txf, nir->info.textures_used_by_txf);
   BITSET_COPY(prog->info.samplers_used, nir->info.samplers_used);
   BITSET_COPY(prog->info.images_used, nir->info.images_used);
   BITSET_COPY(prog->info.image_buffers, nir->info.image_buffers);
         }
      static int
   st_packed_uniforms_type_size(const struct glsl_type *type, bool bindless)
   {
         }
      static int
   st_unpacked_uniforms_type_size(const struct glsl_type *type, bool bindless)
   {
         }
      void
   st_nir_lower_uniforms(struct st_context *st, nir_shader *nir)
   {
      if (st->ctx->Const.PackedDriverUniformStorage) {
      NIR_PASS_V(nir, nir_lower_io, nir_var_uniform,
            } else {
      NIR_PASS_V(nir, nir_lower_io, nir_var_uniform,
                     if (nir->options->lower_uniforms_to_ubo)
      NIR_PASS_V(nir, nir_lower_uniforms_to_ubo,
         }
      /* Last third of preparing nir from glsl, which happens after shader
   * variant lowering.
   */
   char *
   st_finalize_nir(struct st_context *st, struct gl_program *prog,
                     {
                        NIR_PASS_V(nir, nir_split_var_copies);
            const bool lower_tg4_offsets =
            if (st->lower_rect_tex || lower_tg4_offsets) {
      struct nir_lower_tex_options opts = {0};
   opts.lower_rect = !!st->lower_rect_tex;
                        st_nir_assign_varying_locations(st, nir);
            /* Lower load_deref/store_deref of inputs and outputs.
   * This depends on st_nir_assign_varying_locations.
   */
   if (nir->options->lower_io_variables) {
      nir_lower_io_passes(nir, false);
   NIR_PASS_V(nir, nir_remove_dead_variables,
               /* Set num_uniforms in number of attribute slots (vec4s) */
                     if (is_before_variants && nir->options->lower_uniforms_to_ubo) {
      /* This must be done after uniforms are lowered to UBO and all
   * nir_var_uniform variables are removed from NIR to prevent conflicts
   * between state parameter merging and shader variant generation.
   */
               st_nir_lower_samplers(screen, nir, shader_program, prog);
   if (!screen->get_param(screen, PIPE_CAP_NIR_IMAGES_AS_DEREF))
            char *msg = NULL;
   if (finalize_by_driver && screen->finalize_nir)
               }
      /**
   * Link a GLSL shader program.  Called via glLinkProgram().
   */
   void
   st_link_shader(struct gl_context *ctx, struct gl_shader_program *prog)
   {
      unsigned int i;
                                                for (i = 0; i < prog->NumShaders; i++) {
      linker_error(prog, "linking with uncompiled/unspecialized shader");
                  if (!i) {
         } else if (spirv && !prog->Shaders[i]->spirv_data) {
      /* The GL_ARB_gl_spirv spec adds a new bullet point to the list of
   * reasons LinkProgram can fail:
   *
   *    "All the shader objects attached to <program> do not have the
   *     same value for the SPIR_V_BINARY_ARB state."
   */
   linker_error(prog,
               }
            if (prog->data->LinkStatus) {
      if (!spirv)
         else
               /* If LinkStatus is LINKING_SUCCESS, then reset sampler validated to true.
   * Validation happens via the LinkShader call below. If LinkStatus is
   * LINKING_SKIPPED, then SamplersValidated will have been restored from the
   * shader cache.
   */
   if (prog->data->LinkStatus == LINKING_SUCCESS) {
                  if (prog->data->LinkStatus && !st_link_glsl_to_nir(ctx, prog)) {
                  if (prog->data->LinkStatus != LINKING_FAILURE)
            /* Return early if we are loading the shader from on-disk cache */
   if (prog->data->LinkStatus == LINKING_SKIPPED)
            if (ctx->_Shader->Flags & GLSL_DUMP) {
      fprintf(stderr, "GLSL shader program %d failed to link\n", prog->Name);
                  fprintf(stderr, "GLSL shader program %d info log:\n", prog->Name);
                        #ifdef ENABLE_SHADER_CACHE
      if (prog->data->LinkStatus)
      #endif
   }
      } /* extern "C" */
