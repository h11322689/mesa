   /*
   * Copyright © 2010 Intel Corporation
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
   * Building this file with MinGW g++ 7.3 or 7.4 with:
   *   scons platform=windows toolchain=crossmingw machine=x86 build=profile
   * triggers an internal compiler error.
   * Overriding the optimization level to -O1 works around the issue.
   * MinGW 5.3.1 does not seem to have the bug, neither does 8.3.  So for now
   * we're simply testing for version 7.x here.
   */
   #if defined(__MINGW32__) && __GNUC__ == 7
   #warning "disabling optimizations for this file to work around compiler bug in MinGW gcc 7.x"
   #pragma GCC optimize("O1")
   #endif
         #include "ir.h"
   #include "ir_builder.h"
   #include "linker.h"
   #include "glsl_parser_extras.h"
   #include "glsl_symbol_table.h"
   #include "main/consts_exts.h"
   #include "main/uniforms.h"
   #include "program/prog_statevars.h"
   #include "program/prog_instruction.h"
   #include "util/compiler.h"
   #include "builtin_functions.h"
      using namespace ir_builder;
      static const struct gl_builtin_uniform_element gl_NumSamples_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_DepthRange_elements[] = {
      {"near", {STATE_DEPTH_RANGE, 0, 0}, SWIZZLE_XXXX},
   {"far", {STATE_DEPTH_RANGE, 0, 0}, SWIZZLE_YYYY},
      };
      static const struct gl_builtin_uniform_element gl_ClipPlane_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_Point_elements[] = {
      {"size", {STATE_POINT_SIZE}, SWIZZLE_XXXX},
   {"sizeMin", {STATE_POINT_SIZE}, SWIZZLE_YYYY},
   {"sizeMax", {STATE_POINT_SIZE}, SWIZZLE_ZZZZ},
   {"fadeThresholdSize", {STATE_POINT_SIZE}, SWIZZLE_WWWW},
   {"distanceConstantAttenuation", {STATE_POINT_ATTENUATION}, SWIZZLE_XXXX},
   {"distanceLinearAttenuation", {STATE_POINT_ATTENUATION}, SWIZZLE_YYYY},
      };
      static const struct gl_builtin_uniform_element gl_FrontMaterial_elements[] = {
      {"emission", {STATE_MATERIAL, MAT_ATTRIB_FRONT_EMISSION}, SWIZZLE_XYZW},
   {"ambient", {STATE_MATERIAL, MAT_ATTRIB_FRONT_AMBIENT}, SWIZZLE_XYZW},
   {"diffuse", {STATE_MATERIAL, MAT_ATTRIB_FRONT_DIFFUSE}, SWIZZLE_XYZW},
   {"specular", {STATE_MATERIAL, MAT_ATTRIB_FRONT_SPECULAR}, SWIZZLE_XYZW},
      };
      static const struct gl_builtin_uniform_element gl_BackMaterial_elements[] = {
      {"emission", {STATE_MATERIAL, MAT_ATTRIB_BACK_EMISSION}, SWIZZLE_XYZW},
   {"ambient", {STATE_MATERIAL, MAT_ATTRIB_BACK_AMBIENT}, SWIZZLE_XYZW},
   {"diffuse", {STATE_MATERIAL, MAT_ATTRIB_BACK_DIFFUSE}, SWIZZLE_XYZW},
   {"specular", {STATE_MATERIAL, MAT_ATTRIB_BACK_SPECULAR}, SWIZZLE_XYZW},
      };
      static const struct gl_builtin_uniform_element gl_LightSource_elements[] = {
      {"ambient", {STATE_LIGHT, 0, STATE_AMBIENT}, SWIZZLE_XYZW},
   {"diffuse", {STATE_LIGHT, 0, STATE_DIFFUSE}, SWIZZLE_XYZW},
   {"specular", {STATE_LIGHT, 0, STATE_SPECULAR}, SWIZZLE_XYZW},
   {"position", {STATE_LIGHT, 0, STATE_POSITION}, SWIZZLE_XYZW},
   {"halfVector", {STATE_LIGHT, 0, STATE_HALF_VECTOR}, SWIZZLE_XYZW},
   {"spotDirection", {STATE_LIGHT, 0, STATE_SPOT_DIRECTION},
   MAKE_SWIZZLE4(SWIZZLE_X,
   SWIZZLE_Y,
   SWIZZLE_Z,
   SWIZZLE_Z)},
   {"spotCosCutoff", {STATE_LIGHT, 0, STATE_SPOT_DIRECTION}, SWIZZLE_WWWW},
   {"constantAttenuation", {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_XXXX},
   {"linearAttenuation", {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_YYYY},
   {"quadraticAttenuation", {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_ZZZZ},
   {"spotExponent", {STATE_LIGHT, 0, STATE_ATTENUATION}, SWIZZLE_WWWW},
      };
      static const struct gl_builtin_uniform_element gl_LightModel_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_FrontLightModelProduct_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_BackLightModelProduct_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_FrontLightProduct_elements[] = {
      {"ambient", {STATE_LIGHTPROD, 0, MAT_ATTRIB_FRONT_AMBIENT}, SWIZZLE_XYZW},
   {"diffuse", {STATE_LIGHTPROD, 0, MAT_ATTRIB_FRONT_DIFFUSE}, SWIZZLE_XYZW},
      };
      static const struct gl_builtin_uniform_element gl_BackLightProduct_elements[] = {
      {"ambient", {STATE_LIGHTPROD, 0, MAT_ATTRIB_BACK_AMBIENT}, SWIZZLE_XYZW},
   {"diffuse", {STATE_LIGHTPROD, 0, MAT_ATTRIB_BACK_DIFFUSE}, SWIZZLE_XYZW},
      };
      static const struct gl_builtin_uniform_element gl_TextureEnvColor_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_EyePlaneS_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_EyePlaneT_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_EyePlaneR_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_EyePlaneQ_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_ObjectPlaneS_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_ObjectPlaneT_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_ObjectPlaneR_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_ObjectPlaneQ_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_Fog_elements[] = {
      {"color", {STATE_FOG_COLOR}, SWIZZLE_XYZW},
   {"density", {STATE_FOG_PARAMS}, SWIZZLE_XXXX},
   {"start", {STATE_FOG_PARAMS}, SWIZZLE_YYYY},
   {"end", {STATE_FOG_PARAMS}, SWIZZLE_ZZZZ},
      };
      static const struct gl_builtin_uniform_element gl_NormalScale_elements[] = {
         };
      static const struct gl_builtin_uniform_element gl_FogParamsOptimizedMESA_elements[] = {
         };
      #define ATTRIB(i) \
      static const struct gl_builtin_uniform_element gl_CurrentAttribFrag##i##MESA_elements[] = { \
               ATTRIB(0)
   ATTRIB(1)
   ATTRIB(2)
   ATTRIB(3)
   ATTRIB(4)
   ATTRIB(5)
   ATTRIB(6)
   ATTRIB(7)
   ATTRIB(8)
   ATTRIB(9)
   ATTRIB(10)
   ATTRIB(11)
   ATTRIB(12)
   ATTRIB(13)
   ATTRIB(14)
   ATTRIB(15)
   ATTRIB(16)
   ATTRIB(17)
   ATTRIB(18)
   ATTRIB(19)
   ATTRIB(20)
   ATTRIB(21)
   ATTRIB(22)
   ATTRIB(23)
   ATTRIB(24)
   ATTRIB(25)
   ATTRIB(26)
   ATTRIB(27)
   ATTRIB(28)
   ATTRIB(29)
   ATTRIB(30)
   ATTRIB(31)
      #define MATRIX(name, statevar)				\
      static const struct gl_builtin_uniform_element name ## _elements[] = { \
      { NULL, { statevar, 0, 0, 0}, SWIZZLE_XYZW },		\
   { NULL, { statevar, 0, 1, 1}, SWIZZLE_XYZW },		\
   { NULL, { statevar, 0, 2, 2}, SWIZZLE_XYZW },		\
            MATRIX(gl_ModelViewMatrix, STATE_MODELVIEW_MATRIX_TRANSPOSE);
   MATRIX(gl_ModelViewMatrixInverse, STATE_MODELVIEW_MATRIX_INVTRANS);
   MATRIX(gl_ModelViewMatrixTranspose, STATE_MODELVIEW_MATRIX);
   MATRIX(gl_ModelViewMatrixInverseTranspose, STATE_MODELVIEW_MATRIX_INVERSE);
      MATRIX(gl_ProjectionMatrix, STATE_PROJECTION_MATRIX_TRANSPOSE);
   MATRIX(gl_ProjectionMatrixInverse, STATE_PROJECTION_MATRIX_INVTRANS);
   MATRIX(gl_ProjectionMatrixTranspose, STATE_PROJECTION_MATRIX);
   MATRIX(gl_ProjectionMatrixInverseTranspose, STATE_PROJECTION_MATRIX_INVERSE);
      MATRIX(gl_ModelViewProjectionMatrix, STATE_MVP_MATRIX_TRANSPOSE);
   MATRIX(gl_ModelViewProjectionMatrixInverse, STATE_MVP_MATRIX_INVTRANS);
   MATRIX(gl_ModelViewProjectionMatrixTranspose, STATE_MVP_MATRIX);
   MATRIX(gl_ModelViewProjectionMatrixInverseTranspose, STATE_MVP_MATRIX_INVERSE);
      MATRIX(gl_TextureMatrix, STATE_TEXTURE_MATRIX_TRANSPOSE);
   MATRIX(gl_TextureMatrixInverse, STATE_TEXTURE_MATRIX_INVTRANS);
   MATRIX(gl_TextureMatrixTranspose, STATE_TEXTURE_MATRIX);
   MATRIX(gl_TextureMatrixInverseTranspose, STATE_TEXTURE_MATRIX_INVERSE);
      static const struct gl_builtin_uniform_element gl_NormalMatrix_elements[] = {
      { NULL, { STATE_MODELVIEW_MATRIX_INVERSE, 0, 0, 0},
   MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z) },
   { NULL, { STATE_MODELVIEW_MATRIX_INVERSE, 0, 1, 1},
   MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z) },
   { NULL, { STATE_MODELVIEW_MATRIX_INVERSE, 0, 2, 2},
      };
      #undef MATRIX
      #define STATEVAR(name) {#name, name ## _elements, ARRAY_SIZE(name ## _elements)}
      static const struct gl_builtin_uniform_desc _mesa_builtin_uniform_desc[] = {
      STATEVAR(gl_NumSamples),
   STATEVAR(gl_DepthRange),
   STATEVAR(gl_ClipPlane),
   STATEVAR(gl_Point),
   STATEVAR(gl_FrontMaterial),
   STATEVAR(gl_BackMaterial),
   STATEVAR(gl_LightSource),
   STATEVAR(gl_LightModel),
   STATEVAR(gl_FrontLightModelProduct),
   STATEVAR(gl_BackLightModelProduct),
   STATEVAR(gl_FrontLightProduct),
   STATEVAR(gl_BackLightProduct),
   STATEVAR(gl_TextureEnvColor),
   STATEVAR(gl_EyePlaneS),
   STATEVAR(gl_EyePlaneT),
   STATEVAR(gl_EyePlaneR),
   STATEVAR(gl_EyePlaneQ),
   STATEVAR(gl_ObjectPlaneS),
   STATEVAR(gl_ObjectPlaneT),
   STATEVAR(gl_ObjectPlaneR),
   STATEVAR(gl_ObjectPlaneQ),
            STATEVAR(gl_ModelViewMatrix),
   STATEVAR(gl_ModelViewMatrixInverse),
   STATEVAR(gl_ModelViewMatrixTranspose),
            STATEVAR(gl_ProjectionMatrix),
   STATEVAR(gl_ProjectionMatrixInverse),
   STATEVAR(gl_ProjectionMatrixTranspose),
            STATEVAR(gl_ModelViewProjectionMatrix),
   STATEVAR(gl_ModelViewProjectionMatrixInverse),
   STATEVAR(gl_ModelViewProjectionMatrixTranspose),
            STATEVAR(gl_TextureMatrix),
   STATEVAR(gl_TextureMatrixInverse),
   STATEVAR(gl_TextureMatrixTranspose),
            STATEVAR(gl_NormalMatrix),
                     STATEVAR(gl_CurrentAttribFrag0MESA),
   STATEVAR(gl_CurrentAttribFrag1MESA),
   STATEVAR(gl_CurrentAttribFrag2MESA),
   STATEVAR(gl_CurrentAttribFrag3MESA),
   STATEVAR(gl_CurrentAttribFrag4MESA),
   STATEVAR(gl_CurrentAttribFrag5MESA),
   STATEVAR(gl_CurrentAttribFrag6MESA),
   STATEVAR(gl_CurrentAttribFrag7MESA),
   STATEVAR(gl_CurrentAttribFrag8MESA),
   STATEVAR(gl_CurrentAttribFrag9MESA),
   STATEVAR(gl_CurrentAttribFrag10MESA),
   STATEVAR(gl_CurrentAttribFrag11MESA),
   STATEVAR(gl_CurrentAttribFrag12MESA),
   STATEVAR(gl_CurrentAttribFrag13MESA),
   STATEVAR(gl_CurrentAttribFrag14MESA),
   STATEVAR(gl_CurrentAttribFrag15MESA),
   STATEVAR(gl_CurrentAttribFrag16MESA),
   STATEVAR(gl_CurrentAttribFrag17MESA),
   STATEVAR(gl_CurrentAttribFrag18MESA),
   STATEVAR(gl_CurrentAttribFrag19MESA),
   STATEVAR(gl_CurrentAttribFrag20MESA),
   STATEVAR(gl_CurrentAttribFrag21MESA),
   STATEVAR(gl_CurrentAttribFrag22MESA),
   STATEVAR(gl_CurrentAttribFrag23MESA),
   STATEVAR(gl_CurrentAttribFrag24MESA),
   STATEVAR(gl_CurrentAttribFrag25MESA),
   STATEVAR(gl_CurrentAttribFrag26MESA),
   STATEVAR(gl_CurrentAttribFrag27MESA),
   STATEVAR(gl_CurrentAttribFrag28MESA),
   STATEVAR(gl_CurrentAttribFrag29MESA),
   STATEVAR(gl_CurrentAttribFrag30MESA),
               };
         namespace {
      /**
   * Data structure that accumulates fields for the gl_PerVertex interface
   * block.
   */
   class per_vertex_accumulator
   {
   public:
      per_vertex_accumulator();
   void add_field(int slot, const glsl_type *type, int precision,
               private:
      glsl_struct_field fields[14];
      };
         per_vertex_accumulator::per_vertex_accumulator()
      : fields(),
      {
   }
         void
   per_vertex_accumulator::add_field(int slot, const glsl_type *type,
               {
      assert(this->num_fields < ARRAY_SIZE(this->fields));
   this->fields[this->num_fields].type = type;
   this->fields[this->num_fields].name = name;
   this->fields[this->num_fields].matrix_layout = GLSL_MATRIX_LAYOUT_INHERITED;
   this->fields[this->num_fields].location = slot;
   this->fields[this->num_fields].offset = -1;
   this->fields[this->num_fields].interpolation = interp;
   this->fields[this->num_fields].centroid = 0;
   this->fields[this->num_fields].sample = 0;
   this->fields[this->num_fields].patch = 0;
   this->fields[this->num_fields].precision = precision;
   this->fields[this->num_fields].memory_read_only = 0;
   this->fields[this->num_fields].memory_write_only = 0;
   this->fields[this->num_fields].memory_coherent = 0;
   this->fields[this->num_fields].memory_volatile = 0;
   this->fields[this->num_fields].memory_restrict = 0;
   this->fields[this->num_fields].image_format = PIPE_FORMAT_NONE;
   this->fields[this->num_fields].explicit_xfb_buffer = 0;
   this->fields[this->num_fields].xfb_buffer = -1;
   this->fields[this->num_fields].xfb_stride = -1;
      }
         const glsl_type *
   per_vertex_accumulator::construct_interface_instance() const
   {
      return glsl_type::get_interface_instance(this->fields, this->num_fields,
                  }
         class builtin_variable_generator
   {
   public:
      builtin_variable_generator(exec_list *instructions,
         void generate_constants();
   void generate_uniforms();
   void generate_special_vars();
   void generate_vs_special_vars();
   void generate_tcs_special_vars();
   void generate_tes_special_vars();
   void generate_gs_special_vars();
   void generate_fs_special_vars();
   void generate_cs_special_vars();
         private:
      const glsl_type *array(const glsl_type *base, unsigned elements)
   {
                  const glsl_type *type(const char *name)
   {
                  ir_variable *add_input(int slot, const glsl_type *type, int precision,
               {
                  ir_variable *add_input(int slot, const glsl_type *type, const char *name,
         {
                  ir_variable *add_output(int slot, const glsl_type *type, int precision,
         {
                  ir_variable *add_output(int slot, const glsl_type *type, const char *name)
   {
                  ir_variable *add_index_output(int slot, int index, const glsl_type *type,
         {
      return add_index_variable(name, type, precision, ir_var_shader_out, slot,
               ir_variable *add_system_value(int slot, const glsl_type *type, int precision,
         {
         }
   ir_variable *add_system_value(int slot, const glsl_type *type,
         {
                  ir_variable *add_variable(const char *name, const glsl_type *type,
               ir_variable *add_index_variable(const char *name, const glsl_type *type,
               ir_variable *add_uniform(const glsl_type *type, int precision,
         ir_variable *add_uniform(const glsl_type *type, const char *name)
   {
         }
   ir_variable *add_const(const char *name, int precision, int value);
   ir_variable *add_const(const char *name, int value)
   {
         }
   ir_variable *add_const_ivec3(const char *name, int x, int y, int z);
   void add_varying(int slot, const glsl_type *type, int precision,
               void add_varying(int slot, const glsl_type *type, const char *name,
         {
                  exec_list * const instructions;
   struct _mesa_glsl_parse_state * const state;
            /**
   * True if compatibility-profile-only variables should be included.  (In
   * desktop GL, these are always included when the GLSL version is 1.30 and
   * or below).
   */
            const glsl_type * const bool_t;
   const glsl_type * const int_t;
   const glsl_type * const uint_t;
   const glsl_type * const uint64_t;
   const glsl_type * const float_t;
   const glsl_type * const vec2_t;
   const glsl_type * const vec3_t;
   const glsl_type * const vec4_t;
   const glsl_type * const uvec3_t;
   const glsl_type * const mat3_t;
            per_vertex_accumulator per_vertex_in;
      };
         builtin_variable_generator::builtin_variable_generator(
      exec_list *instructions, struct _mesa_glsl_parse_state *state)
   : instructions(instructions), state(state), symtab(state->symbols),
   compatibility(state->compat_shader || state->ARB_compatibility_enable),
   bool_t(glsl_type::bool_type), int_t(glsl_type::int_type),
   uint_t(glsl_type::uint_type),
   uint64_t(glsl_type::uint64_t_type),
   float_t(glsl_type::float_type), vec2_t(glsl_type::vec2_type),
   vec3_t(glsl_type::vec3_type), vec4_t(glsl_type::vec4_type),
   uvec3_t(glsl_type::uvec3_type),
      {
   }
      ir_variable *
   builtin_variable_generator::add_index_variable(const char *name,
                           {
      ir_variable *var = new(symtab) ir_variable(type, name, mode);
            switch (var->data.mode) {
   case ir_var_auto:
   case ir_var_shader_in:
   case ir_var_uniform:
   case ir_var_system_value:
      var->data.read_only = true;
      case ir_var_shader_out:
   case ir_var_shader_storage:
         default:
      /* The only variables that are added using this function should be
   * uniforms, shader storage, shader inputs, and shader outputs, constants
   * (which use ir_var_auto), and system values.
   */
   assert(0);
               var->data.location = slot;
   var->data.explicit_location = (slot >= 0);
   var->data.explicit_index = 1;
            if (state->es_shader)
            /* Once the variable is created an initialized, add it to the symbol table
   * and add the declaration to the IR stream.
   */
            symtab->add_variable(var);
      }
      ir_variable *
   builtin_variable_generator::add_variable(const char *name,
                           {
      ir_variable *var = new(symtab) ir_variable(type, name, mode);
            switch (var->data.mode) {
   case ir_var_auto:
   case ir_var_shader_in:
   case ir_var_uniform:
   case ir_var_system_value:
      var->data.read_only = true;
      case ir_var_shader_out:
   case ir_var_shader_storage:
         default:
      /* The only variables that are added using this function should be
   * uniforms, shader storage, shader inputs, and shader outputs, constants
   * (which use ir_var_auto), and system values.
   */
   assert(0);
               var->data.location = slot;
   var->data.explicit_location = (slot >= 0);
   var->data.explicit_index = 0;
            if (state->es_shader)
            /* Once the variable is created an initialized, add it to the symbol table
   * and add the declaration to the IR stream.
   */
            symtab->add_variable(var);
      }
      extern "C" const struct gl_builtin_uniform_desc *
   _mesa_glsl_get_builtin_uniform_desc(const char *name)
   {
      for (unsigned i = 0; _mesa_builtin_uniform_desc[i].name != NULL; i++) {
      if (strcmp(_mesa_builtin_uniform_desc[i].name, name) == 0) {
            }
      }
      ir_variable *
   builtin_variable_generator::add_uniform(const glsl_type *type,
               {
      ir_variable *const uni =
            const struct gl_builtin_uniform_desc* const statevar =
                           ir_state_slot *slots =
            for (unsigned a = 0; a < array_count; a++) {
      const struct gl_builtin_uniform_element *element =
            memcpy(slots->tokens, element->tokens, sizeof(element->tokens));
   if (type->is_array())
            slots++;
                        }
         ir_variable *
   builtin_variable_generator::add_const(const char *name, int precision,
         {
      ir_variable *const var = add_variable(name, glsl_type::int_type,
         var->constant_value = new(var) ir_constant(value);
   var->constant_initializer = new(var) ir_constant(value);
   var->data.has_initializer = true;
      }
         ir_variable *
   builtin_variable_generator::add_const_ivec3(const char *name, int x, int y,
         {
      ir_variable *const var = add_variable(name, glsl_type::ivec3_type,
               ir_constant_data data;
   memset(&data, 0, sizeof(data));
   data.i[0] = x;
   data.i[1] = y;
   data.i[2] = z;
   var->constant_value = new(var) ir_constant(glsl_type::ivec3_type, &data);
   var->constant_initializer =
         var->data.has_initializer = true;
      }
         void
   builtin_variable_generator::generate_constants()
   {
      add_const("gl_MaxVertexAttribs", state->Const.MaxVertexAttribs);
   add_const("gl_MaxVertexTextureImageUnits",
         add_const("gl_MaxCombinedTextureImageUnits",
         add_const("gl_MaxTextureImageUnits", state->Const.MaxTextureImageUnits);
            /* Max uniforms/varyings: GLSL ES counts these in units of vectors; desktop
   * GL counts them in units of "components" or "floats" and also in units
   * of vectors since GL 4.1
   */
   if (!state->es_shader) {
      add_const("gl_MaxFragmentUniformComponents",
         add_const("gl_MaxVertexUniformComponents",
               if (state->is_version(410, 100)) {
      add_const("gl_MaxVertexUniformVectors",
         add_const("gl_MaxFragmentUniformVectors",
            /* In GLSL ES 3.00, gl_MaxVaryingVectors was split out to separate
   * vertex and fragment shader constants.
   */
   if (state->is_version(0, 300)) {
      add_const("gl_MaxVertexOutputVectors",
         add_const("gl_MaxFragmentInputVectors",
      } else {
      add_const("gl_MaxVaryingVectors",
               /* EXT_blend_func_extended brings a built in constant
   * for determining number of dual source draw buffers
   */
   if (state->EXT_blend_func_extended_enable) {
      add_const("gl_MaxDualSourceDrawBuffersEXT",
                  /* gl_MaxVaryingFloats was deprecated in GLSL 1.30+, and was moved to
   * compat profile in GLSL 4.20. GLSL ES never supported this constant.
   */
   if (compatibility || !state->is_version(420, 100))  {
                  /* Texel offsets were introduced in ARB_shading_language_420pack (which
   * requires desktop GLSL version 130), and adopted into desktop GLSL
   * version 4.20 and GLSL ES version 3.00.
   */
   if ((state->is_version(130, 0) &&
      state->ARB_shading_language_420pack_enable) ||
   state->is_version(420, 300)) {
   add_const("gl_MinProgramTexelOffset",
         add_const("gl_MaxProgramTexelOffset",
               if (state->has_clip_distance()) {
         }
   if (state->is_version(130, 0)) {
         }
   if (state->has_cull_distance()) {
      add_const("gl_MaxCullDistances", state->Const.MaxClipPlanes);
   add_const("gl_MaxCombinedClipAndCullDistances",
               if (state->has_geometry_shader()) {
      add_const("gl_MaxVertexOutputComponents",
         add_const("gl_MaxGeometryInputComponents",
         add_const("gl_MaxGeometryOutputComponents",
         add_const("gl_MaxFragmentInputComponents",
         add_const("gl_MaxGeometryTextureImageUnits",
         add_const("gl_MaxGeometryOutputVertices",
         add_const("gl_MaxGeometryTotalOutputComponents",
         add_const("gl_MaxGeometryUniformComponents",
            /* Note: the GLSL 1.50-4.40 specs require
   * gl_MaxGeometryVaryingComponents to be present, and to be at least 64.
   * But they do not define what it means (and there does not appear to be
   * any corresponding constant in the GL specs).  However,
   * ARB_geometry_shader4 defines MAX_GEOMETRY_VARYING_COMPONENTS_ARB to
   * be the maximum number of components available for use as geometry
   * outputs.  So we assume this is a synonym for
   * gl_MaxGeometryOutputComponents.
   */
   add_const("gl_MaxGeometryVaryingComponents",
               if (compatibility) {
      /* Note: gl_MaxLights stopped being listed as an explicit constant in
   * GLSL 1.30, however it continues to be referred to (as a minimum size
   * for compatibility-mode uniforms) all the way up through GLSL 4.30, so
   * this seems like it was probably an oversight.
   */
                     /* Note: gl_MaxTextureUnits wasn't made compatibility-only until GLSL
   * 1.50, however this seems like it was probably an oversight.
   */
            /* Note: gl_MaxTextureCoords was left out of GLSL 1.40, but it was
   * re-introduced in GLSL 1.50, so this seems like it was probably an
   * oversight.
   */
               if (state->has_atomic_counters()) {
      add_const("gl_MaxVertexAtomicCounters",
         add_const("gl_MaxFragmentAtomicCounters",
         add_const("gl_MaxCombinedAtomicCounters",
         add_const("gl_MaxAtomicCounterBindings",
            if (state->has_geometry_shader()) {
      add_const("gl_MaxGeometryAtomicCounters",
      }
   if (state->is_version(110, 320)) {
      add_const("gl_MaxTessControlAtomicCounters",
         add_const("gl_MaxTessEvaluationAtomicCounters",
                  if (state->is_version(420, 310)) {
      add_const("gl_MaxVertexAtomicCounterBuffers",
         add_const("gl_MaxFragmentAtomicCounterBuffers",
         add_const("gl_MaxCombinedAtomicCounterBuffers",
         add_const("gl_MaxAtomicCounterBufferSize",
            if (state->has_geometry_shader()) {
      add_const("gl_MaxGeometryAtomicCounterBuffers",
      }
   if (state->is_version(110, 320)) {
      add_const("gl_MaxTessControlAtomicCounterBuffers",
         add_const("gl_MaxTessEvaluationAtomicCounterBuffers",
                  if (state->is_version(430, 310) || state->ARB_compute_shader_enable) {
      add_const("gl_MaxComputeAtomicCounterBuffers",
         add_const("gl_MaxComputeAtomicCounters",
         add_const("gl_MaxComputeImageUniforms",
         add_const("gl_MaxComputeTextureImageUnits",
         add_const("gl_MaxComputeUniformComponents",
            add_const_ivec3("gl_MaxComputeWorkGroupCount",
                     add_const_ivec3("gl_MaxComputeWorkGroupSize",
                        /* From the GLSL 4.40 spec, section 7.1 (Built-In Language Variables):
   *
   *     The built-in constant gl_WorkGroupSize is a compute-shader
   *     constant containing the local work-group size of the shader.  The
   *     size of the work group in the X, Y, and Z dimensions is stored in
   *     the x, y, and z components.  The constants values in
   *     gl_WorkGroupSize will match those specified in the required
   *     local_size_x, local_size_y, and local_size_z layout qualifiers
   *     for the current shader.  This is a constant so that it can be
   *     used to size arrays of memory that can be shared within the local
   *     work group.  It is a compile-time error to use gl_WorkGroupSize
   *     in a shader that does not declare a fixed local group size, or
   *     before that shader has declared a fixed local group size, using
   *     local_size_x, local_size_y, and local_size_z.
   *
   * To prevent the shader from trying to refer to gl_WorkGroupSize before
   * the layout declaration, we don't define it here.  Intead we define it
   * in ast_cs_input_layout::hir().
               if (state->has_enhanced_layouts()) {
      add_const("gl_MaxTransformFeedbackBuffers",
         add_const("gl_MaxTransformFeedbackInterleavedComponents",
               if (state->has_shader_image_load_store()) {
      add_const("gl_MaxImageUnits",
         add_const("gl_MaxVertexImageUniforms",
         add_const("gl_MaxFragmentImageUniforms",
         add_const("gl_MaxCombinedImageUniforms",
            if (state->has_geometry_shader()) {
      add_const("gl_MaxGeometryImageUniforms",
               if (!state->es_shader) {
      add_const("gl_MaxCombinedImageUnitsAndFragmentOutputs",
         add_const("gl_MaxImageSamples",
               if (state->has_tessellation_shader()) {
      add_const("gl_MaxTessControlImageUniforms",
         add_const("gl_MaxTessEvaluationImageUniforms",
                  if (state->is_version(440, 310) ||
      state->ARB_ES3_1_compatibility_enable) {
   add_const("gl_MaxCombinedShaderOutputResources",
               if (state->is_version(410, 0) ||
      state->ARB_viewport_array_enable ||
   state->OES_viewport_array_enable) {
   add_const("gl_MaxViewports", GLSL_PRECISION_HIGH,
               if (state->has_tessellation_shader()) {
      add_const("gl_MaxPatchVertices", state->Const.MaxPatchVertices);
   add_const("gl_MaxTessGenLevel", state->Const.MaxTessGenLevel);
   add_const("gl_MaxTessControlInputComponents", state->Const.MaxTessControlInputComponents);
   add_const("gl_MaxTessControlOutputComponents", state->Const.MaxTessControlOutputComponents);
   add_const("gl_MaxTessControlTextureImageUnits", state->Const.MaxTessControlTextureImageUnits);
   add_const("gl_MaxTessEvaluationInputComponents", state->Const.MaxTessEvaluationInputComponents);
   add_const("gl_MaxTessEvaluationOutputComponents", state->Const.MaxTessEvaluationOutputComponents);
   add_const("gl_MaxTessEvaluationTextureImageUnits", state->Const.MaxTessEvaluationTextureImageUnits);
   add_const("gl_MaxTessPatchComponents", state->Const.MaxTessPatchComponents);
   add_const("gl_MaxTessControlTotalOutputComponents", state->Const.MaxTessControlTotalOutputComponents);
   add_const("gl_MaxTessControlUniformComponents", state->Const.MaxTessControlUniformComponents);
               if (state->is_version(450, 320) ||
      state->OES_sample_variables_enable ||
   state->ARB_ES3_1_compatibility_enable)
   }
         /**
   * Generate uniform variables (which exist in all types of shaders).
   */
   void
   builtin_variable_generator::generate_uniforms()
   {
      if (state->is_version(400, 320) ||
      state->ARB_sample_shading_enable ||
   state->OES_sample_variables_enable)
               for (unsigned i = 0; i < VARYING_SLOT_VAR0; i++) {
               snprintf(name, sizeof(name), "gl_CurrentAttribFrag%uMESA", i);
               if (compatibility) {
      add_uniform(mat4_t, "gl_ModelViewMatrix");
   add_uniform(mat4_t, "gl_ProjectionMatrix");
   add_uniform(mat4_t, "gl_ModelViewProjectionMatrix");
   add_uniform(mat3_t, "gl_NormalMatrix");
   add_uniform(mat4_t, "gl_ModelViewMatrixInverse");
   add_uniform(mat4_t, "gl_ProjectionMatrixInverse");
   add_uniform(mat4_t, "gl_ModelViewProjectionMatrixInverse");
   add_uniform(mat4_t, "gl_ModelViewMatrixTranspose");
   add_uniform(mat4_t, "gl_ProjectionMatrixTranspose");
   add_uniform(mat4_t, "gl_ModelViewProjectionMatrixTranspose");
   add_uniform(mat4_t, "gl_ModelViewMatrixInverseTranspose");
   add_uniform(mat4_t, "gl_ProjectionMatrixInverseTranspose");
   add_uniform(mat4_t, "gl_ModelViewProjectionMatrixInverseTranspose");
   add_uniform(float_t, "gl_NormalScale");
   add_uniform(type("gl_LightModelParameters"), "gl_LightModel");
            array(mat4_t, state->Const.MaxTextureCoords);
         add_uniform(mat4_array_type, "gl_TextureMatrix");
   add_uniform(mat4_array_type, "gl_TextureMatrixInverse");
   add_uniform(mat4_array_type, "gl_TextureMatrixTranspose");
            add_uniform(array(vec4_t, state->Const.MaxClipPlanes), "gl_ClipPlane");
            type("gl_MaterialParameters");
         add_uniform(material_parameters_type, "gl_FrontMaterial");
            add_uniform(array(type("gl_LightSourceParameters"),
                  const glsl_type *const light_model_products_type =
         add_uniform(light_model_products_type, "gl_FrontLightModelProduct");
            const glsl_type *const light_products_type =
         add_uniform(light_products_type, "gl_FrontLightProduct");
            add_uniform(array(vec4_t, state->Const.MaxTextureUnits),
            array(vec4_t, state->Const.MaxTextureCoords);
         add_uniform(texcoords_vec4, "gl_EyePlaneS");
   add_uniform(texcoords_vec4, "gl_EyePlaneT");
   add_uniform(texcoords_vec4, "gl_EyePlaneR");
   add_uniform(texcoords_vec4, "gl_EyePlaneQ");
   add_uniform(texcoords_vec4, "gl_ObjectPlaneS");
   add_uniform(texcoords_vec4, "gl_ObjectPlaneT");
   add_uniform(texcoords_vec4, "gl_ObjectPlaneR");
                  }
         /**
   * Generate special variables which exist in all shaders.
   */
   void
   builtin_variable_generator::generate_special_vars()
   {
      if (state->ARB_shader_ballot_enable) {
      add_system_value(SYSTEM_VALUE_SUBGROUP_SIZE, uint_t, "gl_SubGroupSizeARB");
   add_system_value(SYSTEM_VALUE_SUBGROUP_INVOCATION, uint_t, "gl_SubGroupInvocationARB");
   add_system_value(SYSTEM_VALUE_SUBGROUP_EQ_MASK, uint64_t, "gl_SubGroupEqMaskARB");
   add_system_value(SYSTEM_VALUE_SUBGROUP_GE_MASK, uint64_t, "gl_SubGroupGeMaskARB");
   add_system_value(SYSTEM_VALUE_SUBGROUP_GT_MASK, uint64_t, "gl_SubGroupGtMaskARB");
   add_system_value(SYSTEM_VALUE_SUBGROUP_LE_MASK, uint64_t, "gl_SubGroupLeMaskARB");
         }
         /**
   * Generate variables which only exist in vertex shaders.
   */
   void
   builtin_variable_generator::generate_vs_special_vars()
   {
      if (state->is_version(130, 300) || state->EXT_gpu_shader4_enable) {
      add_system_value(SYSTEM_VALUE_VERTEX_ID, int_t, GLSL_PRECISION_HIGH,
      }
   if (state->is_version(460, 0)) {
      add_system_value(SYSTEM_VALUE_BASE_VERTEX, int_t, "gl_BaseVertex");
   add_system_value(SYSTEM_VALUE_BASE_INSTANCE, int_t, "gl_BaseInstance");
      }
   if (state->EXT_draw_instanced_enable && state->is_version(0, 100))
      add_system_value(SYSTEM_VALUE_INSTANCE_ID, int_t, GLSL_PRECISION_HIGH,
         if (state->ARB_draw_instanced_enable)
            if (state->ARB_draw_instanced_enable || state->is_version(140, 300) ||
      state->EXT_gpu_shader4_enable) {
   add_system_value(SYSTEM_VALUE_INSTANCE_ID, int_t, GLSL_PRECISION_HIGH,
      }
   if (state->ARB_shader_draw_parameters_enable) {
      add_system_value(SYSTEM_VALUE_BASE_VERTEX, int_t, "gl_BaseVertexARB");
   add_system_value(SYSTEM_VALUE_BASE_INSTANCE, int_t, "gl_BaseInstanceARB");
      }
   if (compatibility) {
      add_input(VERT_ATTRIB_POS, vec4_t, "gl_Vertex");
   add_input(VERT_ATTRIB_NORMAL, vec3_t, "gl_Normal");
   add_input(VERT_ATTRIB_COLOR0, vec4_t, "gl_Color");
   add_input(VERT_ATTRIB_COLOR1, vec4_t, "gl_SecondaryColor");
   add_input(VERT_ATTRIB_TEX0, vec4_t, "gl_MultiTexCoord0");
   add_input(VERT_ATTRIB_TEX1, vec4_t, "gl_MultiTexCoord1");
   add_input(VERT_ATTRIB_TEX2, vec4_t, "gl_MultiTexCoord2");
   add_input(VERT_ATTRIB_TEX3, vec4_t, "gl_MultiTexCoord3");
   add_input(VERT_ATTRIB_TEX4, vec4_t, "gl_MultiTexCoord4");
   add_input(VERT_ATTRIB_TEX5, vec4_t, "gl_MultiTexCoord5");
   add_input(VERT_ATTRIB_TEX6, vec4_t, "gl_MultiTexCoord6");
   add_input(VERT_ATTRIB_TEX7, vec4_t, "gl_MultiTexCoord7");
         }
         /**
   * Generate variables which only exist in tessellation control shaders.
   */
   void
   builtin_variable_generator::generate_tcs_special_vars()
   {
      add_system_value(SYSTEM_VALUE_PRIMITIVE_ID, int_t, GLSL_PRECISION_HIGH,
         add_system_value(SYSTEM_VALUE_INVOCATION_ID, int_t, GLSL_PRECISION_HIGH,
         add_system_value(SYSTEM_VALUE_VERTICES_IN, int_t, GLSL_PRECISION_HIGH,
            add_output(VARYING_SLOT_TESS_LEVEL_OUTER, array(float_t, 4),
         add_output(VARYING_SLOT_TESS_LEVEL_INNER, array(float_t, 2),
         /* XXX What to do if multiple are flipped on? */
   int bbox_slot = state->consts->NoPrimitiveBoundingBoxOutput ? -1 :
         if (state->EXT_primitive_bounding_box_enable)
      add_output(bbox_slot, array(vec4_t, 2), "gl_BoundingBoxEXT")
      if (state->OES_primitive_bounding_box_enable) {
      add_output(bbox_slot, array(vec4_t, 2), GLSL_PRECISION_HIGH,
      }
   if (state->is_version(0, 320) || state->ARB_ES3_2_compatibility_enable) {
      add_output(bbox_slot, array(vec4_t, 2), GLSL_PRECISION_HIGH,
               /* NOTE: These are completely pointless. Writing these will never go
   * anywhere. But the specs demands it. So we add them with a slot of -1,
   * which makes the data go nowhere.
   */
   if (state->NV_viewport_array2_enable) {
      add_output(-1, int_t, "gl_Layer");
   add_output(-1, int_t, "gl_ViewportIndex");
            }
         /**
   * Generate variables which only exist in tessellation evaluation shaders.
   */
   void
   builtin_variable_generator::generate_tes_special_vars()
   {
               add_system_value(SYSTEM_VALUE_PRIMITIVE_ID, int_t, GLSL_PRECISION_HIGH,
         add_system_value(SYSTEM_VALUE_VERTICES_IN, int_t, GLSL_PRECISION_HIGH,
         add_system_value(SYSTEM_VALUE_TESS_COORD, vec3_t, GLSL_PRECISION_HIGH,
         if (this->state->consts->GLSLTessLevelsAsInputs) {
      add_input(VARYING_SLOT_TESS_LEVEL_OUTER, array(float_t, 4),
         add_input(VARYING_SLOT_TESS_LEVEL_INNER, array(float_t, 2),
      } else {
      add_system_value(SYSTEM_VALUE_TESS_LEVEL_OUTER, array(float_t, 4),
         add_system_value(SYSTEM_VALUE_TESS_LEVEL_INNER, array(float_t, 2),
      }
   if (state->ARB_shader_viewport_layer_array_enable ||
      state->NV_viewport_array2_enable) {
   var = add_output(VARYING_SLOT_LAYER, int_t, "gl_Layer");
   var->data.interpolation = INTERP_MODE_FLAT;
   var = add_output(VARYING_SLOT_VIEWPORT, int_t, "gl_ViewportIndex");
      }
   if (state->NV_viewport_array2_enable) {
      var = add_output(VARYING_SLOT_VIEWPORT_MASK, array(int_t, 1),
               }
         /**
   * Generate variables which only exist in geometry shaders.
   */
   void
   builtin_variable_generator::generate_gs_special_vars()
   {
               var = add_output(VARYING_SLOT_LAYER, int_t, GLSL_PRECISION_HIGH, "gl_Layer");
   var->data.interpolation = INTERP_MODE_FLAT;
   if (state->is_version(410, 0) || state->ARB_viewport_array_enable ||
      state->OES_viewport_array_enable) {
   var = add_output(VARYING_SLOT_VIEWPORT, int_t, GLSL_PRECISION_HIGH,
            }
   if (state->NV_viewport_array2_enable) {
      var = add_output(VARYING_SLOT_VIEWPORT_MASK, array(int_t, 1),
            }
   if (state->is_version(400, 320) || state->ARB_gpu_shader5_enable ||
      state->OES_geometry_shader_enable || state->EXT_geometry_shader_enable) {
   add_system_value(SYSTEM_VALUE_INVOCATION_ID, int_t, GLSL_PRECISION_HIGH,
               /* Although gl_PrimitiveID appears in tessellation control and tessellation
   * evaluation shaders, it has a different function there than it has in
   * geometry shaders, so we treat it (and its counterpart gl_PrimitiveIDIn)
   * as special geometry shader variables.
   *
   * Note that although the general convention of suffixing geometry shader
   * input varyings with "In" was not adopted into GLSL 1.50, it is used in
   * the specific case of gl_PrimitiveIDIn.  So we don't need to treat
   * gl_PrimitiveIDIn as an {ARB,EXT}_geometry_shader4-only variable.
   */
   var = add_input(VARYING_SLOT_PRIMITIVE_ID, int_t, GLSL_PRECISION_HIGH,
         var->data.interpolation = INTERP_MODE_FLAT;
   var = add_output(VARYING_SLOT_PRIMITIVE_ID, int_t, GLSL_PRECISION_HIGH,
            }
         /**
   * Generate variables which only exist in fragment shaders.
   */
   void
   builtin_variable_generator::generate_fs_special_vars()
   {
               int frag_coord_precision = (state->is_version(0, 300) ?
                  if (this->state->consts->GLSLFragCoordIsSysVal) {
      add_system_value(SYSTEM_VALUE_FRAG_COORD, vec4_t, frag_coord_precision,
      } else {
                  if (this->state->consts->GLSLFrontFacingIsSysVal) {
      var = add_system_value(SYSTEM_VALUE_FRONT_FACE, bool_t, "gl_FrontFacing");
      } else {
      var = add_input(VARYING_SLOT_FACE, bool_t, "gl_FrontFacing");
               if (state->is_version(120, 100)) {
      if (this->state->consts->GLSLPointCoordIsSysVal)
      add_system_value(SYSTEM_VALUE_POINT_COORD, vec2_t,
      else
      add_input(VARYING_SLOT_PNTC, vec2_t, GLSL_PRECISION_MEDIUM,
            if (state->has_geometry_shader() || state->EXT_gpu_shader4_enable) {
      var = add_input(VARYING_SLOT_PRIMITIVE_ID, int_t, GLSL_PRECISION_HIGH,
                     /* gl_FragColor and gl_FragData were deprecated starting in desktop GLSL
   * 1.30, and were relegated to the compatibility profile in GLSL 4.20.
   * They were removed from GLSL ES 3.00.
   */
   if (compatibility || !state->is_version(420, 300)) {
      add_output(FRAG_RESULT_COLOR, vec4_t, GLSL_PRECISION_MEDIUM,
         add_output(FRAG_RESULT_DATA0,
            array(vec4_t, state->Const.MaxDrawBuffers),
            if (state->has_framebuffer_fetch() && !state->is_version(130, 300)) {
      ir_variable *const var =
      add_output(FRAG_RESULT_DATA0,
            var->data.precision = GLSL_PRECISION_MEDIUM;
   var->data.read_only = 1;
   var->data.fb_fetch_output = 1;
               if (state->has_framebuffer_fetch_zs()) {
      ir_variable *const depth_var =
      add_output(FRAG_RESULT_DEPTH, float_t,
      depth_var->data.read_only = 1;
   depth_var->data.fb_fetch_output = 1;
            ir_variable *const stencil_var =
      add_output(FRAG_RESULT_STENCIL, int_t,
      stencil_var->data.read_only = 1;
   stencil_var->data.fb_fetch_output = 1;
               if (state->es_shader && state->language_version == 100 && state->EXT_blend_func_extended_enable) {
      add_index_output(FRAG_RESULT_COLOR, 1, vec4_t,
         add_index_output(FRAG_RESULT_DATA0, 1,
                     /* gl_FragDepth has always been in desktop GLSL, but did not appear in GLSL
   * ES 1.00.
   */
   if (state->is_version(110, 300)) {
      add_output(FRAG_RESULT_DEPTH, float_t, GLSL_PRECISION_HIGH,
               if (state->EXT_frag_depth_enable)
            if (state->ARB_shader_stencil_export_enable) {
      ir_variable *const var =
         if (state->ARB_shader_stencil_export_warn)
               if (state->AMD_shader_stencil_export_enable) {
      ir_variable *const var =
         if (state->AMD_shader_stencil_export_warn)
               if (state->is_version(400, 320) ||
      state->ARB_sample_shading_enable ||
   state->OES_sample_variables_enable) {
   add_system_value(SYSTEM_VALUE_SAMPLE_ID, int_t, GLSL_PRECISION_LOW,
         add_system_value(SYSTEM_VALUE_SAMPLE_POS, vec2_t, GLSL_PRECISION_MEDIUM,
         /* From the ARB_sample_shading specification:
   *    "The number of elements in the array is ceil(<s>/32), where
   *    <s> is the maximum number of color samples supported by the
   *    implementation."
   * Since no drivers expose more than 32x MSAA, we can simply set
   * the array size to 1 rather than computing it.
   */
   add_output(FRAG_RESULT_SAMPLE_MASK, array(int_t, 1),
               if (state->is_version(400, 320) ||
      state->ARB_gpu_shader5_enable ||
   state->OES_sample_variables_enable) {
   add_system_value(SYSTEM_VALUE_SAMPLE_MASK_IN, array(int_t, 1),
               if (state->is_version(430, 320) ||
      state->ARB_fragment_layer_viewport_enable ||
   state->OES_geometry_shader_enable ||
   state->EXT_geometry_shader_enable) {
   add_varying(VARYING_SLOT_LAYER, int_t, GLSL_PRECISION_HIGH,
               if (state->is_version(430, 0) ||
      state->ARB_fragment_layer_viewport_enable ||
   state->OES_viewport_array_enable) {
               if (state->is_version(450, 310) || state->ARB_ES3_1_compatibility_enable)
      }
         /**
   * Generate variables which only exist in compute shaders.
   */
   void
   builtin_variable_generator::generate_cs_special_vars()
   {
      add_system_value(SYSTEM_VALUE_LOCAL_INVOCATION_ID, uvec3_t,
         add_system_value(SYSTEM_VALUE_WORKGROUP_ID, uvec3_t, "gl_WorkGroupID");
            if (state->ARB_compute_variable_group_size_enable) {
      add_system_value(SYSTEM_VALUE_WORKGROUP_SIZE,
               add_system_value(SYSTEM_VALUE_GLOBAL_INVOCATION_ID,
         add_system_value(SYSTEM_VALUE_LOCAL_INVOCATION_INDEX,
      }
         /**
   * Add a single "varying" variable.  The variable's type and direction (input
   * or output) are adjusted as appropriate for the type of shader being
   * compiled.
   */
   void
   builtin_variable_generator::add_varying(int slot, const glsl_type *type,
               {
      switch (state->stage) {
   case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
   case MESA_SHADER_GEOMETRY:
      this->per_vertex_in.add_field(slot, type, precision, name, interp);
      case MESA_SHADER_VERTEX:
      this->per_vertex_out.add_field(slot, type, precision, name, interp);
      case MESA_SHADER_FRAGMENT:
      add_input(slot, type, precision, name, interp);
      case MESA_SHADER_COMPUTE:
      /* Compute shaders don't have varyings. */
      default:
            }
         /**
   * Generate variables that are used to communicate data from one shader stage
   * to the next ("varyings").
   */
   void
   builtin_variable_generator::generate_varyings()
   {
      const struct gl_shader_compiler_options *options =
            /* gl_Position and gl_PointSize are not visible from fragment shaders. */
   if (state->stage != MESA_SHADER_FRAGMENT) {
      add_varying(VARYING_SLOT_POS, vec4_t, GLSL_PRECISION_HIGH, "gl_Position");
   if (!state->es_shader ||
      state->stage == MESA_SHADER_VERTEX ||
   (state->stage == MESA_SHADER_GEOMETRY &&
   (state->OES_geometry_point_size_enable ||
         ((state->stage == MESA_SHADER_TESS_CTRL ||
         (state->OES_tessellation_point_size_enable ||
         add_varying(VARYING_SLOT_PSIZ,
               float_t,
   state->is_version(0, 300) ?
      }
   if (state->stage == MESA_SHADER_VERTEX) {
      if (state->AMD_vertex_shader_viewport_index_enable ||
      state->ARB_shader_viewport_layer_array_enable ||
   state->NV_viewport_array2_enable) {
               if (state->AMD_vertex_shader_layer_enable ||
      state->ARB_shader_viewport_layer_array_enable ||
   state->NV_viewport_array2_enable) {
   add_varying(VARYING_SLOT_LAYER, int_t, GLSL_PRECISION_HIGH,
               /* From the NV_viewport_array2 specification:
   *
   *    "The variable gl_ViewportMask[] is available as an output variable
   *    in the VTG languages. The array has ceil(v/32) elements where v is
   *    the maximum number of viewports supported by the implementation."
   *
   * Since no drivers expose more than 16 viewports, we can simply set the
   * array size to 1 rather than computing it and dealing with varying
   * slot complication.
   */
   if (state->NV_viewport_array2_enable)
      add_varying(VARYING_SLOT_VIEWPORT_MASK, array(int_t, 1),
               if (state->has_clip_distance()) {
      add_varying(VARYING_SLOT_CLIP_DIST0, array(float_t, 0),
      }
   if (state->has_cull_distance()) {
      add_varying(VARYING_SLOT_CULL_DIST0, array(float_t, 0),
               if (compatibility) {
      add_varying(VARYING_SLOT_TEX0, array(vec4_t, 0), "gl_TexCoord");
   add_varying(VARYING_SLOT_FOGC, float_t, "gl_FogFragCoord");
   if (state->stage == MESA_SHADER_FRAGMENT) {
      add_varying(VARYING_SLOT_COL0, vec4_t, "gl_Color");
      } else {
      add_varying(VARYING_SLOT_CLIP_VERTEX, vec4_t, "gl_ClipVertex");
   add_varying(VARYING_SLOT_COL0, vec4_t, "gl_FrontColor");
   add_varying(VARYING_SLOT_BFC0, vec4_t, "gl_BackColor");
   add_varying(VARYING_SLOT_COL1, vec4_t, "gl_FrontSecondaryColor");
                  /* Section 7.1 (Built-In Language Variables) of the GLSL 4.00 spec
   * says:
   *
   *    "In the tessellation control language, built-in variables are
   *    intrinsically declared as:
   *
   *        in gl_PerVertex {
   *            vec4 gl_Position;
   *            float gl_PointSize;
   *            float gl_ClipDistance[];
   *        } gl_in[gl_MaxPatchVertices];"
   */
   if (state->stage == MESA_SHADER_TESS_CTRL ||
      state->stage == MESA_SHADER_TESS_EVAL) {
   const glsl_type *per_vertex_in_type =
         add_variable("gl_in", array(per_vertex_in_type, state->Const.MaxPatchVertices),
      }
   if (state->stage == MESA_SHADER_GEOMETRY) {
      const glsl_type *per_vertex_in_type =
         add_variable("gl_in", array(per_vertex_in_type, 0),
      }
   if (state->stage == MESA_SHADER_TESS_CTRL) {
      const glsl_type *per_vertex_out_type =
         add_variable("gl_out", array(per_vertex_out_type, 0),
      }
   if (state->stage == MESA_SHADER_VERTEX ||
      state->stage == MESA_SHADER_TESS_EVAL ||
   state->stage == MESA_SHADER_GEOMETRY) {
   const glsl_type *per_vertex_out_type =
         const glsl_struct_field *fields = per_vertex_out_type->fields.structure;
   for (unsigned i = 0; i < per_vertex_out_type->length; i++) {
      ir_variable *var =
      add_variable(fields[i].name, fields[i].type, fields[i].precision,
      var->data.interpolation = fields[i].interpolation;
   var->data.centroid = fields[i].centroid;
   var->data.sample = fields[i].sample;
                                 var->data.precise = fields[i].location == VARYING_SLOT_POS &&
            }
         }; /* Anonymous namespace */
         void
   _mesa_glsl_initialize_variables(exec_list *instructions,
         {
               gen.generate_constants();
   gen.generate_uniforms();
                     switch (state->stage) {
   case MESA_SHADER_VERTEX:
      gen.generate_vs_special_vars();
      case MESA_SHADER_TESS_CTRL:
      gen.generate_tcs_special_vars();
      case MESA_SHADER_TESS_EVAL:
      gen.generate_tes_special_vars();
      case MESA_SHADER_GEOMETRY:
      gen.generate_gs_special_vars();
      case MESA_SHADER_FRAGMENT:
      gen.generate_fs_special_vars();
      case MESA_SHADER_COMPUTE:
      gen.generate_cs_special_vars();
      default:
            }
