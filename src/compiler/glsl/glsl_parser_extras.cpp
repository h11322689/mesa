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
   #include <inttypes.h> /* for PRIx64 macro */
   #include <stdio.h>
   #include <stdarg.h>
   #include <string.h>
   #include <assert.h>
      #include "main/context.h"
   #include "main/debug_output.h"
   #include "main/formats.h"
   #include "main/shaderobj.h"
   #include "util/u_atomic.h" /* for p_atomic_cmpxchg */
   #include "util/ralloc.h"
   #include "util/disk_cache.h"
   #include "util/mesa-sha1.h"
   #include "ast.h"
   #include "glsl_parser_extras.h"
   #include "glsl_parser.h"
   #include "ir_optimization.h"
   #include "builtin_functions.h"
      /**
   * Format a short human-readable description of the given GLSL version.
   */
   const char *
   glsl_compute_version_string(void *mem_ctx, bool is_es, unsigned version)
   {
      return ralloc_asprintf(mem_ctx, "GLSL%s %d.%02d", is_es ? " ES" : "",
      }
         static const unsigned known_desktop_glsl_versions[] =
         static const unsigned known_desktop_gl_versions[] =
               _mesa_glsl_parse_state::_mesa_glsl_parse_state(struct gl_context *_ctx,
                  : ctx(_ctx), exts(&_ctx->Extensions), consts(&_ctx->Const),
   api(_ctx->API), cs_input_local_size_specified(false), cs_input_local_size(),
      {
      assert(stage < MESA_SHADER_STAGES);
            this->scanner = NULL;
   this->translation_unit.make_empty();
                     this->info_log = ralloc_strdup(mem_ctx, "");
   this->error = false;
                     /* Set default language version and extensions */
   this->language_version = 110;
   this->forced_language_version = ctx->Const.ForceGLSLVersion;
   if (ctx->Const.GLSLZeroInit == 1) {
         } else if (ctx->Const.GLSLZeroInit == 2) {
         } else {
         }
   this->gl_version = 20;
   this->compat_shader = true;
   this->es_shader = false;
            /* OpenGL ES 2.0 has different defaults from desktop GL. */
   if (_mesa_is_gles2(ctx)) {
      this->language_version = 100;
   this->es_shader = true;
                        this->Const.MaxLights = ctx->Const.MaxLights;
   this->Const.MaxClipPlanes = ctx->Const.MaxClipPlanes;
   this->Const.MaxTextureUnits = ctx->Const.MaxTextureUnits;
   this->Const.MaxTextureCoords = ctx->Const.MaxTextureCoordUnits;
   this->Const.MaxVertexAttribs = ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs;
   this->Const.MaxVertexUniformComponents = ctx->Const.Program[MESA_SHADER_VERTEX].MaxUniformComponents;
   this->Const.MaxVertexTextureImageUnits = ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits;
   this->Const.MaxCombinedTextureImageUnits = ctx->Const.MaxCombinedTextureImageUnits;
   this->Const.MaxTextureImageUnits = ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits;
   this->Const.MaxFragmentUniformComponents = ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxUniformComponents;
   this->Const.MinProgramTexelOffset = ctx->Const.MinProgramTexelOffset;
                              /* 1.50 constants */
   this->Const.MaxVertexOutputComponents = ctx->Const.Program[MESA_SHADER_VERTEX].MaxOutputComponents;
   this->Const.MaxGeometryInputComponents = ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxInputComponents;
   this->Const.MaxGeometryOutputComponents = ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxOutputComponents;
   this->Const.MaxGeometryShaderInvocations = ctx->Const.MaxGeometryShaderInvocations;
   this->Const.MaxFragmentInputComponents = ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxInputComponents;
   this->Const.MaxGeometryTextureImageUnits = ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits;
   this->Const.MaxGeometryOutputVertices = ctx->Const.MaxGeometryOutputVertices;
   this->Const.MaxGeometryTotalOutputComponents = ctx->Const.MaxGeometryTotalOutputComponents;
            this->Const.MaxVertexAtomicCounters = ctx->Const.Program[MESA_SHADER_VERTEX].MaxAtomicCounters;
   this->Const.MaxTessControlAtomicCounters = ctx->Const.Program[MESA_SHADER_TESS_CTRL].MaxAtomicCounters;
   this->Const.MaxTessEvaluationAtomicCounters = ctx->Const.Program[MESA_SHADER_TESS_EVAL].MaxAtomicCounters;
   this->Const.MaxGeometryAtomicCounters = ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxAtomicCounters;
   this->Const.MaxFragmentAtomicCounters = ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxAtomicCounters;
   this->Const.MaxComputeAtomicCounters = ctx->Const.Program[MESA_SHADER_COMPUTE].MaxAtomicCounters;
   this->Const.MaxCombinedAtomicCounters = ctx->Const.MaxCombinedAtomicCounters;
   this->Const.MaxAtomicBufferBindings = ctx->Const.MaxAtomicBufferBindings;
   this->Const.MaxVertexAtomicCounterBuffers =
         this->Const.MaxTessControlAtomicCounterBuffers =
         this->Const.MaxTessEvaluationAtomicCounterBuffers =
         this->Const.MaxGeometryAtomicCounterBuffers =
         this->Const.MaxFragmentAtomicCounterBuffers =
         this->Const.MaxComputeAtomicCounterBuffers =
         this->Const.MaxCombinedAtomicCounterBuffers =
         this->Const.MaxAtomicCounterBufferSize =
            /* ARB_enhanced_layouts constants */
   this->Const.MaxTransformFeedbackBuffers = ctx->Const.MaxTransformFeedbackBuffers;
            /* Compute shader constants */
   for (unsigned i = 0; i < ARRAY_SIZE(this->Const.MaxComputeWorkGroupCount); i++)
         for (unsigned i = 0; i < ARRAY_SIZE(this->Const.MaxComputeWorkGroupSize); i++)
            this->Const.MaxComputeTextureImageUnits = ctx->Const.Program[MESA_SHADER_COMPUTE].MaxTextureImageUnits;
            this->Const.MaxImageUnits = ctx->Const.MaxImageUnits;
   this->Const.MaxCombinedShaderOutputResources = ctx->Const.MaxCombinedShaderOutputResources;
   this->Const.MaxImageSamples = ctx->Const.MaxImageSamples;
   this->Const.MaxVertexImageUniforms = ctx->Const.Program[MESA_SHADER_VERTEX].MaxImageUniforms;
   this->Const.MaxTessControlImageUniforms = ctx->Const.Program[MESA_SHADER_TESS_CTRL].MaxImageUniforms;
   this->Const.MaxTessEvaluationImageUniforms = ctx->Const.Program[MESA_SHADER_TESS_EVAL].MaxImageUniforms;
   this->Const.MaxGeometryImageUniforms = ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxImageUniforms;
   this->Const.MaxFragmentImageUniforms = ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxImageUniforms;
   this->Const.MaxComputeImageUniforms = ctx->Const.Program[MESA_SHADER_COMPUTE].MaxImageUniforms;
            /* ARB_viewport_array */
            /* tessellation shader constants */
   this->Const.MaxPatchVertices = ctx->Const.MaxPatchVertices;
   this->Const.MaxTessGenLevel = ctx->Const.MaxTessGenLevel;
   this->Const.MaxTessControlInputComponents = ctx->Const.Program[MESA_SHADER_TESS_CTRL].MaxInputComponents;
   this->Const.MaxTessControlOutputComponents = ctx->Const.Program[MESA_SHADER_TESS_CTRL].MaxOutputComponents;
   this->Const.MaxTessControlTextureImageUnits = ctx->Const.Program[MESA_SHADER_TESS_CTRL].MaxTextureImageUnits;
   this->Const.MaxTessEvaluationInputComponents = ctx->Const.Program[MESA_SHADER_TESS_EVAL].MaxInputComponents;
   this->Const.MaxTessEvaluationOutputComponents = ctx->Const.Program[MESA_SHADER_TESS_EVAL].MaxOutputComponents;
   this->Const.MaxTessEvaluationTextureImageUnits = ctx->Const.Program[MESA_SHADER_TESS_EVAL].MaxTextureImageUnits;
   this->Const.MaxTessPatchComponents = ctx->Const.MaxTessPatchComponents;
   this->Const.MaxTessControlTotalOutputComponents = ctx->Const.MaxTessControlTotalOutputComponents;
   this->Const.MaxTessControlUniformComponents = ctx->Const.Program[MESA_SHADER_TESS_CTRL].MaxUniformComponents;
            /* GL 4.5 / OES_sample_variables */
            this->current_function = NULL;
   this->toplevel_ir = NULL;
   this->found_return = false;
   this->found_begin_interlock = false;
   this->found_end_interlock = false;
   this->all_invariant = false;
   this->user_structures = NULL;
   this->num_user_structures = 0;
   this->num_subroutines = 0;
   this->subroutines = NULL;
   this->num_subroutine_types = 0;
            /* supported_versions should be large enough to support the known desktop
   * GLSL versions plus 4 GLES versions (ES 1.00, ES 3.00, ES 3.10, ES 3.20)
   */
   STATIC_ASSERT((ARRAY_SIZE(known_desktop_glsl_versions) + 4) ==
            /* Populate the list of supported GLSL versions */
   /* FINISHME: Once the OpenGL 3.0 'forward compatible' context or
   * the OpenGL 3.2 Core context is supported, this logic will need
   * change.  Older versions of GLSL are no longer supported
   * outside the compatibility contexts of 3.x.
   */
   this->num_supported_versions = 0;
   if (_mesa_is_desktop_gl(ctx)) {
      for (unsigned i = 0; i < ARRAY_SIZE(known_desktop_glsl_versions); i++) {
      if (known_desktop_glsl_versions[i] <= ctx->Const.GLSLVersion) {
      this->supported_versions[this->num_supported_versions].ver
         this->supported_versions[this->num_supported_versions].gl_ver
         this->supported_versions[this->num_supported_versions].es = false;
            }
   if (_mesa_is_gles2(ctx) || ctx->Extensions.ARB_ES2_compatibility) {
      this->supported_versions[this->num_supported_versions].ver = 100;
   this->supported_versions[this->num_supported_versions].gl_ver = 20;
   this->supported_versions[this->num_supported_versions].es = true;
      }
   if (_mesa_is_gles3(ctx) || ctx->Extensions.ARB_ES3_compatibility) {
      this->supported_versions[this->num_supported_versions].ver = 300;
   this->supported_versions[this->num_supported_versions].gl_ver = 30;
   this->supported_versions[this->num_supported_versions].es = true;
      }
   if (_mesa_is_gles31(ctx) || ctx->Extensions.ARB_ES3_1_compatibility) {
      this->supported_versions[this->num_supported_versions].ver = 310;
   this->supported_versions[this->num_supported_versions].gl_ver = 31;
   this->supported_versions[this->num_supported_versions].es = true;
      }
   if (_mesa_is_gles32(ctx) ||
      ctx->Extensions.ARB_ES3_2_compatibility) {
   this->supported_versions[this->num_supported_versions].ver = 320;
   this->supported_versions[this->num_supported_versions].gl_ver = 32;
   this->supported_versions[this->num_supported_versions].es = true;
               /* Create a string for use in error messages to tell the user which GLSL
   * versions are supported.
   */
   char *supported = ralloc_strdup(this, "");
   for (unsigned i = 0; i < this->num_supported_versions; i++) {
      unsigned ver = this->supported_versions[i].ver;
   ? ""
   : ((i == this->num_supported_versions - 1) ? ", and " : ", ");
                  ralloc_asprintf_append(& supported, "%s%u.%02u%s",
   prefix,
   ver / 100, ver % 100,
                        if (ctx->Const.ForceGLSLExtensionsWarn)
            this->default_uniform_qualifier = new(this) ast_type_qualifier();
   this->default_uniform_qualifier->flags.q.shared = 1;
            this->default_shader_storage_qualifier = new(this) ast_type_qualifier();
   this->default_shader_storage_qualifier->flags.q.shared = 1;
            this->fs_uses_gl_fragcoord = false;
   this->fs_redeclares_gl_fragcoord = false;
   this->fs_origin_upper_left = false;
   this->fs_pixel_center_integer = false;
            this->gs_input_prim_type_specified = false;
   this->tcs_output_vertices_specified = false;
   this->gs_input_size = 0;
   this->in_qualifier = new(this) ast_type_qualifier();
   this->out_qualifier = new(this) ast_type_qualifier();
   this->fs_early_fragment_tests = false;
   this->fs_inner_coverage = false;
   this->fs_post_depth_coverage = false;
   this->fs_pixel_interlock_ordered = false;
   this->fs_pixel_interlock_unordered = false;
   this->fs_sample_interlock_ordered = false;
   this->fs_sample_interlock_unordered = false;
   this->fs_blend_support = 0;
   memset(this->atomic_counter_offsets, 0,
         this->allow_extension_directive_midshader =
         this->alias_shader_extension =
         this->allow_vertex_texture_bias =
         this->allow_glsl_120_subset_in_110 =
         this->allow_builtin_variable_redeclaration =
         this->ignore_write_to_readonly_var =
                     /* ARB_bindless_texture */
   this->bindless_sampler_specified = false;
   this->bindless_image_specified = false;
   this->bound_sampler_specified = false;
            this->language_version = this->forced_language_version ?
            }
      /**
   * Determine whether the current GLSL version is sufficiently high to support
   * a certain feature, and generate an error message if it isn't.
   *
   * \param required_glsl_version and \c required_glsl_es_version are
   * interpreted as they are in _mesa_glsl_parse_state::is_version().
   *
   * \param locp is the parser location where the error should be reported.
   *
   * \param fmt (and additional arguments) constitute a printf-style error
   * message to report if the version check fails.  Information about the
   * current and required GLSL versions will be appended.  So, for example, if
   * the GLSL version being compiled is 1.20, and check_version(130, 300, locp,
   * "foo unsupported") is called, the error message will be "foo unsupported in
   * GLSL 1.20 (GLSL 1.30 or GLSL 3.00 ES required)".
   */
   bool
   _mesa_glsl_parse_state::check_version(unsigned required_glsl_version,
               {
      if (this->is_version(required_glsl_version, required_glsl_es_version))
            va_list args;
   va_start(args, fmt);
   char *problem = ralloc_vasprintf(this, fmt, args);
   va_end(args);
   const char *glsl_version_string
         const char *glsl_es_version_string
         const char *requirement_string = "";
   if (required_glsl_version && required_glsl_es_version) {
      requirement_string = ralloc_asprintf(this, " (%s or %s required)",
            } else if (required_glsl_version) {
      requirement_string = ralloc_asprintf(this, " (%s required)",
      } else if (required_glsl_es_version) {
      requirement_string = ralloc_asprintf(this, " (%s required)",
      }
   _mesa_glsl_error(locp, this, "%s in %s%s",
                     }
      /**
   * This makes sure any GLSL versions defined or overridden are valid. If not it
   * sets a valid value.
   */
   void
   _mesa_glsl_parse_state::set_valid_gl_and_glsl_versions(YYLTYPE *locp)
   {
      bool supported = false;
   for (unsigned i = 0; i < this->num_supported_versions; i++) {
      if (this->supported_versions[i].ver == this->language_version
      && this->supported_versions[i].es == this->es_shader) {
   this->gl_version = this->supported_versions[i].gl_ver;
   supported = true;
                  if (!supported) {
      if (locp) {
      _mesa_glsl_error(locp, this, "%s is not supported. "
                           /* On exit, the language_version must be set to a valid value.
   * Later calls to _mesa_glsl_initialize_types will misbehave if
   * the version is invalid.
   */
   switch (this->api) {
   case API_OPENGL_COMPAT:
   this->language_version = this->consts->GLSLVersion;
   break;
            FALLTHROUGH;
            this->language_version = 100;
   break;
               }
      /**
   * Process a GLSL #version directive.
   *
   * \param version is the integer that follows the #version token.
   *
   * \param ident is a string identifier that follows the integer, if any is
   * present.  Otherwise NULL.
   */
   void
   _mesa_glsl_parse_state::process_version_directive(YYLTYPE *locp, int version,
         {
      bool es_token_present = false;
   bool compat_token_present = false;
   if (ident) {
      if (strcmp(ident, "es") == 0) {
         } else if (version >= 150) {
      if (strcmp(ident, "core") == 0) {
      /* Accept the token.  There's no need to record that this is
   * a core profile shader since that's the only profile we support.
                        if (this->api != API_OPENGL_COMPAT &&
      !this->consts->AllowGLSLCompatShaders) {
   _mesa_glsl_error(locp, this,
         } else {
      _mesa_glsl_error(locp, this,
               } else {
      _mesa_glsl_error(locp, this,
                  this->es_shader = es_token_present;
   if (version == 100) {
      if (es_token_present) {
      _mesa_glsl_error(locp, this,
            } else {
                     if (this->es_shader) {
                  if (this->forced_language_version)
         else
            this->compat_shader = compat_token_present ||
                                 }
         /* This helper function will append the given message to the shader's
      info log and report it via GL_ARB_debug_output. Per that extension,
   'type' is one of the enum values classifying the message, and
      static void
   _mesa_glsl_msg(const YYLTYPE *locp, _mesa_glsl_parse_state *state,
         {
      bool error = (type == MESA_DEBUG_TYPE_ERROR);
                     /* Get the offset that the new message will be written to. */
            if (locp->path) {
         } else {
         }
   ralloc_asprintf_append(&state->info_log, ":%u(%u): %s: ",
                           const char *const msg = &state->info_log[msg_offset];
            /* Report the error via GL_ARB_debug_output. */
               }
      void
   _mesa_glsl_error(YYLTYPE *locp, _mesa_glsl_parse_state *state,
         {
                        va_start(ap, fmt);
   _mesa_glsl_msg(locp, state, MESA_DEBUG_TYPE_ERROR, fmt, ap);
      }
         void
   _mesa_glsl_warning(const YYLTYPE *locp, _mesa_glsl_parse_state *state,
         {
      if (state->warnings_enabled) {
               va_start(ap, fmt);
   _mesa_glsl_msg(locp, state, MESA_DEBUG_TYPE_OTHER, fmt, ap);
         }
         /**
   * Enum representing the possible behaviors that can be specified in
   * an #extension directive.
   */
   enum ext_behavior {
      extension_disable,
   extension_enable,
   extension_require,
      };
      /**
   * Element type for _mesa_glsl_supported_extensions
   */
   struct _mesa_glsl_extension {
      /**
   * Name of the extension when referred to in a GLSL extension
   * statement
   */
            /**
   * Whether this extension is a part of AEP
   */
            /**
   * Predicate that checks whether the relevant extension is available for
   * this context.
   */
   bool (*available_pred)(const struct gl_extensions *,
            /**
   * Flag in the _mesa_glsl_parse_state struct that should be set
   * when this extension is enabled.
   *
   * See note in _mesa_glsl_extension::supported_flag about "pointer
   * to member" types.
   */
            /**
   * Flag in the _mesa_glsl_parse_state struct that should be set
   * when the shader requests "warn" behavior for this extension.
   *
   * See note in _mesa_glsl_extension::supported_flag about "pointer
   * to member" types.
   */
               bool compatible_with_state(const _mesa_glsl_parse_state *state,
            };
      /** Checks if the context supports a user-facing extension */
   #define EXT(name_str, driver_cap, ...) \
   static UNUSED bool \
   has_##name_str(const struct gl_extensions *exts, gl_api api, uint8_t version) \
   { \
      return exts->driver_cap && (version >= \
      }
   #include "main/extensions_table.h"
   #undef EXT
      #define EXT(NAME)                                           \
      { "GL_" #NAME, false, has_##NAME,                        \
   &_mesa_glsl_parse_state::NAME##_enable,                \
         #define EXT_AEP(NAME)                                       \
      { "GL_" #NAME, true, has_##NAME,                         \
   &_mesa_glsl_parse_state::NAME##_enable,                \
         /**
   * Table of extensions that can be enabled/disabled within a shader,
   * and the conditions under which they are supported.
   */
   static const _mesa_glsl_extension _mesa_glsl_supported_extensions[] = {
      /* ARB extensions go here, sorted alphabetically.
   */
   EXT(ARB_ES3_1_compatibility),
   EXT(ARB_ES3_2_compatibility),
   EXT(ARB_arrays_of_arrays),
   EXT(ARB_bindless_texture),
   EXT(ARB_compatibility),
   EXT(ARB_compute_shader),
   EXT(ARB_compute_variable_group_size),
   EXT(ARB_conservative_depth),
   EXT(ARB_cull_distance),
   EXT(ARB_derivative_control),
   EXT(ARB_draw_buffers),
   EXT(ARB_draw_instanced),
   EXT(ARB_enhanced_layouts),
   EXT(ARB_explicit_attrib_location),
   EXT(ARB_explicit_uniform_location),
   EXT(ARB_fragment_coord_conventions),
   EXT(ARB_fragment_layer_viewport),
   EXT(ARB_fragment_shader_interlock),
   EXT(ARB_gpu_shader5),
   EXT(ARB_gpu_shader_fp64),
   EXT(ARB_gpu_shader_int64),
   EXT(ARB_post_depth_coverage),
   EXT(ARB_sample_shading),
   EXT(ARB_separate_shader_objects),
   EXT(ARB_shader_atomic_counter_ops),
   EXT(ARB_shader_atomic_counters),
   EXT(ARB_shader_ballot),
   EXT(ARB_shader_bit_encoding),
   EXT(ARB_shader_clock),
   EXT(ARB_shader_draw_parameters),
   EXT(ARB_shader_group_vote),
   EXT(ARB_shader_image_load_store),
   EXT(ARB_shader_image_size),
   EXT(ARB_shader_precision),
   EXT(ARB_shader_stencil_export),
   EXT(ARB_shader_storage_buffer_object),
   EXT(ARB_shader_subroutine),
   EXT(ARB_shader_texture_image_samples),
   EXT(ARB_shader_texture_lod),
   EXT(ARB_shader_viewport_layer_array),
   EXT(ARB_shading_language_420pack),
   EXT(ARB_shading_language_include),
   EXT(ARB_shading_language_packing),
   EXT(ARB_sparse_texture2),
   EXT(ARB_sparse_texture_clamp),
   EXT(ARB_tessellation_shader),
   EXT(ARB_texture_cube_map_array),
   EXT(ARB_texture_gather),
   EXT(ARB_texture_multisample),
   EXT(ARB_texture_query_levels),
   EXT(ARB_texture_query_lod),
   EXT(ARB_texture_rectangle),
   EXT(ARB_uniform_buffer_object),
   EXT(ARB_vertex_attrib_64bit),
            /* KHR extensions go here, sorted alphabetically.
   */
            /* OES extensions go here, sorted alphabetically.
   */
   EXT(OES_EGL_image_external),
   EXT(OES_EGL_image_external_essl3),
   EXT(OES_geometry_point_size),
   EXT(OES_geometry_shader),
   EXT(OES_gpu_shader5),
   EXT(OES_primitive_bounding_box),
   EXT_AEP(OES_sample_variables),
   EXT_AEP(OES_shader_image_atomic),
   EXT(OES_shader_io_blocks),
   EXT_AEP(OES_shader_multisample_interpolation),
   EXT(OES_standard_derivatives),
   EXT(OES_tessellation_point_size),
   EXT(OES_tessellation_shader),
   EXT(OES_texture_3D),
   EXT(OES_texture_buffer),
   EXT(OES_texture_cube_map_array),
   EXT_AEP(OES_texture_storage_multisample_2d_array),
            /* All other extensions go here, sorted alphabetically.
   */
   EXT(AMD_conservative_depth),
   EXT(AMD_gpu_shader_int64),
   EXT(AMD_shader_stencil_export),
   EXT(AMD_shader_trinary_minmax),
   EXT(AMD_texture_texture4),
   EXT(AMD_vertex_shader_layer),
   EXT(AMD_vertex_shader_viewport_index),
   EXT(ANDROID_extension_pack_es31a),
   EXT(ARM_shader_framebuffer_fetch_depth_stencil),
   EXT(EXT_blend_func_extended),
   EXT(EXT_demote_to_helper_invocation),
   EXT(EXT_frag_depth),
   EXT(EXT_draw_buffers),
   EXT(EXT_draw_instanced),
   EXT(EXT_clip_cull_distance),
   EXT(EXT_geometry_point_size),
   EXT_AEP(EXT_geometry_shader),
   EXT(EXT_gpu_shader4),
   EXT_AEP(EXT_gpu_shader5),
   EXT_AEP(EXT_primitive_bounding_box),
   EXT(EXT_separate_shader_objects),
   EXT(EXT_shader_framebuffer_fetch),
   EXT(EXT_shader_framebuffer_fetch_non_coherent),
   EXT(EXT_shader_group_vote),
   EXT(EXT_shader_image_load_formatted),
   EXT(EXT_shader_image_load_store),
   EXT(EXT_shader_implicit_conversions),
   EXT(EXT_shader_integer_mix),
   EXT_AEP(EXT_shader_io_blocks),
   EXT(EXT_shader_samples_identical),
   EXT(EXT_tessellation_point_size),
   EXT_AEP(EXT_tessellation_shader),
   EXT(EXT_texture_array),
   EXT_AEP(EXT_texture_buffer),
   EXT_AEP(EXT_texture_cube_map_array),
   EXT(EXT_texture_query_lod),
   EXT(EXT_texture_shadow_lod),
   EXT(INTEL_conservative_rasterization),
   EXT(INTEL_shader_atomic_float_minmax),
   EXT(INTEL_shader_integer_functions2),
   EXT(MESA_shader_integer_functions),
   EXT(NV_compute_shader_derivatives),
   EXT(NV_fragment_shader_interlock),
   EXT(NV_image_formats),
   EXT(NV_shader_atomic_float),
   EXT(NV_shader_atomic_int64),
   EXT(NV_shader_noperspective_interpolation),
      };
      #undef EXT
         /**
   * Determine whether a given extension is compatible with the target,
   * API, and extension information in the current parser state.
   */
   bool _mesa_glsl_extension::compatible_with_state(
         {
         }
      /**
   * Set the appropriate flags in the parser state to establish the
   * given behavior for this extension.
   */
   void _mesa_glsl_extension::set_flags(_mesa_glsl_parse_state *state,
         {
      /* Note: the ->* operator indexes into state by the
   * offsets this->enable_flag and this->warn_flag.  See
   * _mesa_glsl_extension::supported_flag for more info.
   */
   state->*(this->enable_flag) = (behavior != extension_disable);
      }
      /**
   * Check alias_shader_extension for any aliased shader extensions
   */
   static const char *find_extension_alias(_mesa_glsl_parse_state *state, const char *name)
   {
               /* Copy alias_shader_extension because strtok() is destructive. */
   exts = strdup(state->alias_shader_extension);
   if (exts) {
      for (field = strtok(exts, ","); field != NULL; field = strtok(NULL, ",")) {
      if(strncmp(name, field, strlen(name)) == 0) {
      field = strstr(field, ":");
   if(field) {
         }
         }
         }
      }
      /**
   * Find an extension by name in _mesa_glsl_supported_extensions.  If
   * the name is not found, return NULL.
   */
   static const _mesa_glsl_extension *find_extension(_mesa_glsl_parse_state *state, const char *name)
   {
      const char *ext_alias = NULL;
   if (state->alias_shader_extension) {
      ext_alias = find_extension_alias(state, name);
      }
      for (unsigned i = 0; i < ARRAY_SIZE(_mesa_glsl_supported_extensions); ++i) {
      if (strcmp(name, _mesa_glsl_supported_extensions[i].name) == 0) {
      free((void *)ext_alias);
                  free((void *)ext_alias);
      }
      bool
   _mesa_glsl_process_extension(const char *name, YYLTYPE *name_locp,
         const char *behavior_string, YYLTYPE *behavior_locp,
   {
      uint8_t gl_version = state->exts->Version;
   gl_api api = state->api;
   ext_behavior behavior;
   if (strcmp(behavior_string, "warn") == 0) {
         } else if (strcmp(behavior_string, "require") == 0) {
         } else if (strcmp(behavior_string, "enable") == 0) {
         } else if (strcmp(behavior_string, "disable") == 0) {
         } else {
      _mesa_glsl_error(behavior_locp, state,
      "unknown extension behavior `%s'",
                  /* If we're in a desktop context but with an ES shader, use an ES API enum
   * to verify extension availability.
   */
   if (state->es_shader && api != API_OPENGLES2)
         /* Use the language-version derived GL version to extension checks, unless
   * we're using meta, which sets the version to the max.
   */
   if (gl_version != 0xff)
            if (strcmp(name, "all") == 0) {
      _mesa_glsl_error(name_locp, state, "cannot %s all extensions",
      (behavior == extension_enable)
      return false;
         } else {
      for (unsigned i = 0;
      i < ARRAY_SIZE(_mesa_glsl_supported_extensions); ++i) {
   const _mesa_glsl_extension *extension
         if (extension->compatible_with_state(state, api, gl_version)) {
                  } else {
      const _mesa_glsl_extension *extension = find_extension(state, name);
   if (extension &&
      (extension->compatible_with_state(state, api, gl_version) ||
   (state->consts->AllowGLSLCompatShaders &&
         extension->set_flags(state, behavior);
   if (extension->available_pred == has_ANDROID_extension_pack_es31a) {
      for (unsigned i = 0;
                           if (!extension->aep)
         /* AEP should not be enabled if all of the sub-extensions can't
   * also be enabled. This is not the proper layer to do such
   * error-checking though.
   */
   assert(extension->compatible_with_state(state, api, gl_version));
            } else {
               if (behavior == extension_require) {
      _mesa_glsl_error(name_locp, state, fmt,
            } else {
      _mesa_glsl_warning(name_locp, state, fmt,
                        }
      bool
   _mesa_glsl_can_implicitly_convert(const glsl_type *from, const glsl_type *desired,
         {
      if (from == desired)
            /* GLSL 1.10 and ESSL do not allow implicit conversions. If there is no
   * state, we're doing intra-stage function linking where these checks have
   * already been done.
   */
   if (state && !state->has_implicit_conversions())
            /* There is no conversion among matrix types. */
   if (from->matrix_columns > 1 || desired->matrix_columns > 1)
            /* Vector size must match. */
   if (from->vector_elements != desired->vector_elements)
            /* int and uint can be converted to float. */
   if (desired->is_float() && from->is_integer_32())
            /* With GLSL 4.0, ARB_gpu_shader5, or MESA_shader_integer_functions, int
   * can be converted to uint.  Note that state may be NULL here, when
   * resolving function calls in the linker. By this time, all the
   * state-dependent checks have already happened though, so allow anything
   * that's allowed in any shader version.
   */
   if ((!state || state->has_implicit_int_to_uint_conversion()) &&
                  /* No implicit conversions from double. */
   if ((!state || state->has_double()) && from->is_double())
            /* Conversions from different types to double. */
   if ((!state || state->has_double()) && desired->is_double()) {
      if (from->is_float())
         if (from->is_integer_32())
                  }
      /**
   * Recurses through <type> and <expr> if <expr> is an aggregate initializer
   * and sets <expr>'s <constructor_type> field to <type>. Gives later functions
   * (process_array_constructor, et al) sufficient information to do type
   * checking.
   *
   * Operates on assignments involving an aggregate initializer. E.g.,
   *
   * vec4 pos = {1.0, -1.0, 0.0, 1.0};
   *
   * or more ridiculously,
   *
   * struct S {
   *     vec4 v[2];
   * };
   *
   * struct {
   *     S a[2], b;
   *     int c;
   * } aggregate = {
   *     {
   *         {
   *             {
   *                 {1.0, 2.0, 3.0, 4.0}, // a[0].v[0]
   *                 {5.0, 6.0, 7.0, 8.0}  // a[0].v[1]
   *             } // a[0].v
   *         }, // a[0]
   *         {
   *             {
   *                 {1.0, 2.0, 3.0, 4.0}, // a[1].v[0]
   *                 {5.0, 6.0, 7.0, 8.0}  // a[1].v[1]
   *             } // a[1].v
   *         } // a[1]
   *     }, // a
   *     {
   *         {
   *             {1.0, 2.0, 3.0, 4.0}, // b.v[0]
   *             {5.0, 6.0, 7.0, 8.0}  // b.v[1]
   *         } // b.v
   *     }, // b
   *     4 // c
   * };
   *
   * This pass is necessary because the right-hand side of <type> e = { ... }
   * doesn't contain sufficient information to determine if the types match.
   */
   void
   _mesa_ast_set_aggregate_type(const glsl_type *type,
         {
      ast_aggregate_initializer *ai = (ast_aggregate_initializer *)expr;
            /* If the aggregate is an array, recursively set its elements' types. */
   if (type->is_array()) {
      /* Each array element has the type type->fields.array.
   *
   * E.g., if <type> if struct S[2] we want to set each element's type to
   * struct S.
   */
   for (exec_node *expr_node = ai->expressions.get_head_raw();
      !expr_node->is_tail_sentinel();
   expr_node = expr_node->next) {
                  if (expr->oper == ast_aggregate)
            /* If the aggregate is a struct, recursively set its fields' types. */
   } else if (type->is_struct()) {
               /* Iterate through the struct's fields. */
   for (unsigned i = 0; !expr_node->is_tail_sentinel() && i < type->length;
      i++, expr_node = expr_node->next) {
                  if (expr->oper == ast_aggregate) {
               /* If the aggregate is a matrix, set its columns' types. */
   } else if (type->is_matrix()) {
      for (exec_node *expr_node = ai->expressions.get_head_raw();
      !expr_node->is_tail_sentinel();
   expr_node = expr_node->next) {
                  if (expr->oper == ast_aggregate)
            }
      void
   _mesa_ast_process_interface_block(YYLTYPE *locp,
                     {
      if (q.flags.q.buffer) {
      if (!state->has_shader_storage_buffer_objects()) {
      _mesa_glsl_error(locp, state,
            } else if (state->ARB_shader_storage_buffer_object_warn) {
      _mesa_glsl_warning(locp, state,
               } else if (q.flags.q.uniform) {
      if (!state->has_uniform_buffer_objects()) {
      _mesa_glsl_error(locp, state,
            } else if (state->ARB_uniform_buffer_object_warn) {
      _mesa_glsl_warning(locp, state,
               } else {
      if (!state->has_shader_io_blocks()) {
      if (state->es_shader) {
      _mesa_glsl_error(locp, state,
            } else {
      _mesa_glsl_error(locp, state,
                           /* From the GLSL 1.50.11 spec, section 4.3.7 ("Interface Blocks"):
   * "It is illegal to have an input block in a vertex shader
   *  or an output block in a fragment shader"
   */
   if ((state->stage == MESA_SHADER_VERTEX) && q.flags.q.in) {
      _mesa_glsl_error(locp, state,
            } else if ((state->stage == MESA_SHADER_FRAGMENT) && q.flags.q.out) {
      _mesa_glsl_error(locp, state,
                     /* Since block arrays require names, and both features are added in
   * the same language versions, we don't have to explicitly
   * version-check both things.
   */
   if (block->instance_name != NULL) {
      state->check_version(150, 300, locp, "interface blocks with "
               ast_type_qualifier::bitset_t interface_type_mask;
            /* Get a bitmask containing only the in/out/uniform/buffer
   * flags, allowing us to ignore other irrelevant flags like
   * interpolation qualifiers.
   */
   temp_type_qualifier.flags.i = 0;
   temp_type_qualifier.flags.q.uniform = true;
   temp_type_qualifier.flags.q.in = true;
   temp_type_qualifier.flags.q.out = true;
   temp_type_qualifier.flags.q.buffer = true;
   temp_type_qualifier.flags.q.patch = true;
            /* Get the block's interface qualifier.  The interface_qualifier
   * production rule guarantees that only one bit will be set (and
   * it will be in/out/uniform).
   */
                     if (state->stage == MESA_SHADER_GEOMETRY &&
      state->has_explicit_attrib_stream() &&
   block->default_layout.flags.q.out) {
   /* Assign global layout's stream value. */
   block->default_layout.flags.q.stream = 1;
   block->default_layout.flags.q.explicit_stream = 0;
               if (state->has_enhanced_layouts() && block->default_layout.flags.q.out &&
      state->exts->ARB_transform_feedback3) {
   /* Assign global layout's xfb_buffer value. */
   block->default_layout.flags.q.xfb_buffer = 1;
   block->default_layout.flags.q.explicit_xfb_buffer = 0;
               foreach_list_typed (ast_declarator_list, member, link, &block->declarations) {
      ast_type_qualifier& qualifier = member->type->qualifier;
   if ((qualifier.flags.i & interface_type_mask) == 0) {
      /* GLSLangSpec.1.50.11, 4.3.7 (Interface Blocks):
   * "If no optional qualifier is used in a member declaration, the
   *  qualifier of the variable is just in, out, or uniform as declared
   *  by interface-qualifier."
   */
      } else if ((qualifier.flags.i & interface_type_mask) !=
            /* GLSLangSpec.1.50.11, 4.3.7 (Interface Blocks):
   * "If optional qualifiers are used, they can include interpolation
   *  and storage qualifiers and they must declare an input, output,
   *  or uniform variable consistent with the interface qualifier of
   *  the block."
   */
   _mesa_glsl_error(locp, state,
                           if (!(q.flags.q.in || q.flags.q.out) && qualifier.flags.q.invariant)
      _mesa_glsl_error(locp, state,
                  }
      static void
   _mesa_ast_type_qualifier_print(const struct ast_type_qualifier *q)
   {
      if (q->is_subroutine_decl())
            if (q->subroutine_list) {
      printf("subroutine (");
   q->subroutine_list->print();
               if (q->flags.q.constant)
            if (q->flags.q.invariant)
            if (q->flags.q.attribute)
            if (q->flags.q.varying)
            if (q->flags.q.in && q->flags.q.out)
         else {
      printf("in ");
            printf("out ");
               if (q->flags.q.centroid)
         if (q->flags.q.sample)
         if (q->flags.q.patch)
         if (q->flags.q.uniform)
         if (q->flags.q.buffer)
         if (q->flags.q.smooth)
         if (q->flags.q.flat)
         if (q->flags.q.noperspective)
      }
         void
   ast_node::print(void) const
   {
         }
         ast_node::ast_node(void)
   {
      this->location.path = NULL;
   this->location.source = 0;
   this->location.first_line = 0;
   this->location.first_column = 0;
   this->location.last_line = 0;
      }
         static void
   ast_opt_array_dimensions_print(const ast_array_specifier *array_specifier)
   {
      if (array_specifier)
      }
         void
   ast_compound_statement::print(void) const
   {
               foreach_list_typed(ast_node, ast, link, &this->statements) {
                     }
         ast_compound_statement::ast_compound_statement(int new_scope,
         {
               if (statements != NULL) {
            }
         void
   ast_expression::print(void) const
   {
      switch (oper) {
   case ast_assign:
   case ast_mul_assign:
   case ast_div_assign:
   case ast_mod_assign:
   case ast_add_assign:
   case ast_sub_assign:
   case ast_ls_assign:
   case ast_rs_assign:
   case ast_and_assign:
   case ast_xor_assign:
   case ast_or_assign:
      subexpressions[0]->print();
   printf("%s ", operator_string(oper));
   subexpressions[1]->print();
         case ast_field_selection:
      subexpressions[0]->print();
   printf(". %s ", primary_expression.identifier);
         case ast_plus:
   case ast_neg:
   case ast_bit_not:
   case ast_logic_not:
   case ast_pre_inc:
   case ast_pre_dec:
      printf("%s ", operator_string(oper));
   subexpressions[0]->print();
         case ast_post_inc:
   case ast_post_dec:
      subexpressions[0]->print();
   printf("%s ", operator_string(oper));
         case ast_conditional:
      subexpressions[0]->print();
   printf("? ");
   subexpressions[1]->print();
   printf(": ");
   subexpressions[2]->print();
         case ast_array_index:
      subexpressions[0]->print();
   printf("[ ");
   subexpressions[1]->print();
   printf("] ");
         case ast_function_call: {
      subexpressions[0]->print();
            if (&ast->link != this->expressions.get_head())
            ast->print();
                  printf(") ");
               case ast_identifier:
      printf("%s ", primary_expression.identifier);
         case ast_int_constant:
      printf("%d ", primary_expression.int_constant);
         case ast_uint_constant:
      printf("%u ", primary_expression.uint_constant);
         case ast_float_constant:
      printf("%f ", primary_expression.float_constant);
         case ast_double_constant:
      printf("%f ", primary_expression.double_constant);
         case ast_int64_constant:
      printf("%" PRId64 " ", primary_expression.int64_constant);
         case ast_uint64_constant:
      printf("%" PRIu64 " ", primary_expression.uint64_constant);
         case ast_bool_constant:
      printf("%s ",
   primary_expression.bool_constant
   ? "true" : "false");
         case ast_sequence: {
      printf("( ");
   if (&ast->link != this->expressions.get_head())
            ast->print();
         }
   printf(") ");
               case ast_aggregate: {
      printf("{ ");
   if (&ast->link != this->expressions.get_head())
            ast->print();
         }
   printf("} ");
               default:
      assert(0);
         }
      ast_expression::ast_expression(int oper,
            ast_expression *ex0,
   ast_expression *ex1,
      {
      this->oper = ast_operators(oper);
   this->subexpressions[0] = ex0;
   this->subexpressions[1] = ex1;
   this->subexpressions[2] = ex2;
   this->non_lvalue_description = NULL;
      }
         void
   ast_expression_statement::print(void) const
   {
      if (expression)
               }
         ast_expression_statement::ast_expression_statement(ast_expression *ex) :
         {
         }
         void
   ast_function::print(void) const
   {
      return_type->print();
            foreach_list_typed(ast_node, ast, link, & this->parameters) {
                     }
         ast_function::ast_function(void)
      : return_type(NULL), identifier(NULL), is_definition(false),
      {
         }
         void
   ast_fully_specified_type::print(void) const
   {
      _mesa_ast_type_qualifier_print(& qualifier);
      }
         void
   ast_parameter_declarator::print(void) const
   {
      type->print();
   if (identifier)
            }
         void
   ast_function_definition::print(void) const
   {
      prototype->print();
      }
         void
   ast_declaration::print(void) const
   {
      printf("%s ", identifier);
            if (initializer) {
      printf("= ");
         }
         ast_declaration::ast_declaration(const char *identifier,
      ast_array_specifier *array_specifier,
      {
      this->identifier = identifier;
   this->array_specifier = array_specifier;
      }
         void
   ast_declarator_list::print(void) const
   {
               if (type)
         else if (invariant)
         else
            foreach_list_typed (ast_node, ast, link, & this->declarations) {
      printf(", ");
                           }
         ast_declarator_list::ast_declarator_list(ast_fully_specified_type *type)
   {
      this->type = type;
   this->invariant = false;
      }
      void
   ast_jump_statement::print(void) const
   {
      switch (mode) {
   case ast_continue:
      printf("continue; ");
      case ast_break:
      printf("break; ");
      case ast_return:
      printf("return ");
   opt_return_value->print();
            printf("; ");
      case ast_discard:
      printf("discard; ");
         }
         ast_jump_statement::ast_jump_statement(int mode, ast_expression *return_value)
         {
               if (mode == ast_return)
      }
         void
   ast_demote_statement::print(void) const
   {
         }
         void
   ast_selection_statement::print(void) const
   {
      printf("if ( ");
   condition->print();
                     if (else_statement) {
      printf("else ");
         }
         ast_selection_statement::ast_selection_statement(ast_expression *condition,
         ast_node *then_statement,
   {
      this->condition = condition;
   this->then_statement = then_statement;
      }
         void
   ast_switch_statement::print(void) const
   {
      printf("switch ( ");
   test_expression->print();
               }
         ast_switch_statement::ast_switch_statement(ast_expression *test_expression,
         {
      this->test_expression = test_expression;
   this->body = body;
      }
         void
   ast_switch_body::print(void) const
   {
      printf("{\n");
   if (stmts != NULL) {
         }
      }
         ast_switch_body::ast_switch_body(ast_case_statement_list *stmts)
   {
         }
         void ast_case_label::print(void) const
   {
      if (test_value != NULL) {
      printf("case ");
   test_value->print();
      } else {
            }
         ast_case_label::ast_case_label(ast_expression *test_value)
   {
         }
         void ast_case_label_list::print(void) const
   {
      foreach_list_typed(ast_node, ast, link, & this->labels) {
         }
      }
         ast_case_label_list::ast_case_label_list(void)
   {
   }
         void ast_case_statement::print(void) const
   {
      labels->print();
   foreach_list_typed(ast_node, ast, link, & this->stmts) {
      ast->print();
         }
         ast_case_statement::ast_case_statement(ast_case_label_list *labels)
   {
         }
         void ast_case_statement_list::print(void) const
   {
      foreach_list_typed(ast_node, ast, link, & this->cases) {
            }
         ast_case_statement_list::ast_case_statement_list(void)
   {
   }
         void
   ast_iteration_statement::print(void) const
   {
      switch (mode) {
   case ast_for:
      printf("for( ");
   init_statement->print();
                  condition->print();
                  rest_expression->print();
                  body->print();
         case ast_while:
      printf("while ( ");
   condition->print();
         printf(") ");
   body->print();
         case ast_do_while:
      printf("do ");
   body->print();
   printf("while ( ");
   condition->print();
         printf("); ");
         }
         ast_iteration_statement::ast_iteration_statement(int mode,
         ast_node *init,
   ast_node *condition,
   ast_expression *rest_expression,
   {
      this->mode = ast_iteration_modes(mode);
   this->init_statement = init;
   this->condition = condition;
   this->rest_expression = rest_expression;
      }
         void
   ast_struct_specifier::print(void) const
   {
      printf("struct %s { ", name);
   foreach_list_typed(ast_node, ast, link, &this->declarations) {
         }
      }
         ast_struct_specifier::ast_struct_specifier(const char *identifier,
            : name(identifier), layout(NULL), declarations(), is_declaration(true),
      {
         }
      void ast_subroutine_list::print(void) const
   {
      foreach_list_typed (ast_node, ast, link, & this->declarations) {
      if (&ast->link != this->declarations.get_head())
               }
      static void
   set_shader_inout_layout(struct gl_shader *shader,
         {
      /* Should have been prevented by the parser. */
   if (shader->Stage != MESA_SHADER_GEOMETRY &&
      shader->Stage != MESA_SHADER_TESS_EVAL &&
   shader->Stage != MESA_SHADER_COMPUTE) {
               if (shader->Stage != MESA_SHADER_COMPUTE) {
      /* Should have been prevented by the parser. */
   assert(!state->cs_input_local_size_specified);
   assert(!state->cs_input_local_size_variable_specified);
               if (shader->Stage != MESA_SHADER_FRAGMENT) {
      /* Should have been prevented by the parser. */
   assert(!state->fs_uses_gl_fragcoord);
   assert(!state->fs_redeclares_gl_fragcoord);
   assert(!state->fs_pixel_center_integer);
   assert(!state->fs_origin_upper_left);
   assert(!state->fs_early_fragment_tests);
   assert(!state->fs_inner_coverage);
   assert(!state->fs_post_depth_coverage);
   assert(!state->fs_pixel_interlock_ordered);
   assert(!state->fs_pixel_interlock_unordered);
   assert(!state->fs_sample_interlock_ordered);
               for (unsigned i = 0; i < MAX_FEEDBACK_BUFFERS; i++) {
      if (state->out_qualifier->out_xfb_stride[i]) {
      unsigned xfb_stride;
   if (state->out_qualifier->out_xfb_stride[i]->
         process_qualifier_constant(state, "xfb_stride", &xfb_stride,
                        switch (shader->Stage) {
   case MESA_SHADER_TESS_CTRL:
      shader->info.TessCtrl.VerticesOut = 0;
   if (state->tcs_output_vertices_specified) {
      unsigned vertices;
   if (state->out_qualifier->vertices->
                     YYLTYPE loc = state->out_qualifier->vertices->get_location();
   if (vertices > state->Const.MaxPatchVertices) {
      _mesa_glsl_error(&loc, state, "vertices (%d) exceeds "
      }
         }
      case MESA_SHADER_TESS_EVAL:
      shader->OES_tessellation_point_size_enable = state->OES_tessellation_point_size_enable || state->EXT_tessellation_point_size_enable;
   shader->info.TessEval._PrimitiveMode = TESS_PRIMITIVE_UNSPECIFIED;
   if (state->in_qualifier->flags.q.prim_type) {
      switch (state->in_qualifier->prim_type) {
   case GL_TRIANGLES:
      shader->info.TessEval._PrimitiveMode = TESS_PRIMITIVE_TRIANGLES;
      case GL_QUADS:
      shader->info.TessEval._PrimitiveMode = TESS_PRIMITIVE_QUADS;
      case GL_ISOLINES:
      shader->info.TessEval._PrimitiveMode = TESS_PRIMITIVE_ISOLINES;
                  shader->info.TessEval.Spacing = TESS_SPACING_UNSPECIFIED;
   if (state->in_qualifier->flags.q.vertex_spacing)
            shader->info.TessEval.VertexOrder = 0;
   if (state->in_qualifier->flags.q.ordering)
            shader->info.TessEval.PointMode = -1;
   if (state->in_qualifier->flags.q.point_mode)
            case MESA_SHADER_GEOMETRY:
      shader->OES_geometry_point_size_enable = state->OES_geometry_point_size_enable || state->EXT_geometry_point_size_enable;
   shader->info.Geom.VerticesOut = -1;
   if (state->out_qualifier->flags.q.max_vertices) {
      unsigned qual_max_vertices;
   if (state->out_qualifier->max_vertices->
                     if (qual_max_vertices > state->Const.MaxGeometryOutputVertices) {
      YYLTYPE loc = state->out_qualifier->max_vertices->get_location();
   _mesa_glsl_error(&loc, state,
                  }
                  if (state->gs_input_prim_type_specified) {
         } else {
                  if (state->out_qualifier->flags.q.prim_type) {
         } else {
                  shader->info.Geom.Invocations = 0;
   if (state->in_qualifier->flags.q.invocations) {
      unsigned invocations;
   if (state->in_qualifier->invocations->
                     YYLTYPE loc = state->in_qualifier->invocations->get_location();
   if (invocations > state->Const.MaxGeometryShaderInvocations) {
      _mesa_glsl_error(&loc, state,
                  }
         }
         case MESA_SHADER_COMPUTE:
      if (state->cs_input_local_size_specified) {
      for (int i = 0; i < 3; i++)
      } else {
      for (int i = 0; i < 3; i++)
               shader->info.Comp.LocalSizeVariable =
                     if (state->NV_compute_shader_derivatives_enable) {
      /* We allow multiple cs_input_layout nodes, but do not store them in
   * a convenient place, so for now live with an empty location error.
   */
   YYLTYPE loc = {0};
   if (shader->info.Comp.DerivativeGroup == DERIVATIVE_GROUP_QUADS) {
      if (shader->info.Comp.LocalSize[0] % 2 != 0) {
      _mesa_glsl_error(&loc, state, "derivative_group_quadsNV must be used with a "
            }
   if (shader->info.Comp.LocalSize[1] % 2 != 0) {
      _mesa_glsl_error(&loc, state, "derivative_group_quadsNV must be used with a "
               } else if (shader->info.Comp.DerivativeGroup == DERIVATIVE_GROUP_LINEAR) {
      if ((shader->info.Comp.LocalSize[0] *
      shader->info.Comp.LocalSize[1] *
   shader->info.Comp.LocalSize[2]) % 4 != 0) {
   _mesa_glsl_error(&loc, state, "derivative_group_linearNV must be used with a "
                                 case MESA_SHADER_FRAGMENT:
      shader->redeclares_gl_fragcoord = state->fs_redeclares_gl_fragcoord;
   shader->uses_gl_fragcoord = state->fs_uses_gl_fragcoord;
   shader->pixel_center_integer = state->fs_pixel_center_integer;
   shader->origin_upper_left = state->fs_origin_upper_left;
   shader->ARB_fragment_coord_conventions_enable =
         shader->EarlyFragmentTests = state->fs_early_fragment_tests;
   shader->InnerCoverage = state->fs_inner_coverage;
   shader->PostDepthCoverage = state->fs_post_depth_coverage;
   shader->PixelInterlockOrdered = state->fs_pixel_interlock_ordered;
   shader->PixelInterlockUnordered = state->fs_pixel_interlock_unordered;
   shader->SampleInterlockOrdered = state->fs_sample_interlock_ordered;
   shader->SampleInterlockUnordered = state->fs_sample_interlock_unordered;
   shader->BlendSupport = state->fs_blend_support;
         default:
      /* Nothing to do. */
               shader->bindless_sampler = state->bindless_sampler_specified;
   shader->bindless_image = state->bindless_image_specified;
   shader->bound_sampler = state->bound_sampler_specified;
   shader->bound_image = state->bound_image_specified;
   shader->redeclares_gl_layer = state->redeclares_gl_layer;
      }
      /* src can be NULL if only the symbols found in the exec_list should be
   * copied
   */
   void
   _mesa_glsl_copy_symbols_from_table(struct exec_list *shader_ir,
               {
      foreach_in_list (ir_instruction, ir, shader_ir) {
      switch (ir->ir_type) {
   case ir_type_function:
      dest->add_function((ir_function *) ir);
      case ir_type_variable: {
               if (var->data.mode != ir_var_temporary)
            }
   default:
                     if (src != NULL) {
      /* Explicitly copy the gl_PerVertex interface definitions because these
   * are needed to check they are the same during the interstage link.
   * They can’t necessarily be found via the exec_list because the members
   * might not be referenced. The GL spec still requires that they match
   * in that case.
   */
   const glsl_type *iface =
         if (iface)
            iface = src->get_interface("gl_PerVertex", ir_var_shader_out);
   if (iface)
         }
      extern "C" {
      static void
   assign_subroutine_indexes(struct _mesa_glsl_parse_state *state)
   {
      int j, k;
            for (j = 0; j < state->num_subroutines; j++) {
      while (state->subroutines[j]->subroutine_index == -1) {
      for (k = 0; k < state->num_subroutines; k++) {
      if (state->subroutines[k]->subroutine_index == index)
         else if (k == state->num_subroutines - 1) {
            }
            }
      static void
   add_builtin_defines(struct _mesa_glsl_parse_state *state,
                     void (*add_builtin_define)(struct glcpp_parser *, const char *, int),
   {
      unsigned gl_version = state->exts->Version;
            if (gl_version != 0xff) {
      unsigned i;
   for (i = 0; i < state->num_supported_versions; i++) {
      if (state->supported_versions[i].ver == version &&
      state->supported_versions[i].es == es) {
   gl_version = state->supported_versions[i].gl_ver;
                  if (i == state->num_supported_versions)
               if (es)
            for (unsigned i = 0;
      i < ARRAY_SIZE(_mesa_glsl_supported_extensions); ++i) {
   const _mesa_glsl_extension *extension
         if (extension->compatible_with_state(state, api, gl_version)) {
               }
      /* Implements parsing checks that we can't do during parsing */
   static void
   do_late_parsing_checks(struct _mesa_glsl_parse_state *state)
   {
      if (state->stage == MESA_SHADER_COMPUTE && !state->has_compute_shader()) {
      YYLTYPE loc;
   memset(&loc, 0, sizeof(loc));
   _mesa_glsl_error(&loc, state, "Compute shaders require "
         }
      static void
   opt_shader_and_create_symbol_table(const struct gl_constants *consts,
               {
      assert(shader->CompileStatus != COMPILE_FAILURE &&
            const struct gl_shader_compiler_options *options =
            /* Do some optimization at compile time to reduce shader IR size
   * and reduce later work if the same shader is linked multiple times.
   *
   * Run it just once, since NIR will do the real optimization.
   */
                     enum ir_variable_mode other;
   switch (shader->Stage) {
   case MESA_SHADER_VERTEX:
      other = ir_var_shader_in;
      case MESA_SHADER_FRAGMENT:
      other = ir_var_shader_out;
      default:
      /* Something invalid to ensure optimize_dead_builtin_uniforms
   * doesn't remove anything other than uniforms or constants.
   */
   other = ir_var_mode_count;
                                 /* Retain any live IR, but trash the rest. */
            /* Destroy the symbol table.  Create a new symbol table that contains only
   * the variables and functions that still exist in the IR.  The symbol
   * table will be used later during linking.
   *
   * There must NOT be any freed objects still referenced by the symbol
   * table.  That could cause the linker to dereference freed memory.
   *
   * We don't have to worry about types or interface-types here because those
   * are fly-weights that are looked up by glsl_type.
   */
   _mesa_glsl_copy_symbols_from_table(shader->ir, source_symbols,
      }
      static bool
   can_skip_compile(struct gl_context *ctx, struct gl_shader *shader,
                     {
      if (!force_recompile) {
      if (ctx->Cache) {
      char buf[41];
   disk_cache_compute_key(ctx->Cache, source, strlen(source),
         if (disk_cache_has_key(ctx->Cache, shader->disk_cache_sha1)) {
      /* We've seen this shader before and know it compiles */
   if (ctx->_Shader->Flags & GLSL_CACHE_INFO) {
      _mesa_sha1_format(buf, shader->disk_cache_sha1);
                              /* Copy pre-processed shader include to fallback source otherwise
   * we have no guarantee the shader include source tree has not
   * changed.
   */
   if (source_has_shader_include) {
      shader->FallbackSource = strdup(source);
   memcpy(shader->fallback_source_sha1, source_sha1,
      } else {
         }
   memcpy(shader->compiled_source_sha1, source_sha1,
                  } else {
      /* We should only ever end up here if a re-compile has been forced by a
   * shader cache miss. In which case we can skip the compile if its
   * already been done by a previous fallback or the initial compile call.
   */
   if (shader->CompileStatus == COMPILE_SUCCESS)
                  }
      void
   _mesa_glsl_compile_shader(struct gl_context *ctx, struct gl_shader *shader,
         {
      const char *source;
            if (force_recompile && shader->FallbackSource) {
      source = shader->FallbackSource;
      } else {
      source = shader->Source;
               /* Note this will be true for shaders the have #include inside comments
   * however that should be rare enough not to worry about.
   */
   bool source_has_shader_include =
            /* If there was no shader include we can check the shader cache and skip
   * compilation before we run the preprocessor. We never skip compiling
   * shaders that use ARB_shading_language_include because we would need to
   * keep duplicate copies of the shader include source tree and paths.
   */
   if (!source_has_shader_include &&
      can_skip_compile(ctx, shader, source, source_sha1, force_recompile,
               struct _mesa_glsl_parse_state *state =
            if (ctx->Const.GenerateTemporaryNames)
      (void) p_atomic_cmpxchg(&ir_variable::temporaries_allocate_names,
         if (!source_has_shader_include || !force_recompile) {
      state->error = glcpp_preprocess(state, &source, &state->info_log,
               /* Now that we have run the preprocessor we can check the shader cache and
   * skip compilation if possible for those shaders that contained a shader
   * include.
   */
   if (source_has_shader_include &&
      can_skip_compile(ctx, shader, source, source_sha1, force_recompile,
               if (!state->error) {
   _mesa_glsl_lexer_ctor(state, source);
   _mesa_glsl_parse(state);
   _mesa_glsl_lexer_dtor(state);
   do_late_parsing_checks(state);
            if (dump_ast) {
      foreach_list_typed(ast_node, ast, link, &state->translation_unit) {
         }
               ralloc_free(shader->ir);
   shader->ir = new(shader) exec_list;
   if (!state->error && !state->translation_unit.is_empty())
            if (!state->error) {
               /* Print out the unoptimized IR. */
   if (dump_hir) {
                     if (shader->InfoLog)
            if (!state->error)
            shader->symbols = new(shader->ir) glsl_symbol_table;
   shader->CompileStatus = state->error ? COMPILE_FAILURE : COMPILE_SUCCESS;
   shader->InfoLog = state->info_log;
   shader->Version = state->language_version;
            struct gl_shader_compiler_options *options =
            if (!state->error && !shader->ir->is_empty()) {
      if (state->es_shader &&
      (options->LowerPrecisionFloat16 || options->LowerPrecisionInt16))
      lower_builtins(shader->ir);
   assign_subroutine_indexes(state);
   lower_subroutine(shader->ir, state);
               if (!force_recompile) {
               /* Copy pre-processed shader include to fallback source otherwise we
   * have no guarantee the shader include source tree has not changed.
   */
   if (source_has_shader_include) {
      shader->FallbackSource = strdup(source);
      } else {
                     delete state->symbols;
            if (shader->CompileStatus == COMPILE_SUCCESS)
            if (ctx->Cache && shader->CompileStatus == COMPILE_SUCCESS) {
      char sha1_buf[41];
   disk_cache_put_key(ctx->Cache, shader->disk_cache_sha1);
   if (ctx->_Shader->Flags & GLSL_CACHE_INFO) {
      _mesa_sha1_format(sha1_buf, shader->disk_cache_sha1);
            }
      } /* extern "C" */
   /**
   * Do the set of common optimizations passes
   *
   * \param ir                          List of instructions to be optimized
   * \param linked                      Is the shader linked?  This enables
   *                                    optimizations passes that remove code at
   *                                    global scope and could cause linking to
   *                                    fail.
   * \param uniform_locations_assigned  Have locations already been assigned for
   *                                    uniforms?  This prevents the declarations
   *                                    of unused uniforms from being removed.
   *                                    The setting of this flag only matters if
   *                                    \c linked is \c true.
   * \param options                     The driver's preferred shader options.
   * \param native_integers             Selects optimizations that depend on the
   *                                    implementations supporting integers
   *                                    natively (as opposed to supporting
   *                                    integers in floating point registers).
   */
   bool
   do_common_optimization(exec_list *ir, bool linked,
               {
      const bool debug = false;
         #define OPT(PASS, ...) do {                                             \
         if (debug) {                                                      \
      fprintf(stderr, "START GLSL optimization %s\n", #PASS);        \
   const bool opt_progress = PASS(__VA_ARGS__);                   \
   progress = opt_progress || progress;                           \
   if (opt_progress)                                              \
         fprintf(stderr, "GLSL optimization %s: %s progress\n",         \
      } else {                                                          \
                     if (linked) {
      OPT(do_function_inlining, ir);
      }
   OPT(propagate_invariance, ir);
   OPT(do_if_simplification, ir);
            if (options->OptimizeForAOS && !linked)
            OPT(do_dead_code_unlinked, ir);
   OPT(do_dead_code_local, ir);
   OPT(do_tree_grafting, ir);
   OPT(do_minmax_prune, ir);
   OPT(do_rebalance_tree, ir);
   OPT(do_algebraic, ir, native_integers, options);
   OPT(do_lower_jumps, ir, true, true, options->EmitNoMainReturn,
            /* If an optimization pass fails to preserve the invariant flag, calling
   * the pass only once earlier may result in incorrect code generation. Always call
   * propagate_invariance() last to avoid this possibility.
   */
         #undef OPT
            }
