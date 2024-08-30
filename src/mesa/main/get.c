   /*
   * Copyright (C) 2010  Brian Paul   All Rights Reserved.
   * Copyright (C) 2010  Intel Corporation
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
   *
   * Author: Kristian Høgsberg <krh@bitplanet.net>
   */
      #include "util/glheader.h"
   #include "context.h"
   #include "blend.h"
   #include "debug_output.h"
   #include "enable.h"
   #include "enums.h"
   #include "errors.h"
   #include "extensions.h"
   #include "get.h"
   #include "macros.h"
   #include "multisample.h"
   #include "mtypes.h"
   #include "queryobj.h"
   #include "spirv_extensions.h"
   #include "state.h"
   #include "texcompress.h"
   #include "texstate.h"
   #include "framebuffer.h"
   #include "samplerobj.h"
   #include "stencil.h"
   #include "version.h"
      #include "state_tracker/st_context.h"
   #include "api_exec_decl.h"
      /* This is a table driven implemetation of the glGet*v() functions.
   * The basic idea is that most getters just look up an int somewhere
   * in struct gl_context and then convert it to a bool or float according to
   * which of glGetIntegerv() glGetBooleanv() etc is being called.
   * Instead of generating code to do this, we can just record the enum
   * value and the offset into struct gl_context in an array of structs.  Then
   * in glGet*(), we lookup the struct for the enum in question, and use
   * the offset to get the int we need.
   *
   * Sometimes we need to look up a float, a boolean, a bit in a
   * bitfield, a matrix or other types instead, so we need to track the
   * type of the value in struct gl_context.  And sometimes the value isn't in
   * struct gl_context but in the drawbuffer, the array object, current texture
   * unit, or maybe it's a computed value.  So we need to also track
   * where or how to find the value.  Finally, we sometimes need to
   * check that one of a number of extensions are enabled, the GL
   * version or flush or call _mesa_update_state().  This is done by
   * attaching optional extra information to the value description
   * struct, it's sort of like an array of opcodes that describe extra
   * checks or actions.
   *
   * Putting all this together we end up with struct value_desc below,
   * and with a couple of macros to help, the table of struct value_desc
   * is about as concise as the specification in the old python script.
   */
      static inline GLboolean
   FLOAT_TO_BOOLEAN(GLfloat X)
   {
         }
      static inline GLint
   FLOAT_TO_FIXED(GLfloat F)
   {
      return ( ((F) * 65536.0f > (float)INT32_MAX) ? INT32_MAX :
            }
      static inline GLboolean
   INT_TO_BOOLEAN(GLint I)
   {
         }
      static inline GLfixed
   INT_TO_FIXED(GLint I)
   {
      return (((I) > SHRT_MAX) ? INT_MAX :
            }
         static inline GLboolean
   INT64_TO_BOOLEAN(GLint64 I)
   {
         }
      static inline GLint
   INT64_TO_INT(GLint64 I)
   {
         }
      static inline GLint
   BOOLEAN_TO_INT(GLboolean B)
   {
         }
      static inline GLfloat
   BOOLEAN_TO_FLOAT(GLboolean B)
   {
         }
      static inline GLfixed
   BOOLEAN_TO_FIXED(GLboolean B)
   {
         }
      enum value_type {
      TYPE_INVALID,
   TYPE_INT,
   TYPE_INT_2,
   TYPE_INT_3,
   TYPE_INT_4,
   TYPE_INT_N,
   TYPE_UINT,
   TYPE_UINT_2,
   TYPE_UINT_3,
   TYPE_UINT_4,
   TYPE_INT64,
   TYPE_ENUM16,
   TYPE_ENUM,
   TYPE_ENUM_2,
   TYPE_BOOLEAN,
   TYPE_UBYTE,
   TYPE_SHORT,
   TYPE_BIT_0,
   TYPE_BIT_1,
   TYPE_BIT_2,
   TYPE_BIT_3,
   TYPE_BIT_4,
   TYPE_BIT_5,
   TYPE_BIT_6,
   TYPE_BIT_7,
   TYPE_FLOAT,
   TYPE_FLOAT_2,
   TYPE_FLOAT_3,
   TYPE_FLOAT_4,
   TYPE_FLOAT_8,
   TYPE_FLOATN,
   TYPE_FLOATN_2,
   TYPE_FLOATN_3,
   TYPE_FLOATN_4,
   TYPE_DOUBLEN,
   TYPE_DOUBLEN_2,
   TYPE_MATRIX,
   TYPE_MATRIX_T,
      };
      enum value_location {
      LOC_BUFFER,
   LOC_CONTEXT,
   LOC_ARRAY,
   LOC_TEXUNIT,
      };
      enum value_extra {
      EXTRA_END = 0x8000,
   EXTRA_VERSION_30,
   EXTRA_VERSION_31,
   EXTRA_VERSION_32,
   EXTRA_VERSION_40,
   EXTRA_VERSION_43,
   EXTRA_API_GL,
   EXTRA_API_GL_CORE,
   EXTRA_API_GL_COMPAT,
   EXTRA_API_ES,
   EXTRA_API_ES2,
   EXTRA_API_ES3,
   EXTRA_API_ES31,
   EXTRA_API_ES32,
   EXTRA_NEW_BUFFERS,
   EXTRA_VALID_DRAW_BUFFER,
   EXTRA_VALID_TEXTURE_UNIT,
   EXTRA_VALID_CLIP_DISTANCE,
   EXTRA_FLUSH_CURRENT,
   EXTRA_GLSL_130,
   EXTRA_EXT_UBO_GS,
   EXTRA_EXT_ATOMICS_GS,
   EXTRA_EXT_SHADER_IMAGE_GS,
   EXTRA_EXT_ATOMICS_TESS,
   EXTRA_EXT_SHADER_IMAGE_TESS,
   EXTRA_EXT_SSBO_GS,
   EXTRA_EXT_FB_NO_ATTACH_GS,
   EXTRA_EXT_ES_GS,
      };
      #define NO_EXTRA NULL
   #define NO_OFFSET 0
      struct value_desc {
      GLenum pname;
   GLubyte location;  /**< enum value_location */
   GLubyte type;      /**< enum value_type */
   int offset;
      };
      union value {
      GLfloat value_float;
   GLfloat value_float_4[4];
   GLdouble value_double_2[2];
   GLmatrix *value_matrix;
   GLint value_int;
   GLint value_int_2[2];
   GLint value_int_4[4];
   GLint64 value_int64;
   GLenum value_enum;
   GLenum16 value_enum16;
   GLubyte value_ubyte;
   GLshort value_short;
            /* Sigh, see GL_COMPRESSED_TEXTURE_FORMATS_ARB handling */
   struct {
         } value_int_n;
      };
      #define BUFFER_FIELD(field, type) \
         #define CONTEXT_FIELD(field, type) \
         #define ARRAY_FIELD(field, type) \
         #undef CONST /* already defined through windows.h */
   #define CONST(value) \
            #define BUFFER_INT(field) BUFFER_FIELD(field, TYPE_INT)
   #define BUFFER_ENUM(field) BUFFER_FIELD(field, TYPE_ENUM)
   #define BUFFER_ENUM16(field) BUFFER_FIELD(field, TYPE_ENUM16)
   #define BUFFER_BOOL(field) BUFFER_FIELD(field, TYPE_BOOLEAN)
      #define CONTEXT_INT(field) CONTEXT_FIELD(field, TYPE_INT)
   #define CONTEXT_INT2(field) CONTEXT_FIELD(field, TYPE_INT_2)
   #define CONTEXT_INT64(field) CONTEXT_FIELD(field, TYPE_INT64)
   #define CONTEXT_UINT(field) CONTEXT_FIELD(field, TYPE_UINT)
   #define CONTEXT_ENUM16(field) CONTEXT_FIELD(field, TYPE_ENUM16)
   #define CONTEXT_ENUM(field) CONTEXT_FIELD(field, TYPE_ENUM)
   #define CONTEXT_ENUM2(field) CONTEXT_FIELD(field, TYPE_ENUM_2)
   #define CONTEXT_BOOL(field) CONTEXT_FIELD(field, TYPE_BOOLEAN)
   #define CONTEXT_BIT0(field) CONTEXT_FIELD(field, TYPE_BIT_0)
   #define CONTEXT_BIT1(field) CONTEXT_FIELD(field, TYPE_BIT_1)
   #define CONTEXT_BIT2(field) CONTEXT_FIELD(field, TYPE_BIT_2)
   #define CONTEXT_BIT3(field) CONTEXT_FIELD(field, TYPE_BIT_3)
   #define CONTEXT_BIT4(field) CONTEXT_FIELD(field, TYPE_BIT_4)
   #define CONTEXT_BIT5(field) CONTEXT_FIELD(field, TYPE_BIT_5)
   #define CONTEXT_BIT6(field) CONTEXT_FIELD(field, TYPE_BIT_6)
   #define CONTEXT_BIT7(field) CONTEXT_FIELD(field, TYPE_BIT_7)
   #define CONTEXT_FLOAT(field) CONTEXT_FIELD(field, TYPE_FLOAT)
   #define CONTEXT_FLOAT2(field) CONTEXT_FIELD(field, TYPE_FLOAT_2)
   #define CONTEXT_FLOAT3(field) CONTEXT_FIELD(field, TYPE_FLOAT_3)
   #define CONTEXT_FLOAT4(field) CONTEXT_FIELD(field, TYPE_FLOAT_4)
   #define CONTEXT_FLOAT8(field) CONTEXT_FIELD(field, TYPE_FLOAT_8)
   #define CONTEXT_MATRIX(field) CONTEXT_FIELD(field, TYPE_MATRIX)
   #define CONTEXT_MATRIX_T(field) CONTEXT_FIELD(field, TYPE_MATRIX_T)
      /* Vertex array fields */
   #define ARRAY_INT(field) ARRAY_FIELD(field, TYPE_INT)
   #define ARRAY_ENUM(field) ARRAY_FIELD(field, TYPE_ENUM)
   #define ARRAY_ENUM16(field) ARRAY_FIELD(field, TYPE_ENUM16)
   #define ARRAY_BOOL(field) ARRAY_FIELD(field, TYPE_BOOLEAN)
   #define ARRAY_UBYTE(field) ARRAY_FIELD(field, TYPE_UBYTE)
   #define ARRAY_SHORT(field) ARRAY_FIELD(field, TYPE_SHORT)
      #define EXT(f)					\
            #define EXTRA_EXT(e)				\
      static const int extra_##e[] = {		\
               #define EXTRA_EXT2(e1, e2)			\
      static const int extra_##e1##_##e2[] = {	\
               /* The 'extra' mechanism is a way to specify extra checks (such as
   * extensions or specific gl versions) or actions (flush current, new
   * buffers) that we need to do before looking up an enum.  We need to
   * declare them all up front so we can refer to them in the value_desc
   * structs below.
   *
   * Each EXTRA_ will be executed.  For EXTRA_* enums of extensions and API
   * versions, listing multiple ones in an array means an error will be thrown
   * only if none of them are available.  If you need to check for "AND"
   * behavior, you would need to make a custom EXTRA_ enum.
   */
      static const int extra_new_buffers_compat_es[] = {
      EXTRA_API_GL_COMPAT,
   EXTRA_API_ES,
   EXTRA_NEW_BUFFERS,
      };
      static const int extra_new_buffers[] = {
      EXTRA_NEW_BUFFERS,
      };
      static const int extra_valid_draw_buffer[] = {
      EXTRA_VALID_DRAW_BUFFER,
      };
      static const int extra_valid_texture_unit[] = {
      EXTRA_VALID_TEXTURE_UNIT,
      };
      static const int extra_valid_clip_distance[] = {
      EXTRA_VALID_CLIP_DISTANCE,
      };
      static const int extra_flush_current_valid_texture_unit[] = {
      EXTRA_FLUSH_CURRENT,
   EXTRA_VALID_TEXTURE_UNIT,
      };
      static const int extra_flush_current[] = {
      EXTRA_FLUSH_CURRENT,
      };
      static const int extra_EXT_texture_integer_and_new_buffers[] = {
      EXT(EXT_texture_integer),
   EXTRA_NEW_BUFFERS,
      };
      static const int extra_GLSL_130_es3_gpushader4[] = {
      EXTRA_GLSL_130,
   EXTRA_API_ES3,
   EXT(EXT_gpu_shader4),
      };
      static const int extra_texture_buffer_object[] = {
      EXT(ARB_texture_buffer_object),
      };
      static const int extra_ARB_transform_feedback2_api_es3[] = {
      EXT(ARB_transform_feedback2),
   EXTRA_API_ES3,
      };
      static const int extra_ARB_uniform_buffer_object_and_geometry_shader[] = {
      EXTRA_EXT_UBO_GS,
      };
      static const int extra_ARB_ES2_compatibility_api_es2[] = {
      EXT(ARB_ES2_compatibility),
   EXTRA_API_ES2,
      };
      static const int extra_ARB_ES3_compatibility_api_es3[] = {
      EXT(ARB_ES3_compatibility),
   EXTRA_API_ES3,
      };
      static const int extra_EXT_framebuffer_sRGB_and_new_buffers[] = {
      EXT(EXT_framebuffer_sRGB),
   EXTRA_NEW_BUFFERS,
      };
      static const int extra_EXT_packed_float[] = {
      EXT(EXT_packed_float),
   EXTRA_NEW_BUFFERS,
      };
      static const int extra_EXT_texture_array_es3[] = {
      EXT(EXT_texture_array),
   EXTRA_API_ES3,
      };
      static const int extra_ARB_shader_atomic_counters_and_geometry_shader[] = {
      EXTRA_EXT_ATOMICS_GS,
      };
      static const int extra_ARB_shader_atomic_counters_es31[] = {
      EXT(ARB_shader_atomic_counters),
   EXTRA_API_ES31,
      };
      static const int extra_ARB_shader_image_load_store_and_geometry_shader[] = {
      EXTRA_EXT_SHADER_IMAGE_GS,
      };
      static const int extra_ARB_shader_atomic_counters_and_tessellation[] = {
      EXTRA_EXT_ATOMICS_TESS,
      };
      static const int extra_ARB_shader_image_load_store_and_tessellation[] = {
      EXTRA_EXT_SHADER_IMAGE_TESS,
      };
      static const int extra_ARB_shader_image_load_store_es31[] = {
      EXT(ARB_shader_image_load_store),
   EXTRA_API_ES31,
      };
      /* HACK: remove when ARB_compute_shader is actually supported */
   static const int extra_ARB_compute_shader_es31[] = {
      EXT(ARB_compute_shader),
   EXTRA_API_ES31,
      };
      static const int extra_ARB_shader_storage_buffer_object_es31[] = {
      EXT(ARB_shader_storage_buffer_object),
   EXTRA_API_ES31,
      };
      static const int extra_ARB_shader_storage_buffer_object_and_geometry_shader[] = {
      EXTRA_EXT_SSBO_GS,
      };
      static const int extra_ARB_shader_image_load_store_shader_storage_buffer_object_es31[] = {
      EXT(ARB_shader_image_load_store),
   EXT(ARB_shader_storage_buffer_object),
   EXTRA_API_ES31,
      };
      static const int extra_ARB_framebuffer_no_attachments_and_geometry_shader[] = {
      EXTRA_EXT_FB_NO_ATTACH_GS,
      };
      static const int extra_ARB_viewport_array_or_oes_geometry_shader[] = {
      EXT(ARB_viewport_array),
   EXTRA_EXT_ES_GS,
      };
      static const int extra_ARB_viewport_array_or_oes_viewport_array[] = {
      EXT(ARB_viewport_array),
   EXT(OES_viewport_array),
      };
      static const int extra_ARB_gpu_shader5_or_oes_geometry_shader[] = {
      EXT(ARB_gpu_shader5),
   EXTRA_EXT_ES_GS,
      };
      static const int extra_ARB_gpu_shader5_or_OES_sample_variables[] = {
      EXT(ARB_gpu_shader5),
   EXT(OES_sample_variables),
      };
      static const int extra_ES32[] = {
      EXT(ARB_ES3_2_compatibility),
   EXTRA_API_ES32,
      };
      static const int extra_KHR_robustness_or_GL[] = {
      EXT(KHR_robustness),
   EXTRA_API_GL,
   EXTRA_API_GL_CORE,
      };
      static const int extra_INTEL_conservative_rasterization[] = {
      EXT(INTEL_conservative_rasterization),
      };
      static const int extra_ARB_timer_query_or_EXT_disjoint_timer_query[] = {
      EXT(ARB_timer_query),
   EXT(EXT_disjoint_timer_query),
      };
      static const int extra_ARB_framebuffer_object_or_EXT_framebuffer_multisample_or_EXT_multisampled_render_to_texture[] = {
      EXT(ARB_framebuffer_object),
   EXT(EXT_framebuffer_multisample),
   EXT(EXT_multisampled_render_to_texture),
      };
      EXTRA_EXT(EXT_texture_array);
   EXTRA_EXT(NV_fog_distance);
   EXTRA_EXT(EXT_texture_filter_anisotropic);
   EXTRA_EXT(NV_texture_rectangle);
   EXTRA_EXT(EXT_stencil_two_side);
   EXTRA_EXT(EXT_depth_bounds_test);
   EXTRA_EXT(ARB_depth_clamp);
   EXTRA_EXT(AMD_depth_clamp_separate);
   EXTRA_EXT(ATI_fragment_shader);
   EXTRA_EXT(EXT_provoking_vertex);
   EXTRA_EXT(ARB_fragment_shader);
   EXTRA_EXT(ARB_fragment_program);
   EXTRA_EXT(ARB_seamless_cube_map);
   EXTRA_EXT(ARB_sync);
   EXTRA_EXT(ARB_vertex_shader);
   EXTRA_EXT(EXT_transform_feedback);
   EXTRA_EXT(ARB_transform_feedback3);
   EXTRA_EXT(ARB_vertex_program);
   EXTRA_EXT2(ARB_vertex_program, ARB_fragment_program);
   EXTRA_EXT(ARB_color_buffer_float);
   EXTRA_EXT(EXT_framebuffer_sRGB);
   EXTRA_EXT(OES_EGL_image_external);
   EXTRA_EXT(ARB_blend_func_extended);
   EXTRA_EXT(ARB_uniform_buffer_object);
   EXTRA_EXT2(ARB_texture_cube_map_array, OES_texture_cube_map_array);
   EXTRA_EXT(ARB_texture_buffer_range);
   EXTRA_EXT(ARB_texture_multisample);
   EXTRA_EXT(ARB_texture_gather);
   EXTRA_EXT(ARB_draw_indirect);
   EXTRA_EXT(ARB_shader_image_load_store);
   EXTRA_EXT(ARB_query_buffer_object);
   EXTRA_EXT2(ARB_transform_feedback3, ARB_gpu_shader5);
   EXTRA_EXT(INTEL_performance_query);
   EXTRA_EXT(ARB_explicit_uniform_location);
   EXTRA_EXT(ARB_clip_control);
   EXTRA_EXT(ARB_polygon_offset_clamp);
   EXTRA_EXT(ARB_framebuffer_no_attachments);
   EXTRA_EXT(ARB_tessellation_shader);
   EXTRA_EXT(ARB_shader_storage_buffer_object);
   EXTRA_EXT(ARB_indirect_parameters);
   EXTRA_EXT(ATI_meminfo);
   EXTRA_EXT(NVX_gpu_memory_info);
   EXTRA_EXT(ARB_cull_distance);
   EXTRA_EXT(EXT_window_rectangles);
   EXTRA_EXT(KHR_blend_equation_advanced_coherent);
   EXTRA_EXT(OES_primitive_bounding_box);
   EXTRA_EXT(ARB_compute_variable_group_size);
   EXTRA_EXT(KHR_robustness);
   EXTRA_EXT(ARB_sparse_buffer);
   EXTRA_EXT(NV_conservative_raster);
   EXTRA_EXT(NV_conservative_raster_dilate);
   EXTRA_EXT(NV_conservative_raster_pre_snap_triangles);
   EXTRA_EXT(ARB_sample_locations);
   EXTRA_EXT(AMD_framebuffer_multisample_advanced);
   EXTRA_EXT(ARB_spirv_extensions);
   EXTRA_EXT(NV_viewport_swizzle);
   EXTRA_EXT(ARB_sparse_texture);
      static const int extra_ARB_gl_spirv_or_es2_compat[] = {
      EXT(ARB_gl_spirv),
   EXT(ARB_ES2_compatibility),
   EXTRA_API_ES2,
      };
      static const int
   extra_ARB_color_buffer_float_or_glcore[] = {
      EXT(ARB_color_buffer_float),
   EXTRA_API_GL_CORE,
      };
      static const int
   extra_NV_primitive_restart[] = {
      EXT(NV_primitive_restart),
      };
      static const int extra_version_30[] = { EXTRA_VERSION_30, EXTRA_END };
   static const int extra_version_31[] = { EXTRA_VERSION_31, EXTRA_END };
   static const int extra_version_32[] = { EXTRA_VERSION_32, EXTRA_END };
   static const int extra_version_43[] = { EXTRA_VERSION_43, EXTRA_END };
      static const int extra_gl30_es3[] = {
      EXTRA_VERSION_30,
   EXTRA_API_ES3,
      };
      static const int extra_gl32_es3[] = {
      EXTRA_VERSION_32,
   EXTRA_API_ES3,
      };
      static const int extra_version_32_OES_geometry_shader[] = {
      EXTRA_VERSION_32,
   EXTRA_EXT_ES_GS,
      };
      static const int extra_gl40_ARB_sample_shading[] = {
      EXTRA_VERSION_40,
   EXT(ARB_sample_shading),
      };
      static const int
   extra_ARB_vertex_program_api_es2[] = {
      EXT(ARB_vertex_program),
   EXTRA_API_ES2,
      };
      /* The ReadBuffer get token is valid under either full GL or under
   * GLES2 if the NV_read_buffer extension is available. */
   static const int
   extra_NV_read_buffer_api_gl[] = {
      EXTRA_API_ES2,
   EXTRA_API_GL,
      };
      static const int extra_core_ARB_color_buffer_float_and_new_buffers[] = {
      EXTRA_API_GL_CORE,
   EXT(ARB_color_buffer_float),
   EXTRA_NEW_BUFFERS,
      };
      static const int extra_EXT_shader_framebuffer_fetch[] = {
      EXTRA_API_ES2,
   EXTRA_API_ES3,
   EXT(EXT_shader_framebuffer_fetch),
      };
      static const int extra_EXT_provoking_vertex_32[] = {
      EXTRA_EXT_PROVOKING_VERTEX_32,
      };
      static const int extra_EXT_disjoint_timer_query[] = {
      EXTRA_API_ES2,
   EXTRA_API_ES3,
   EXT(EXT_disjoint_timer_query),
      };
         /* This is the big table describing all the enums we accept in
   * glGet*v().  The table is partitioned into six parts: enums
   * understood by all GL APIs (OpenGL, GLES and GLES2), enums shared
   * between OpenGL and GLES, enums exclusive to GLES, etc for the
   * remaining combinations. To look up the enums valid in a given API
   * we will use a hash table specific to that API. These tables are in
   * turn generated at build time and included through get_hash.h.
   */
      #include "get_hash.h"
      /* All we need now is a way to look up the value struct from the enum.
   * The code generated by gcc for the old generated big switch
   * statement is a big, balanced, open coded if/else tree, essentially
   * an unrolled binary search.  It would be natural to sort the new
   * enum table and use bsearch(), but we will use a read-only hash
   * table instead.  bsearch() has a nice guaranteed worst case
   * performance, but we're also guaranteed to hit that worst case
   * (log2(n) iterations) for about half the enums.  Instead, using an
   * open addressing hash table, we can find the enum on the first try
   * for 80% of the enums, 1 collision for 10% and never more than 5
   * collisions for any enum (typical numbers).  And the code is very
   * simple, even though it feels a little magic. */
      /**
   * Handle irregular enums
   *
   * Some values don't conform to the "well-known type at context
   * pointer + offset" pattern, so we have this function to catch all
   * the corner cases.  Typically, it's a computed value or a one-off
   * pointer to a custom struct or something.
   *
   * In this case we can't return a pointer to the value, so we'll have
   * to use the temporary variable 'v' declared back in the calling
   * glGet*v() function to store the result.
   *
   * \param ctx the current context
   * \param d the struct value_desc that describes the enum
   * \param v pointer to the tmp declared in the calling glGet*v() function
   */
   static void
   find_custom_value(struct gl_context *ctx, const struct value_desc *d, union value *v)
   {
      struct gl_buffer_object **buffer_obj, *buf;
   struct gl_array_attributes *array;
            switch (d->pname) {
   case GL_MAJOR_VERSION:
      v->value_int = ctx->Version / 10;
      case GL_MINOR_VERSION:
      v->value_int = ctx->Version % 10;
         case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_EXTERNAL_OES:
      v->value_bool = _mesa_IsEnabled(d->pname);
         case GL_LINE_STIPPLE_PATTERN:
      /* This is the only GLushort, special case it here by promoting
   * to an int rather than introducing a new type. */
   v->value_int = ctx->Line.StipplePattern;
         case GL_CURRENT_RASTER_TEXTURE_COORDS:
      unit = ctx->Texture.CurrentUnit;
   v->value_float_4[0] = ctx->Current.RasterTexCoords[unit][0];
   v->value_float_4[1] = ctx->Current.RasterTexCoords[unit][1];
   v->value_float_4[2] = ctx->Current.RasterTexCoords[unit][2];
   v->value_float_4[3] = ctx->Current.RasterTexCoords[unit][3];
         case GL_CURRENT_TEXTURE_COORDS:
      unit = ctx->Texture.CurrentUnit;
   v->value_float_4[0] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][0];
   v->value_float_4[1] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][1];
   v->value_float_4[2] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][2];
   v->value_float_4[3] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][3];
         case GL_COLOR_WRITEMASK:
      v->value_int_4[0] = GET_COLORMASK_BIT(ctx->Color.ColorMask, 0, 0);
   v->value_int_4[1] = GET_COLORMASK_BIT(ctx->Color.ColorMask, 0, 1);
   v->value_int_4[2] = GET_COLORMASK_BIT(ctx->Color.ColorMask, 0, 2);
   v->value_int_4[3] = GET_COLORMASK_BIT(ctx->Color.ColorMask, 0, 3);
         case GL_DEPTH_CLAMP:
      v->value_bool = ctx->Transform.DepthClampNear || ctx->Transform.DepthClampFar;
         case GL_EDGE_FLAG:
      v->value_bool = ctx->Current.Attrib[VERT_ATTRIB_EDGEFLAG][0] == 1.0F;
         case GL_READ_BUFFER:
      v->value_enum16 = ctx->ReadBuffer->ColorReadBuffer;
         case GL_MAP2_GRID_DOMAIN:
      v->value_float_4[0] = ctx->Eval.MapGrid2u1;
   v->value_float_4[1] = ctx->Eval.MapGrid2u2;
   v->value_float_4[2] = ctx->Eval.MapGrid2v1;
   v->value_float_4[3] = ctx->Eval.MapGrid2v2;
         case GL_TEXTURE_STACK_DEPTH:
      unit = ctx->Texture.CurrentUnit;
   v->value_int = ctx->TextureMatrixStack[unit].Depth + 1;
      case GL_TEXTURE_MATRIX:
      unit = ctx->Texture.CurrentUnit;
   v->value_matrix = ctx->TextureMatrixStack[unit].Top;
         case GL_VERTEX_ARRAY:
      v->value_bool = !!(ctx->Array.VAO->Enabled & VERT_BIT_POS);
      case GL_NORMAL_ARRAY:
      v->value_bool = !!(ctx->Array.VAO->Enabled & VERT_BIT_NORMAL);
      case GL_COLOR_ARRAY:
      v->value_bool = !!(ctx->Array.VAO->Enabled & VERT_BIT_COLOR0);
      case GL_TEXTURE_COORD_ARRAY:
      v->value_bool = !!(ctx->Array.VAO->Enabled & VERT_BIT_TEX(ctx->Array.ActiveTexture));
      case GL_INDEX_ARRAY:
      v->value_bool = !!(ctx->Array.VAO->Enabled & VERT_BIT_COLOR_INDEX);
      case GL_EDGE_FLAG_ARRAY:
      v->value_bool = !!(ctx->Array.VAO->Enabled & VERT_BIT_EDGEFLAG);
      case GL_SECONDARY_COLOR_ARRAY:
      v->value_bool = !!(ctx->Array.VAO->Enabled & VERT_BIT_COLOR1);
      case GL_FOG_COORDINATE_ARRAY:
      v->value_bool = !!(ctx->Array.VAO->Enabled & VERT_BIT_FOG);
      case GL_POINT_SIZE_ARRAY_OES:
      v->value_bool = !!(ctx->Array.VAO->Enabled & VERT_BIT_POINT_SIZE);
         case GL_TEXTURE_COORD_ARRAY_TYPE:
   case GL_TEXTURE_COORD_ARRAY_STRIDE:
      array = &ctx->Array.VAO->VertexAttrib[VERT_ATTRIB_TEX(ctx->Array.ActiveTexture)];
   v->value_int = *(GLuint *) ((char *) array + d->offset);
         case GL_TEXTURE_COORD_ARRAY_SIZE:
      array = &ctx->Array.VAO->VertexAttrib[VERT_ATTRIB_TEX(ctx->Array.ActiveTexture)];
   v->value_int = array->Format.User.Size;
         case GL_VERTEX_ARRAY_SIZE:
      array = &ctx->Array.VAO->VertexAttrib[VERT_ATTRIB_POS];
   v->value_int = array->Format.User.Size;
         case GL_ACTIVE_TEXTURE_ARB:
      v->value_int = GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit;
      case GL_CLIENT_ACTIVE_TEXTURE_ARB:
      v->value_int = GL_TEXTURE0_ARB + ctx->Array.ActiveTexture;
         case GL_MODELVIEW_STACK_DEPTH:
   case GL_PROJECTION_STACK_DEPTH:
      v->value_int = *(GLint *) ((char *) ctx + d->offset) + 1;
         case GL_MAX_TEXTURE_SIZE:
   case GL_MAX_3D_TEXTURE_SIZE:
   case GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB:
      p = (GLuint *) ((char *) ctx + d->offset);
   v->value_int = 1 << (*p - 1);
         case GL_SCISSOR_BOX:
      v->value_int_4[0] = ctx->Scissor.ScissorArray[0].X;
   v->value_int_4[1] = ctx->Scissor.ScissorArray[0].Y;
   v->value_int_4[2] = ctx->Scissor.ScissorArray[0].Width;
   v->value_int_4[3] = ctx->Scissor.ScissorArray[0].Height;
         case GL_SCISSOR_TEST:
      v->value_bool = ctx->Scissor.EnableFlags & 1;
         case GL_LIST_INDEX:
      v->value_int =
            case GL_LIST_MODE:
      if (!ctx->CompileFlag)
         else if (ctx->ExecuteFlag)
         else
               case GL_VIEWPORT:
      v->value_float_4[0] = ctx->ViewportArray[0].X;
   v->value_float_4[1] = ctx->ViewportArray[0].Y;
   v->value_float_4[2] = ctx->ViewportArray[0].Width;
   v->value_float_4[3] = ctx->ViewportArray[0].Height;
         case GL_DEPTH_RANGE:
      v->value_double_2[0] = ctx->ViewportArray[0].Near;
   v->value_double_2[1] = ctx->ViewportArray[0].Far;
         case GL_ACTIVE_STENCIL_FACE_EXT:
      v->value_enum16 = ctx->Stencil.ActiveFace ? GL_BACK : GL_FRONT;
         case GL_STENCIL_FAIL:
      v->value_enum16 = ctx->Stencil.FailFunc[ctx->Stencil.ActiveFace];
      case GL_STENCIL_FUNC:
      v->value_enum16 = ctx->Stencil.Function[ctx->Stencil.ActiveFace];
      case GL_STENCIL_PASS_DEPTH_FAIL:
      v->value_enum16 = ctx->Stencil.ZFailFunc[ctx->Stencil.ActiveFace];
      case GL_STENCIL_PASS_DEPTH_PASS:
      v->value_enum16 = ctx->Stencil.ZPassFunc[ctx->Stencil.ActiveFace];
      case GL_STENCIL_REF:
      v->value_int = _mesa_get_stencil_ref(ctx, ctx->Stencil.ActiveFace);
      case GL_STENCIL_BACK_REF:
      v->value_int = _mesa_get_stencil_ref(ctx, 1);
      case GL_STENCIL_VALUE_MASK:
      v->value_int = ctx->Stencil.ValueMask[ctx->Stencil.ActiveFace];
      case GL_STENCIL_WRITEMASK:
      v->value_int = ctx->Stencil.WriteMask[ctx->Stencil.ActiveFace];
         case GL_NUM_EXTENSIONS:
      v->value_int = _mesa_get_extension_count(ctx);
         case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
      v->value_int = _mesa_get_color_read_type(ctx, NULL, "glGetIntegerv");
      case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
      v->value_int = _mesa_get_color_read_format(ctx, NULL, "glGetIntegerv");
         case GL_CURRENT_MATRIX_STACK_DEPTH_ARB:
      v->value_int = ctx->CurrentStack->Depth + 1;
      case GL_CURRENT_MATRIX_ARB:
   case GL_TRANSPOSE_CURRENT_MATRIX_ARB:
      v->value_matrix = ctx->CurrentStack->Top;
         case GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB:
      v->value_int = _mesa_get_compressed_formats(ctx, NULL);
      case GL_COMPRESSED_TEXTURE_FORMATS_ARB:
      v->value_int_n.n =
         assert(v->value_int_n.n <= (int) ARRAY_SIZE(v->value_int_n.ints));
         case GL_MAX_VARYING_FLOATS_ARB:
      v->value_int = ctx->Const.MaxVarying * 4;
                  case GL_TEXTURE_BINDING_1D:
   case GL_TEXTURE_BINDING_2D:
   case GL_TEXTURE_BINDING_3D:
   case GL_TEXTURE_BINDING_1D_ARRAY_EXT:
   case GL_TEXTURE_BINDING_2D_ARRAY_EXT:
   case GL_TEXTURE_BINDING_CUBE_MAP_ARB:
   case GL_TEXTURE_BINDING_RECTANGLE_NV:
   case GL_TEXTURE_BINDING_EXTERNAL_OES:
   case GL_TEXTURE_BINDING_CUBE_MAP_ARRAY:
   case GL_TEXTURE_BINDING_2D_MULTISAMPLE:
   case GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY:
      unit = ctx->Texture.CurrentUnit;
   v->value_int =
               /* GL_EXT_external_objects */
   case GL_NUM_DEVICE_UUIDS_EXT:
      v->value_int = 1;
      case GL_DRIVER_UUID_EXT:
      _mesa_get_driver_uuid(ctx, v->value_int_4);
      case GL_DEVICE_UUID_EXT:
      _mesa_get_device_uuid(ctx, v->value_int_4);
         /* GL_EXT_memory_object_win32 */
   case GL_DEVICE_LUID_EXT:
      _mesa_get_device_luid(ctx, v->value_int_2);
      case GL_DEVICE_NODE_MASK_EXT:
      v->value_int = ctx->pipe->screen->get_device_node_mask(ctx->pipe->screen);
         /* GL_EXT_packed_float */
   case GL_RGBA_SIGNED_COMPONENTS_EXT:
      {
      /* Note: we only check the 0th color attachment. */
   const struct gl_renderbuffer *rb =
         if (rb && _mesa_is_format_signed(rb->Format)) {
      /* Issue 17 of GL_EXT_packed_float:  If a component (such as
   * alpha) has zero bits, the component should not be considered
   * signed and so the bit for the respective component should be
   * zeroed.
   */
   GLint r_bits =
         GLint g_bits =
         GLint b_bits =
         GLint a_bits =
         GLint l_bits =
                        v->value_int_4[0] = r_bits + l_bits + i_bits > 0;
   v->value_int_4[1] = g_bits + l_bits + i_bits > 0;
   v->value_int_4[2] = b_bits + l_bits + i_bits > 0;
      }
   else {
      v->value_int_4[0] =
   v->value_int_4[1] =
   v->value_int_4[2] =
         }
         /* GL_ARB_vertex_buffer_object */
   case GL_VERTEX_ARRAY_BUFFER_BINDING_ARB:
   case GL_NORMAL_ARRAY_BUFFER_BINDING_ARB:
   case GL_COLOR_ARRAY_BUFFER_BINDING_ARB:
   case GL_INDEX_ARRAY_BUFFER_BINDING_ARB:
   case GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB:
   case GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB:
   case GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB:
      buffer_obj = (struct gl_buffer_object **)
         v->value_int = (*buffer_obj) ? (*buffer_obj)->Name : 0;
      case GL_ARRAY_BUFFER_BINDING_ARB:
      buf = ctx->Array.ArrayBufferObj;
   v->value_int = buf ? buf->Name : 0;
      case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB:
      buf = ctx->Array.VAO->BufferBinding[VERT_ATTRIB_TEX(ctx->Array.ActiveTexture)].BufferObj;
   v->value_int = buf ? buf->Name : 0;
      case GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB:
      buf = ctx->Array.VAO->IndexBufferObj;
   v->value_int = buf ? buf->Name : 0;
         /* ARB_vertex_array_bgra */
   case GL_COLOR_ARRAY_SIZE:
      array = &ctx->Array.VAO->VertexAttrib[VERT_ATTRIB_COLOR0];
   v->value_int = array->Format.User.Bgra ? GL_BGRA : array->Format.User.Size;
      case GL_SECONDARY_COLOR_ARRAY_SIZE:
      array = &ctx->Array.VAO->VertexAttrib[VERT_ATTRIB_COLOR1];
   v->value_int = array->Format.User.Bgra ? GL_BGRA : array->Format.User.Size;
         /* ARB_copy_buffer */
   case GL_COPY_READ_BUFFER:
      v->value_int = ctx->CopyReadBuffer ? ctx->CopyReadBuffer->Name : 0;
      case GL_COPY_WRITE_BUFFER:
      v->value_int = ctx->CopyWriteBuffer ? ctx->CopyWriteBuffer->Name : 0;
         case GL_PIXEL_PACK_BUFFER_BINDING_EXT:
      v->value_int = ctx->Pack.BufferObj ? ctx->Pack.BufferObj->Name : 0;
      case GL_PIXEL_UNPACK_BUFFER_BINDING_EXT:
      v->value_int = ctx->Unpack.BufferObj ? ctx->Unpack.BufferObj->Name : 0;
      case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
      v->value_int = ctx->TransformFeedback.CurrentBuffer ?
            case GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED:
      v->value_int = ctx->TransformFeedback.CurrentObject->Paused;
      case GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE:
      v->value_int = ctx->TransformFeedback.CurrentObject->Active;
      case GL_TRANSFORM_FEEDBACK_BINDING:
      v->value_int = ctx->TransformFeedback.CurrentObject->Name;
      case GL_CURRENT_PROGRAM:
      /* The Changelog of the ARB_separate_shader_objects spec says:
   *
   * 24 25 Jul 2011  pbrown  Remove the language erroneously deleting
   *                         CURRENT_PROGRAM.  In the EXT extension, this
   *                         token was aliased to ACTIVE_PROGRAM_EXT, and
   *                         was used to indicate the last program set by
   *                         either ActiveProgramEXT or UseProgram.  In
   *                         the ARB extension, the SSO active programs
   *                         are now program pipeline object state and
   *                         CURRENT_PROGRAM should still be used to query
   *                         the last program set by UseProgram (bug 7822).
   */
   v->value_int =
            case GL_READ_FRAMEBUFFER_BINDING_EXT:
      v->value_int = ctx->ReadBuffer->Name;
      case GL_RENDERBUFFER_BINDING_EXT:
      v->value_int =
            case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
      buf = ctx->Array.VAO->BufferBinding[VERT_ATTRIB_POINT_SIZE].BufferObj;
   v->value_int = buf ? buf->Name : 0;
         case GL_FOG_COLOR:
      if (_mesa_get_clamp_fragment_color(ctx, ctx->DrawBuffer))
         else
            case GL_COLOR_CLEAR_VALUE:
      if (_mesa_get_clamp_fragment_color(ctx, ctx->DrawBuffer)) {
      v->value_float_4[0] = CLAMP(ctx->Color.ClearColor.f[0], 0.0F, 1.0F);
   v->value_float_4[1] = CLAMP(ctx->Color.ClearColor.f[1], 0.0F, 1.0F);
   v->value_float_4[2] = CLAMP(ctx->Color.ClearColor.f[2], 0.0F, 1.0F);
      } else
            case GL_BLEND_COLOR_EXT:
      if (_mesa_get_clamp_fragment_color(ctx, ctx->DrawBuffer))
         else
            case GL_ALPHA_TEST_REF:
      if (_mesa_get_clamp_fragment_color(ctx, ctx->DrawBuffer))
         else
            case GL_MAX_VERTEX_UNIFORM_VECTORS:
      v->value_int = ctx->Const.Program[MESA_SHADER_VERTEX].MaxUniformComponents / 4;
         case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
      v->value_int = ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxUniformComponents / 4;
         /* GL_ARB_texture_buffer_object */
   case GL_TEXTURE_BUFFER_ARB:
      v->value_int = ctx->Texture.BufferObject ? ctx->Texture.BufferObject->Name : 0;
      case GL_TEXTURE_BINDING_BUFFER_ARB:
      unit = ctx->Texture.CurrentUnit;
   v->value_int =
            case GL_TEXTURE_BUFFER_DATA_STORE_BINDING_ARB:
      {
      struct gl_buffer_object *buf =
      ctx->Texture.Unit[ctx->Texture.CurrentUnit]
         }
      case GL_TEXTURE_BUFFER_FORMAT_ARB:
      v->value_int = ctx->Texture.Unit[ctx->Texture.CurrentUnit]
               /* GL_ARB_sampler_objects */
   case GL_SAMPLER_BINDING:
      {
      struct gl_sampler_object *samp =
            }
      /* GL_ARB_uniform_buffer_object */
   case GL_UNIFORM_BUFFER_BINDING:
      v->value_int = ctx->UniformBuffer ? ctx->UniformBuffer->Name : 0;
      /* GL_ARB_shader_storage_buffer_object */
   case GL_SHADER_STORAGE_BUFFER_BINDING:
      v->value_int = ctx->ShaderStorageBuffer ? ctx->ShaderStorageBuffer->Name : 0;
      /* GL_ARB_query_buffer_object */
   case GL_QUERY_BUFFER_BINDING:
      v->value_int = ctx->QueryBuffer ? ctx->QueryBuffer->Name : 0;
      /* GL_ARB_timer_query */
   case GL_TIMESTAMP:
      v->value_int64 = _mesa_get_timestamp(ctx);
      /* GL_KHR_DEBUG */
   case GL_DEBUG_OUTPUT:
   case GL_DEBUG_OUTPUT_SYNCHRONOUS:
   case GL_DEBUG_LOGGED_MESSAGES:
   case GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH:
   case GL_DEBUG_GROUP_STACK_DEPTH:
      v->value_int = _mesa_get_debug_state_int(ctx, d->pname);
      /* GL_ARB_shader_atomic_counters */
   case GL_ATOMIC_COUNTER_BUFFER_BINDING:
      v->value_int = ctx->AtomicBuffer ? ctx->AtomicBuffer->Name : 0;
      /* GL 4.3 */
   case GL_NUM_SHADING_LANGUAGE_VERSIONS:
      v->value_int = _mesa_get_shading_language_version(ctx, -1, NULL);
      /* GL_ARB_draw_indirect */
   case GL_DRAW_INDIRECT_BUFFER_BINDING:
      v->value_int = ctx->DrawIndirectBuffer ? ctx->DrawIndirectBuffer->Name: 0;
      /* GL_ARB_indirect_parameters */
   case GL_PARAMETER_BUFFER_BINDING_ARB:
      v->value_int = ctx->ParameterBuffer ? ctx->ParameterBuffer->Name : 0;
      /* GL_ARB_separate_shader_objects */
   case GL_PROGRAM_PIPELINE_BINDING:
      if (ctx->Pipeline.Current) {
         } else {
         }
      /* GL_ARB_compute_shader */
   case GL_DISPATCH_INDIRECT_BUFFER_BINDING:
      v->value_int = ctx->DispatchIndirectBuffer ?
            /* GL_ARB_multisample */
   case GL_SAMPLES:
      v->value_int = _mesa_geometric_samples(ctx->DrawBuffer);
      case GL_SAMPLE_BUFFERS:
      v->value_int = _mesa_geometric_samples(ctx->DrawBuffer) > 0;
      /* GL_EXT_textrue_integer */
   case GL_RGBA_INTEGER_MODE_EXT:
      v->value_int = (ctx->DrawBuffer->_IntegerBuffers != 0);
      /* GL_ATI_meminfo & GL_NVX_gpu_memory_info */
   case GL_VBO_FREE_MEMORY_ATI:
   case GL_TEXTURE_FREE_MEMORY_ATI:
   case GL_RENDERBUFFER_FREE_MEMORY_ATI:
   case GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX:
   case GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX:
   case GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX:
   case GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX:
   case GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX:
      {
                                    if (d->pname == GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX)
         else if (d->pname == GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX)
      v->value_int = info.total_device_memory +
      else if (d->pname == GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX)
         else if (d->pname == GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX)
         else if (d->pname == GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX)
         else {
      /* ATI free memory enums.
   *
   * Since the GPU memory is (usually) page-table based, every two
   * consecutive elements are equal. From the GL_ATI_meminfo
   * specification:
   *
   *    "param[0] - total memory free in the pool
   *     param[1] - largest available free block in the pool
   *     param[2] - total auxiliary memory free
   *     param[3] - largest auxiliary free block"
   *
   * All three (VBO, TEXTURE, RENDERBUFFER) queries return
   * the same numbers here.
   */
   v->value_int_4[0] = info.avail_device_memory;
   v->value_int_4[1] = info.avail_device_memory;
   v->value_int_4[2] = info.avail_staging_memory;
         }
         /* GL_ARB_get_program_binary */
   case GL_PROGRAM_BINARY_FORMATS:
      assert(ctx->Const.NumProgramBinaryFormats <= 1);
   v->value_int_n.n = MIN2(ctx->Const.NumProgramBinaryFormats, 1);
   if (ctx->Const.NumProgramBinaryFormats > 0) {
         }
      /* GL_ARB_gl_spirv */
   case GL_SHADER_BINARY_FORMATS:
      assert(ctx->Const.NumShaderBinaryFormats <= 1);
   v->value_int_n.n = MIN2(ctx->Const.NumShaderBinaryFormats, 1);
   if (ctx->Const.NumShaderBinaryFormats > 0) {
         }
      /* ARB_spirv_extensions */
   case GL_NUM_SPIR_V_EXTENSIONS:
      v->value_int = _mesa_get_spirv_extension_count(ctx);
      /* GL_EXT_disjoint_timer_query */
   case GL_GPU_DISJOINT_EXT:
      {
         }
      /* GL_ARB_sample_locations */
   case GL_SAMPLE_LOCATION_SUBPIXEL_BITS_ARB:
   case GL_SAMPLE_LOCATION_PIXEL_GRID_WIDTH_ARB:
   case GL_SAMPLE_LOCATION_PIXEL_GRID_HEIGHT_ARB:
      {
                              if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE) {
      v->value_uint = 0;
                              if (d->pname == GL_SAMPLE_LOCATION_PIXEL_GRID_WIDTH_ARB)
         else if (d->pname == GL_SAMPLE_LOCATION_PIXEL_GRID_HEIGHT_ARB)
         else
      }
      case GL_PROGRAMMABLE_SAMPLE_LOCATION_TABLE_SIZE_ARB:
      v->value_uint = MAX_SAMPLE_LOCATION_TABLE_SIZE;
         /* GL_AMD_framebuffer_multisample_advanced */
   case GL_SUPPORTED_MULTISAMPLE_MODES_AMD:
      v->value_int_n.n = ctx->Const.NumSupportedMultisampleModes * 3;
   memcpy(v->value_int_n.ints, ctx->Const.SupportedMultisampleModes,
               /* GL_NV_viewport_swizzle */
   case GL_VIEWPORT_SWIZZLE_X_NV:
      v->value_enum = ctx->ViewportArray[0].SwizzleX;
      case GL_VIEWPORT_SWIZZLE_Y_NV:
      v->value_enum = ctx->ViewportArray[0].SwizzleY;
      case GL_VIEWPORT_SWIZZLE_Z_NV:
      v->value_enum = ctx->ViewportArray[0].SwizzleZ;
      case GL_VIEWPORT_SWIZZLE_W_NV:
      v->value_enum = ctx->ViewportArray[0].SwizzleW;
         }
      /**
   * Check extra constraints on a struct value_desc descriptor
   *
   * If a struct value_desc has a non-NULL extra pointer, it means that
   * there are a number of extra constraints to check or actions to
   * perform.  The extras is just an integer array where each integer
   * encode different constraints or actions.
   *
   * \param ctx current context
   * \param func name of calling glGet*v() function for error reporting
   * \param d the struct value_desc that has the extra constraints
   *
   * \return GL_FALSE if all of the constraints were not satisfied,
   *     otherwise GL_TRUE.
   */
   static GLboolean
   check_extra(struct gl_context *ctx, const char *func, const struct value_desc *d)
   {
      const GLuint version = ctx->Version;
   GLboolean api_check = GL_FALSE;
   GLboolean api_found = GL_FALSE;
            for (e = d->extra; *e != EXTRA_END; e++) {
      switch (*e) {
   case EXTRA_VERSION_30:
      api_check = GL_TRUE;
   if (version >= 30)
            case EXTRA_VERSION_31:
      api_check = GL_TRUE;
   if (version >= 31)
            case EXTRA_VERSION_32:
      api_check = GL_TRUE;
   if (version >= 32)
            case EXTRA_VERSION_40:
      api_check = GL_TRUE;
   if (version >= 40)
            case EXTRA_VERSION_43:
      api_check = GL_TRUE;
   if (_mesa_is_desktop_gl(ctx) && version >= 43)
            case EXTRA_API_ES:
      api_check = GL_TRUE;
   if (_mesa_is_gles(ctx))
            case EXTRA_API_ES2:
      api_check = GL_TRUE;
   if (_mesa_is_gles2(ctx))
            case EXTRA_API_ES3:
      api_check = GL_TRUE;
   if (_mesa_is_gles3(ctx))
            case EXTRA_API_ES31:
      api_check = GL_TRUE;
   if (_mesa_is_gles31(ctx))
            case EXTRA_API_ES32:
      api_check = GL_TRUE;
   if (_mesa_is_gles32(ctx))
            case EXTRA_API_GL:
      api_check = GL_TRUE;
   if (_mesa_is_desktop_gl(ctx))
            case EXTRA_API_GL_CORE:
      api_check = GL_TRUE;
   if (_mesa_is_desktop_gl_core(ctx))
            case EXTRA_API_GL_COMPAT:
      api_check = GL_TRUE;
   if (_mesa_is_desktop_gl_compat(ctx))
            case EXTRA_NEW_BUFFERS:
      if (ctx->NewState & _NEW_BUFFERS)
            case EXTRA_FLUSH_CURRENT:
      FLUSH_CURRENT(ctx, 0);
      case EXTRA_VALID_DRAW_BUFFER:
      if (d->pname - GL_DRAW_BUFFER0_ARB >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(draw buffer %u)",
            }
      case EXTRA_VALID_TEXTURE_UNIT:
      if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(texture %u)",
            }
      case EXTRA_VALID_CLIP_DISTANCE:
      if (d->pname - GL_CLIP_DISTANCE0 >= ctx->Const.MaxClipPlanes) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(clip distance %u)",
            }
      case EXTRA_GLSL_130:
      api_check = GL_TRUE;
   if (ctx->Const.GLSLVersion >= 130)
            case EXTRA_EXT_UBO_GS:
      api_check = GL_TRUE;
   if (ctx->Extensions.ARB_uniform_buffer_object &&
      _mesa_has_geometry_shaders(ctx))
         case EXTRA_EXT_ATOMICS_GS:
      api_check = GL_TRUE;
   if (ctx->Extensions.ARB_shader_atomic_counters &&
      _mesa_has_geometry_shaders(ctx))
         case EXTRA_EXT_SHADER_IMAGE_GS:
      api_check = GL_TRUE;
   if ((ctx->Extensions.ARB_shader_image_load_store ||
      _mesa_is_gles31(ctx)) &&
   _mesa_has_geometry_shaders(ctx))
         case EXTRA_EXT_ATOMICS_TESS:
      api_check = GL_TRUE;
   api_found = ctx->Extensions.ARB_shader_atomic_counters &&
            case EXTRA_EXT_SHADER_IMAGE_TESS:
      api_check = GL_TRUE;
   api_found = ctx->Extensions.ARB_shader_image_load_store &&
            case EXTRA_EXT_SSBO_GS:
      api_check = GL_TRUE;
   if (ctx->Extensions.ARB_shader_storage_buffer_object &&
      _mesa_has_geometry_shaders(ctx))
         case EXTRA_EXT_FB_NO_ATTACH_GS:
      api_check = GL_TRUE;
   if (ctx->Extensions.ARB_framebuffer_no_attachments &&
      (_mesa_is_desktop_gl(ctx) ||
   _mesa_has_OES_geometry_shader(ctx)))
         case EXTRA_EXT_ES_GS:
      api_check = GL_TRUE;
   if (_mesa_has_OES_geometry_shader(ctx))
            case EXTRA_EXT_PROVOKING_VERTEX_32:
      api_check = GL_TRUE;
   if (_mesa_is_desktop_gl_compat(ctx) || version == 32)
            case EXTRA_END:
         default: /* *e is a offset into the extension struct */
      api_check = GL_TRUE;
   if (*(GLboolean *) ((char *) &ctx->Extensions + *e))
                        if (api_check && !api_found) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=%s)", func,
                        }
      static const struct value_desc error_value =
            /**
   * Find the struct value_desc corresponding to the enum 'pname'.
   *
   * We hash the enum value to get an index into the 'table' array,
   * which holds the index in the 'values' array of struct value_desc.
   * Once we've found the entry, we do the extra checks, if any, then
   * look up the value and return a pointer to it.
   *
   * If the value has to be computed (for example, it's the result of a
   * function call or we need to add 1 to it), we use the tmp 'v' to
   * store the result.
   *
   * \param func name of glGet*v() func for error reporting
   * \param pname the enum value we're looking up
   * \param p is were we return the pointer to the value
   * \param v a tmp union value variable in the calling glGet*v() function
   *
   * \return the struct value_desc corresponding to the enum or a struct
   *     value_desc of TYPE_INVALID if not found.  This lets the calling
   *     glGet*v() function jump right into a switch statement and
   *     handle errors there instead of having to check for NULL.
   */
   static const struct value_desc *
   find_value(const char *func, GLenum pname, void **p, union value *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   int mask, hash;
   const struct value_desc *d;
                     api = ctx->API;
   /* We index into the table_set[] list of per-API hash tables using the API's
   * value in the gl_api enum. Since GLES 3 doesn't have an API_OPENGL* enum
   * value since it's compatible with GLES2 its entry in table_set[] is at the
   * end.
   */
   STATIC_ASSERT(ARRAY_SIZE(table_set) == API_OPENGL_LAST + 4);
   if (_mesa_is_gles2(ctx)) {
      if (ctx->Version >= 32)
         else if (ctx->Version >= 31)
         else if (ctx->Version >= 30)
      }
   mask = ARRAY_SIZE(table(api)) - 1;
   hash = (pname * prime_factor);
   while (1) {
               /* If the enum isn't valid, the hash walk ends with index 0,
   * pointing to the first entry of values[] which doesn't hold
   * any valid enum. */
   if (unlikely(idx == 0)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=%s)", func,
                     d = &values[idx];
   if (likely(d->pname == pname))
                        if (unlikely(d->extra && !check_extra(ctx, func, d)))
            switch (d->location) {
   case LOC_BUFFER:
      *p = ((char *) ctx->DrawBuffer + d->offset);
      case LOC_CONTEXT:
      *p = ((char *) ctx + d->offset);
      case LOC_ARRAY:
      *p = ((char *) ctx->Array.VAO + d->offset);
      case LOC_TEXUNIT:
      if (ctx->Texture.CurrentUnit < ARRAY_SIZE(ctx->Texture.FixedFuncUnit)) {
      unsigned index = ctx->Texture.CurrentUnit;
   *p = ((char *)&ctx->Texture.FixedFuncUnit[index] + d->offset);
      }
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(pname=%s,unit=%d)", func,
                  case LOC_CUSTOM:
      find_custom_value(ctx, d, v);
   *p = v;
      default:
      assert(0);
               /* silence warning */
      }
      static const int transpose[] = {
      0, 4,  8, 12,
   1, 5,  9, 13,
   2, 6, 10, 14,
      };
      static GLsizei
   get_value_size(enum value_type type, const union value *v)
   {
      switch (type) {
   case TYPE_INVALID:
         case TYPE_CONST:
   case TYPE_UINT:
   case TYPE_INT:
         case TYPE_INT_2:
   case TYPE_UINT_2:
         case TYPE_INT_3:
   case TYPE_UINT_3:
         case TYPE_INT_4:
   case TYPE_UINT_4:
         case TYPE_INT_N:
         case TYPE_INT64:
      return sizeof(GLint64);
      case TYPE_ENUM16:
         case TYPE_ENUM:
         case TYPE_ENUM_2:
         case TYPE_BOOLEAN:
         case TYPE_UBYTE:
         case TYPE_SHORT:
         case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
         case TYPE_FLOAT:
   case TYPE_FLOATN:
         case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
         case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
         case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
         case TYPE_FLOAT_8:
         case TYPE_DOUBLEN:
         case TYPE_DOUBLEN_2:
         case TYPE_MATRIX:
         case TYPE_MATRIX_T:
         default:
      assert(!"invalid value_type given for get_value_size()");
         }
      void GLAPIENTRY
   _mesa_GetBooleanv(GLenum pname, GLboolean *params)
   {
      const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
            d = find_value("glGetBooleanv", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
         case TYPE_CONST:
      params[0] = INT_TO_BOOLEAN(d->offset);
         case TYPE_FLOAT_8:
      params[7] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[7]);
   params[6] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[6]);
   params[5] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[5]);
   params[4] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[4]);
      case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[3]);
      case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[2]);
      case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[1]);
      case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[0]);
         case TYPE_DOUBLEN_2:
      params[1] = FLOAT_TO_BOOLEAN(((GLdouble *) p)[1]);
      case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_BOOLEAN(((GLdouble *) p)[0]);
         case TYPE_INT_4:
   case TYPE_UINT_4:
      params[3] = INT_TO_BOOLEAN(((GLint *) p)[3]);
      case TYPE_INT_3:
   case TYPE_UINT_3:
      params[2] = INT_TO_BOOLEAN(((GLint *) p)[2]);
      case TYPE_INT_2:
   case TYPE_UINT_2:
   case TYPE_ENUM_2:
      params[1] = INT_TO_BOOLEAN(((GLint *) p)[1]);
      case TYPE_INT:
   case TYPE_UINT:
   case TYPE_ENUM:
      params[0] = INT_TO_BOOLEAN(((GLint *) p)[0]);
         case TYPE_ENUM16:
      params[0] = INT_TO_BOOLEAN(((GLenum16 *) p)[0]);
         case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
               case TYPE_INT64:
      params[0] = INT64_TO_BOOLEAN(((GLint64 *) p)[0]);
         case TYPE_BOOLEAN:
      params[0] = ((GLboolean*) p)[0];
         case TYPE_UBYTE:
      params[0] = INT_TO_BOOLEAN(((GLubyte *) p)[0]);
         case TYPE_SHORT:
      params[0] = INT_TO_BOOLEAN(((GLshort *) p)[0]);
         case TYPE_MATRIX:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
   params[0] = (*(GLbitfield *) p >> shift) & 1;
         }
      void GLAPIENTRY
   _mesa_GetFloatv(GLenum pname, GLfloat *params)
   {
      const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
            d = find_value("glGetFloatv", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
         case TYPE_CONST:
      params[0] = (GLfloat) d->offset;
         case TYPE_FLOAT_8:
      params[7] = ((GLfloat *) p)[7];
   params[6] = ((GLfloat *) p)[6];
   params[5] = ((GLfloat *) p)[5];
   params[4] = ((GLfloat *) p)[4];
      case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = ((GLfloat *) p)[3];
      case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = ((GLfloat *) p)[2];
      case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = ((GLfloat *) p)[1];
      case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = ((GLfloat *) p)[0];
         case TYPE_DOUBLEN_2:
      params[1] = (GLfloat) (((GLdouble *) p)[1]);
      case TYPE_DOUBLEN:
      params[0] = (GLfloat) (((GLdouble *) p)[0]);
         case TYPE_INT_4:
      params[3] = (GLfloat) (((GLint *) p)[3]);
      case TYPE_INT_3:
      params[2] = (GLfloat) (((GLint *) p)[2]);
      case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = (GLfloat) (((GLint *) p)[1]);
      case TYPE_INT:
   case TYPE_ENUM:
      params[0] = (GLfloat) (((GLint *) p)[0]);
         case TYPE_ENUM16:
      params[0] = (GLfloat) (((GLenum16 *) p)[0]);
         case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
               case TYPE_UINT_4:
      params[3] = (GLfloat) (((GLuint *) p)[3]);
      case TYPE_UINT_3:
      params[2] = (GLfloat) (((GLuint *) p)[2]);
      case TYPE_UINT_2:
      params[1] = (GLfloat) (((GLuint *) p)[1]);
      case TYPE_UINT:
      params[0] = (GLfloat) (((GLuint *) p)[0]);
         case TYPE_INT64:
      params[0] = (GLfloat) (((GLint64 *) p)[0]);
         case TYPE_BOOLEAN:
      params[0] = BOOLEAN_TO_FLOAT(*(GLboolean*) p);
         case TYPE_UBYTE:
      params[0] = (GLfloat) ((GLubyte *) p)[0];
         case TYPE_SHORT:
      params[0] = (GLfloat) ((GLshort *) p)[0];
         case TYPE_MATRIX:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
   params[0] = BOOLEAN_TO_FLOAT((*(GLbitfield *) p >> shift) & 1);
         }
      void GLAPIENTRY
   _mesa_GetIntegerv(GLenum pname, GLint *params)
   {
      const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
            d = find_value("glGetIntegerv", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
         case TYPE_CONST:
      params[0] = d->offset;
         case TYPE_FLOAT_8:
      params[7] = lroundf(((GLfloat *) p)[7]);
   params[6] = lroundf(((GLfloat *) p)[6]);
   params[5] = lroundf(((GLfloat *) p)[5]);
   params[4] = lroundf(((GLfloat *) p)[4]);
      case TYPE_FLOAT_4:
      params[3] = lroundf(((GLfloat *) p)[3]);
      case TYPE_FLOAT_3:
      params[2] = lroundf(((GLfloat *) p)[2]);
      case TYPE_FLOAT_2:
      params[1] = lroundf(((GLfloat *) p)[1]);
      case TYPE_FLOAT:
      params[0] = lroundf(((GLfloat *) p)[0]);
         case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_INT(((GLfloat *) p)[3]);
      case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_INT(((GLfloat *) p)[2]);
      case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_INT(((GLfloat *) p)[1]);
      case TYPE_FLOATN:
      params[0] = FLOAT_TO_INT(((GLfloat *) p)[0]);
         case TYPE_DOUBLEN_2:
      params[1] = FLOAT_TO_INT(((GLdouble *) p)[1]);
      case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_INT(((GLdouble *) p)[0]);
         case TYPE_INT_4:
      params[3] = ((GLint *) p)[3];
      case TYPE_INT_3:
      params[2] = ((GLint *) p)[2];
      case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = ((GLint *) p)[1];
      case TYPE_INT:
   case TYPE_ENUM:
      params[0] = ((GLint *) p)[0];
         case TYPE_UINT_4:
      params[3] = MIN2(((GLuint *) p)[3], INT_MAX);
      case TYPE_UINT_3:
      params[2] = MIN2(((GLuint *) p)[2], INT_MAX);
      case TYPE_UINT_2:
      params[1] = MIN2(((GLuint *) p)[1], INT_MAX);
      case TYPE_UINT:
      params[0] = MIN2(((GLuint *) p)[0], INT_MAX);
         case TYPE_ENUM16:
      params[0] = ((GLenum16 *) p)[0];
         case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
               case TYPE_INT64:
      params[0] = INT64_TO_INT(((GLint64 *) p)[0]);
         case TYPE_BOOLEAN:
      params[0] = BOOLEAN_TO_INT(*(GLboolean*) p);
         case TYPE_UBYTE:
      params[0] = ((GLubyte *) p)[0];
         case TYPE_SHORT:
      params[0] = ((GLshort *) p)[0];
         case TYPE_MATRIX:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
   params[0] = (*(GLbitfield *) p >> shift) & 1;
         }
      void GLAPIENTRY
   _mesa_GetInteger64v(GLenum pname, GLint64 *params)
   {
      const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
            d = find_value("glGetInteger64v", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
         case TYPE_CONST:
      params[0] = d->offset;
         case TYPE_FLOAT_8:
      params[7] = llround(((GLfloat *) p)[7]);
   params[6] = llround(((GLfloat *) p)[6]);
   params[5] = llround(((GLfloat *) p)[5]);
   params[4] = llround(((GLfloat *) p)[4]);
      case TYPE_FLOAT_4:
      params[3] = llround(((GLfloat *) p)[3]);
      case TYPE_FLOAT_3:
      params[2] = llround(((GLfloat *) p)[2]);
      case TYPE_FLOAT_2:
      params[1] = llround(((GLfloat *) p)[1]);
      case TYPE_FLOAT:
      params[0] = llround(((GLfloat *) p)[0]);
         case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_INT(((GLfloat *) p)[3]);
      case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_INT(((GLfloat *) p)[2]);
      case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_INT(((GLfloat *) p)[1]);
      case TYPE_FLOATN:
      params[0] = FLOAT_TO_INT(((GLfloat *) p)[0]);
         case TYPE_DOUBLEN_2:
      params[1] = FLOAT_TO_INT(((GLdouble *) p)[1]);
      case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_INT(((GLdouble *) p)[0]);
         case TYPE_INT_4:
      params[3] = ((GLint *) p)[3];
      case TYPE_INT_3:
      params[2] = ((GLint *) p)[2];
      case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = ((GLint *) p)[1];
      case TYPE_INT:
   case TYPE_ENUM:
      params[0] = ((GLint *) p)[0];
         case TYPE_ENUM16:
      params[0] = ((GLenum16 *) p)[0];
         case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
               case TYPE_UINT_4:
      params[3] = ((GLuint *) p)[3];
      case TYPE_UINT_3:
      params[2] = ((GLuint *) p)[2];
      case TYPE_UINT_2:
      params[1] = ((GLuint *) p)[1];
      case TYPE_UINT:
      params[0] = ((GLuint *) p)[0];
         case TYPE_INT64:
      params[0] = ((GLint64 *) p)[0];
         case TYPE_BOOLEAN:
      params[0] = ((GLboolean*) p)[0];
         case TYPE_MATRIX:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
   params[0] = (*(GLbitfield *) p >> shift) & 1;
         }
      void GLAPIENTRY
   _mesa_GetDoublev(GLenum pname, GLdouble *params)
   {
      const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
            d = find_value("glGetDoublev", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
         case TYPE_CONST:
      params[0] = d->offset;
         case TYPE_FLOAT_8:
      params[7] = ((GLfloat *) p)[7];
   params[6] = ((GLfloat *) p)[6];
   params[5] = ((GLfloat *) p)[5];
   params[4] = ((GLfloat *) p)[4];
      case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = ((GLfloat *) p)[3];
      case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = ((GLfloat *) p)[2];
      case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = ((GLfloat *) p)[1];
      case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = ((GLfloat *) p)[0];
         case TYPE_DOUBLEN_2:
      params[1] = ((GLdouble *) p)[1];
      case TYPE_DOUBLEN:
      params[0] = ((GLdouble *) p)[0];
         case TYPE_INT_4:
      params[3] = ((GLint *) p)[3];
      case TYPE_INT_3:
      params[2] = ((GLint *) p)[2];
      case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = ((GLint *) p)[1];
      case TYPE_INT:
   case TYPE_ENUM:
      params[0] = ((GLint *) p)[0];
         case TYPE_ENUM16:
      params[0] = ((GLenum16 *) p)[0];
         case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
               case TYPE_UINT_4:
      params[3] = ((GLuint *) p)[3];
      case TYPE_UINT_3:
      params[2] = ((GLuint *) p)[2];
      case TYPE_UINT_2:
      params[1] = ((GLuint *) p)[1];
      case TYPE_UINT:
      params[0] = ((GLuint *) p)[0];
         case TYPE_INT64:
      params[0] = (GLdouble) (((GLint64 *) p)[0]);
         case TYPE_BOOLEAN:
      params[0] = *(GLboolean*) p;
         case TYPE_UBYTE:
      params[0] = ((GLubyte *) p)[0];
         case TYPE_SHORT:
      params[0] = ((GLshort *) p)[0];
         case TYPE_MATRIX:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
   params[0] = (*(GLbitfield *) p >> shift) & 1;
         }
      void GLAPIENTRY
   _mesa_GetUnsignedBytevEXT(GLenum pname, GLubyte *data)
   {
      const struct value_desc *d;
   union value v;
   int shift;
   void *p = NULL;
   GLsizei size;
                     if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               d = find_value(func, pname, &p, &v);
            switch (d->type) {
   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
   data[0] = (*(GLbitfield *) p >> shift) & 1;
      case TYPE_CONST:
      memcpy(data, &d->offset, size);
      case TYPE_INT_N:
      memcpy(data, &v.value_int_n.ints, size);
      case TYPE_UINT:
   case TYPE_INT:
   case TYPE_INT_2:
   case TYPE_UINT_2:
   case TYPE_INT_3:
   case TYPE_UINT_3:
   case TYPE_INT_4:
   case TYPE_UINT_4:
   case TYPE_INT64:
   case TYPE_ENUM:
   case TYPE_ENUM_2:
   case TYPE_BOOLEAN:
   case TYPE_UBYTE:
   case TYPE_SHORT:
   case TYPE_FLOAT:
   case TYPE_FLOATN:
   case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
   case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
   case TYPE_FLOAT_8:
   case TYPE_DOUBLEN:
   case TYPE_DOUBLEN_2:
   case TYPE_MATRIX:
   case TYPE_MATRIX_T:
      memcpy(data, p, size);
      case TYPE_ENUM16: {
      GLenum e = *(GLenum16 *)p;
   memcpy(data, &e, sizeof(e));
      }
   default:
            }
      /**
   * Convert a GL texture binding enum such as GL_TEXTURE_BINDING_2D
   * into the corresponding Mesa texture target index.
   * \return TEXTURE_x_INDEX or -1 if binding is invalid
   */
   static int
   tex_binding_to_index(const struct gl_context *ctx, GLenum binding)
   {
      switch (binding) {
   case GL_TEXTURE_BINDING_1D:
         case GL_TEXTURE_BINDING_2D:
         case GL_TEXTURE_BINDING_3D:
      return (ctx->API != API_OPENGLES &&
            case GL_TEXTURE_BINDING_CUBE_MAP:
         case GL_TEXTURE_BINDING_RECTANGLE:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.NV_texture_rectangle
      case GL_TEXTURE_BINDING_1D_ARRAY:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.EXT_texture_array
      case GL_TEXTURE_BINDING_2D_ARRAY:
      return (_mesa_is_desktop_gl(ctx) && ctx->Extensions.EXT_texture_array)
      || _mesa_is_gles3(ctx)
   case GL_TEXTURE_BINDING_BUFFER:
      return (_mesa_has_ARB_texture_buffer_object(ctx) ||
            case GL_TEXTURE_BINDING_CUBE_MAP_ARRAY:
      return _mesa_has_texture_cube_map_array(ctx)
      case GL_TEXTURE_BINDING_2D_MULTISAMPLE:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.ARB_texture_multisample
      case GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.ARB_texture_multisample
      default:
            }
      static enum value_type
   find_value_indexed(const char *func, GLenum pname, GLuint index, union value *v)
   {
      GET_CURRENT_CONTEXT(ctx);
                     case GL_BLEND:
      if (index >= ctx->Const.MaxDrawBuffers)
         if (!ctx->Extensions.EXT_draw_buffers2)
         v->value_int = (ctx->Color.BlendEnabled >> index) & 1;
         case GL_BLEND_SRC:
         case GL_BLEND_SRC_RGB:
      if (index >= ctx->Const.MaxDrawBuffers)
         if (!ctx->Extensions.ARB_draw_buffers_blend)
         v->value_int = ctx->Color.Blend[index].SrcRGB;
      case GL_BLEND_SRC_ALPHA:
      if (index >= ctx->Const.MaxDrawBuffers)
         if (!ctx->Extensions.ARB_draw_buffers_blend)
         v->value_int = ctx->Color.Blend[index].SrcA;
      case GL_BLEND_DST:
         case GL_BLEND_DST_RGB:
      if (index >= ctx->Const.MaxDrawBuffers)
         if (!ctx->Extensions.ARB_draw_buffers_blend)
         v->value_int = ctx->Color.Blend[index].DstRGB;
      case GL_BLEND_DST_ALPHA:
      if (index >= ctx->Const.MaxDrawBuffers)
         if (!ctx->Extensions.ARB_draw_buffers_blend)
         v->value_int = ctx->Color.Blend[index].DstA;
      case GL_BLEND_EQUATION_RGB:
      if (index >= ctx->Const.MaxDrawBuffers)
         if (!ctx->Extensions.ARB_draw_buffers_blend)
         v->value_int = ctx->Color.Blend[index].EquationRGB;
      case GL_BLEND_EQUATION_ALPHA:
      if (index >= ctx->Const.MaxDrawBuffers)
         if (!ctx->Extensions.ARB_draw_buffers_blend)
         v->value_int = ctx->Color.Blend[index].EquationA;
         case GL_COLOR_WRITEMASK:
      if (index >= ctx->Const.MaxDrawBuffers)
         if (!ctx->Extensions.EXT_draw_buffers2)
         v->value_int_4[0] = GET_COLORMASK_BIT(ctx->Color.ColorMask, index, 0);
   v->value_int_4[1] = GET_COLORMASK_BIT(ctx->Color.ColorMask, index, 1);
   v->value_int_4[2] = GET_COLORMASK_BIT(ctx->Color.ColorMask, index, 2);
   v->value_int_4[3] = GET_COLORMASK_BIT(ctx->Color.ColorMask, index, 3);
         case GL_SCISSOR_BOX:
      if (index >= ctx->Const.MaxViewports)
         v->value_int_4[0] = ctx->Scissor.ScissorArray[index].X;
   v->value_int_4[1] = ctx->Scissor.ScissorArray[index].Y;
   v->value_int_4[2] = ctx->Scissor.ScissorArray[index].Width;
   v->value_int_4[3] = ctx->Scissor.ScissorArray[index].Height;
         case GL_WINDOW_RECTANGLE_EXT:
      if (!ctx->Extensions.EXT_window_rectangles)
         if (index >= ctx->Const.MaxWindowRectangles)
         v->value_int_4[0] = ctx->Scissor.WindowRects[index].X;
   v->value_int_4[1] = ctx->Scissor.WindowRects[index].Y;
   v->value_int_4[2] = ctx->Scissor.WindowRects[index].Width;
   v->value_int_4[3] = ctx->Scissor.WindowRects[index].Height;
         case GL_VIEWPORT:
      if (index >= ctx->Const.MaxViewports)
         v->value_float_4[0] = ctx->ViewportArray[index].X;
   v->value_float_4[1] = ctx->ViewportArray[index].Y;
   v->value_float_4[2] = ctx->ViewportArray[index].Width;
   v->value_float_4[3] = ctx->ViewportArray[index].Height;
         case GL_DEPTH_RANGE:
      if (index >= ctx->Const.MaxViewports)
         v->value_double_2[0] = ctx->ViewportArray[index].Near;
   v->value_double_2[1] = ctx->ViewportArray[index].Far;
         case GL_TRANSFORM_FEEDBACK_BUFFER_START:
      if (index >= ctx->Const.MaxTransformFeedbackBuffers)
         if (!ctx->Extensions.EXT_transform_feedback)
         v->value_int64 = ctx->TransformFeedback.CurrentObject->Offset[index];
         case GL_TRANSFORM_FEEDBACK_BUFFER_SIZE:
      if (index >= ctx->Const.MaxTransformFeedbackBuffers)
         if (!ctx->Extensions.EXT_transform_feedback)
         v->value_int64
               case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
      if (index >= ctx->Const.MaxTransformFeedbackBuffers)
         if (!ctx->Extensions.EXT_transform_feedback)
         v->value_int = ctx->TransformFeedback.CurrentObject->BufferNames[index];
         case GL_UNIFORM_BUFFER_BINDING:
      if (index >= ctx->Const.MaxUniformBufferBindings)
         if (!ctx->Extensions.ARB_uniform_buffer_object)
         buf = ctx->UniformBufferBindings[index].BufferObject;
   v->value_int = buf ? buf->Name : 0;
         case GL_UNIFORM_BUFFER_START:
      if (index >= ctx->Const.MaxUniformBufferBindings)
         if (!ctx->Extensions.ARB_uniform_buffer_object)
         v->value_int = ctx->UniformBufferBindings[index].Offset < 0 ? 0 :
               case GL_UNIFORM_BUFFER_SIZE:
      if (index >= ctx->Const.MaxUniformBufferBindings)
         if (!ctx->Extensions.ARB_uniform_buffer_object)
         v->value_int = ctx->UniformBufferBindings[index].Size < 0 ? 0 :
               /* ARB_shader_storage_buffer_object */
   case GL_SHADER_STORAGE_BUFFER_BINDING:
      if (!ctx->Extensions.ARB_shader_storage_buffer_object && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxShaderStorageBufferBindings)
         buf = ctx->ShaderStorageBufferBindings[index].BufferObject;
   v->value_int = buf ? buf->Name : 0;
         case GL_SHADER_STORAGE_BUFFER_START:
      if (!ctx->Extensions.ARB_shader_storage_buffer_object && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxShaderStorageBufferBindings)
         v->value_int = ctx->ShaderStorageBufferBindings[index].Offset < 0 ? 0 :
               case GL_SHADER_STORAGE_BUFFER_SIZE:
      if (!ctx->Extensions.ARB_shader_storage_buffer_object && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxShaderStorageBufferBindings)
         v->value_int = ctx->ShaderStorageBufferBindings[index].Size < 0 ? 0 :
               /* ARB_texture_multisample / GL3.2 */
   case GL_SAMPLE_MASK_VALUE:
      if (index != 0)
         if (!ctx->Extensions.ARB_texture_multisample)
         v->value_int = ctx->Multisample.SampleMaskValue;
         case GL_ATOMIC_COUNTER_BUFFER_BINDING:
      if (!ctx->Extensions.ARB_shader_atomic_counters && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxAtomicBufferBindings)
         buf = ctx->AtomicBufferBindings[index].BufferObject;
   v->value_int = buf ? buf->Name : 0;
         case GL_ATOMIC_COUNTER_BUFFER_START:
      if (!ctx->Extensions.ARB_shader_atomic_counters && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxAtomicBufferBindings)
         v->value_int64 = ctx->AtomicBufferBindings[index].Offset < 0 ? 0 :
               case GL_ATOMIC_COUNTER_BUFFER_SIZE:
      if (!ctx->Extensions.ARB_shader_atomic_counters && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxAtomicBufferBindings)
         v->value_int64 = ctx->AtomicBufferBindings[index].Size < 0 ? 0 :
               case GL_VERTEX_BINDING_DIVISOR:
      if ((!_mesa_is_desktop_gl(ctx) || !ctx->Extensions.ARB_instanced_arrays) &&
      !_mesa_is_gles31(ctx))
      if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs)
         v->value_int = ctx->Array.VAO->BufferBinding[VERT_ATTRIB_GENERIC(index)].InstanceDivisor;
         case GL_VERTEX_BINDING_OFFSET:
      if (!_mesa_is_desktop_gl(ctx) && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs)
         v->value_int = ctx->Array.VAO->BufferBinding[VERT_ATTRIB_GENERIC(index)].Offset;
         case GL_VERTEX_BINDING_STRIDE:
      if (!_mesa_is_desktop_gl(ctx) && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs)
         v->value_int = ctx->Array.VAO->BufferBinding[VERT_ATTRIB_GENERIC(index)].Stride;
         case GL_VERTEX_BINDING_BUFFER:
      if (_mesa_is_gles2(ctx) && ctx->Version < 31)
         if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs)
         buf = ctx->Array.VAO->BufferBinding[VERT_ATTRIB_GENERIC(index)].BufferObj;
   v->value_int = buf ? buf->Name : 0;
         /* ARB_shader_image_load_store */
   case GL_IMAGE_BINDING_NAME: {
               if (!ctx->Extensions.ARB_shader_image_load_store && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxImageUnits)
            t = ctx->ImageUnits[index].TexObj;
   v->value_int = (t ? t->Name : 0);
               case GL_IMAGE_BINDING_LEVEL:
      if (!ctx->Extensions.ARB_shader_image_load_store && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxImageUnits)
            v->value_int = ctx->ImageUnits[index].Level;
         case GL_IMAGE_BINDING_LAYERED:
      if (!ctx->Extensions.ARB_shader_image_load_store && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxImageUnits)
            v->value_int = ctx->ImageUnits[index].Layered;
         case GL_IMAGE_BINDING_LAYER:
      if (!ctx->Extensions.ARB_shader_image_load_store && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxImageUnits)
            v->value_int = ctx->ImageUnits[index].Layer;
         case GL_IMAGE_BINDING_ACCESS:
      if (!ctx->Extensions.ARB_shader_image_load_store && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxImageUnits)
            v->value_int = ctx->ImageUnits[index].Access;
         case GL_IMAGE_BINDING_FORMAT:
      if (!ctx->Extensions.ARB_shader_image_load_store && !_mesa_is_gles31(ctx))
         if (index >= ctx->Const.MaxImageUnits)
            v->value_int = ctx->ImageUnits[index].Format;
         /* ARB_direct_state_access */
   case GL_TEXTURE_BINDING_1D:
   case GL_TEXTURE_BINDING_1D_ARRAY:
   case GL_TEXTURE_BINDING_2D:
   case GL_TEXTURE_BINDING_2D_ARRAY:
   case GL_TEXTURE_BINDING_2D_MULTISAMPLE:
   case GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY:
   case GL_TEXTURE_BINDING_3D:
   case GL_TEXTURE_BINDING_BUFFER:
   case GL_TEXTURE_BINDING_CUBE_MAP:
   case GL_TEXTURE_BINDING_CUBE_MAP_ARRAY:
   case GL_TEXTURE_BINDING_RECTANGLE: {
               target = tex_binding_to_index(ctx, pname);
   if (target < 0)
         if (index >= _mesa_max_tex_unit(ctx))
            v->value_int = ctx->Texture.Unit[index].CurrentTex[target]->Name;
               case GL_SAMPLER_BINDING: {
               if (!_mesa_is_desktop_gl(ctx) || ctx->Version < 33)
         if (index >= _mesa_max_tex_unit(ctx))
            samp = ctx->Texture.Unit[index].Sampler;
   v->value_int = samp ? samp->Name : 0;
               case GL_MAX_COMPUTE_WORK_GROUP_COUNT:
      if (!_mesa_has_compute_shaders(ctx))
         if (index >= 3)
         v->value_uint = ctx->Const.MaxComputeWorkGroupCount[index];
         case GL_MAX_COMPUTE_WORK_GROUP_SIZE:
      if (!_mesa_has_compute_shaders(ctx))
         if (index >= 3)
         v->value_int = ctx->Const.MaxComputeWorkGroupSize[index];
         /* ARB_compute_variable_group_size */
   case GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB:
      if (!ctx->Extensions.ARB_compute_variable_group_size)
         if (index >= 3)
         v->value_int = ctx->Const.MaxComputeVariableGroupSize[index];
         /* GL_EXT_external_objects */
   case GL_NUM_DEVICE_UUIDS_EXT:
      v->value_int = 1;
      case GL_DRIVER_UUID_EXT:
      if (index >= 1)
         _mesa_get_driver_uuid(ctx, v->value_int_4);
      case GL_DEVICE_UUID_EXT:
      if (index >= 1)
         _mesa_get_device_uuid(ctx, v->value_int_4);
      /* GL_EXT_memory_object_win32 */
   case GL_DEVICE_LUID_EXT:
      if (index >= 1)
         _mesa_get_device_luid(ctx, v->value_int_2);
      case GL_DEVICE_NODE_MASK_EXT:
      if (index >= 1)
         v->value_int = ctx->pipe->screen->get_device_node_mask(ctx->pipe->screen);
      /* GL_EXT_direct_state_access */
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_GEN_S:
   case GL_TEXTURE_GEN_T:
   case GL_TEXTURE_GEN_R:
   case GL_TEXTURE_GEN_Q:
   case GL_TEXTURE_RECTANGLE_ARB: {
      GLuint curTexUnitSave;
   if (index >= _mesa_max_tex_unit(ctx))
         curTexUnitSave = ctx->Texture.CurrentUnit;
   _mesa_ActiveTexture_no_error(GL_TEXTURE0 + index);
   v->value_int = _mesa_IsEnabled(pname);
   _mesa_ActiveTexture_no_error(GL_TEXTURE0 + curTexUnitSave);
      }
   case GL_TEXTURE_COORD_ARRAY: {
      GLuint curTexUnitSave;
   if (index >= ctx->Const.MaxTextureCoordUnits)
         curTexUnitSave = ctx->Array.ActiveTexture;
   _mesa_ClientActiveTexture(GL_TEXTURE0 + index);
   v->value_int = _mesa_IsEnabled(pname);
   _mesa_ClientActiveTexture(GL_TEXTURE0 + curTexUnitSave);
      }
   case GL_TEXTURE_MATRIX:
      if (index >= ARRAY_SIZE(ctx->TextureMatrixStack))
         v->value_matrix = ctx->TextureMatrixStack[index].Top;
      case GL_TRANSPOSE_TEXTURE_MATRIX:
      if (index >= ARRAY_SIZE(ctx->TextureMatrixStack))
         v->value_matrix = ctx->TextureMatrixStack[index].Top;
      /* GL_NV_viewport_swizzle */
   case GL_VIEWPORT_SWIZZLE_X_NV:
      if (!ctx->Extensions.NV_viewport_swizzle)
         if (index >= ctx->Const.MaxViewports)
         v->value_int = ctx->ViewportArray[index].SwizzleX;
      case GL_VIEWPORT_SWIZZLE_Y_NV:
      if (!ctx->Extensions.NV_viewport_swizzle)
         if (index >= ctx->Const.MaxViewports)
         v->value_int = ctx->ViewportArray[index].SwizzleY;
      case GL_VIEWPORT_SWIZZLE_Z_NV:
      if (!ctx->Extensions.NV_viewport_swizzle)
         if (index >= ctx->Const.MaxViewports)
         v->value_int = ctx->ViewportArray[index].SwizzleZ;
      case GL_VIEWPORT_SWIZZLE_W_NV:
      if (!ctx->Extensions.NV_viewport_swizzle)
         if (index >= ctx->Const.MaxViewports)
         v->value_int = ctx->ViewportArray[index].SwizzleW;
            invalid_enum:
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=%s)", func,
            invalid_value:
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(pname=%s)", func,
            }
      void GLAPIENTRY
   _mesa_GetBooleani_v( GLenum pname, GLuint index, GLboolean *params )
   {
      union value v;
   enum value_type type =
            switch (type) {
   case TYPE_INT:
   case TYPE_UINT:
      params[0] = INT_TO_BOOLEAN(v.value_int);
      case TYPE_INT_4:
   case TYPE_UINT_4:
      params[0] = INT_TO_BOOLEAN(v.value_int_4[0]);
   params[1] = INT_TO_BOOLEAN(v.value_int_4[1]);
   params[2] = INT_TO_BOOLEAN(v.value_int_4[2]);
   params[3] = INT_TO_BOOLEAN(v.value_int_4[3]);
      case TYPE_INT64:
      params[0] = INT64_TO_BOOLEAN(v.value_int64);
      default:
            }
      void GLAPIENTRY
   _mesa_GetIntegeri_v( GLenum pname, GLuint index, GLint *params )
   {
      union value v;
   enum value_type type =
            switch (type) {
   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = lroundf(v.value_float_4[3]);
      case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = lroundf(v.value_float_4[2]);
      case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = lroundf(v.value_float_4[1]);
      case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = lroundf(v.value_float_4[0]);
         case TYPE_DOUBLEN_2:
      params[1] = lroundf(v.value_double_2[1]);
      case TYPE_DOUBLEN:
      params[0] = lroundf(v.value_double_2[0]);
         case TYPE_INT:
      params[0] = v.value_int;
      case TYPE_UINT:
      params[0] = MIN2(v.value_uint, INT_MAX);
      case TYPE_INT_4:
      params[0] = v.value_int_4[0];
   params[1] = v.value_int_4[1];
   params[2] = v.value_int_4[2];
   params[3] = v.value_int_4[3];
      case TYPE_UINT_4:
      params[0] = MIN2((GLuint)v.value_int_4[0], INT_MAX);
   params[1] = MIN2((GLuint)v.value_int_4[1], INT_MAX);
   params[2] = MIN2((GLuint)v.value_int_4[2], INT_MAX);
   params[3] = MIN2((GLuint)v.value_int_4[3], INT_MAX);
      case TYPE_INT64:
      params[0] = INT64_TO_INT(v.value_int64);
      default:
            }
      void GLAPIENTRY
   _mesa_GetInteger64i_v( GLenum pname, GLuint index, GLint64 *params )
   {
      union value v;
   enum value_type type =
            switch (type) {
   case TYPE_INT:
      params[0] = v.value_int;
      case TYPE_INT_4:
      params[0] = v.value_int_4[0];
   params[1] = v.value_int_4[1];
   params[2] = v.value_int_4[2];
   params[3] = v.value_int_4[3];
      case TYPE_UINT:
      params[0] = v.value_uint;
      case TYPE_UINT_4:
      params[0] = (GLuint) v.value_int_4[0];
   params[1] = (GLuint) v.value_int_4[1];
   params[2] = (GLuint) v.value_int_4[2];
   params[3] = (GLuint) v.value_int_4[3];
      case TYPE_INT64:
      params[0] = v.value_int64;
      default:
            }
      void GLAPIENTRY
   _mesa_GetFloati_v(GLenum pname, GLuint index, GLfloat *params)
   {
      int i;
   GLmatrix *m;
   union value v;
   enum value_type type =
            switch (type) {
   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = v.value_float_4[3];
      case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = v.value_float_4[2];
      case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = v.value_float_4[1];
      case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = v.value_float_4[0];
         case TYPE_DOUBLEN_2:
      params[1] = (GLfloat) v.value_double_2[1];
      case TYPE_DOUBLEN:
      params[0] = (GLfloat) v.value_double_2[0];
         case TYPE_INT_4:
      params[3] = (GLfloat) v.value_int_4[3];
      case TYPE_INT_3:
      params[2] = (GLfloat) v.value_int_4[2];
      case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = (GLfloat) v.value_int_4[1];
      case TYPE_INT:
   case TYPE_ENUM:
   case TYPE_ENUM16:
      params[0] = (GLfloat) v.value_int_4[0];
         case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
               case TYPE_UINT_4:
      params[3] = (GLfloat) ((GLuint) v.value_int_4[3]);
      case TYPE_UINT_3:
      params[2] = (GLfloat) ((GLuint) v.value_int_4[2]);
      case TYPE_UINT_2:
      params[1] = (GLfloat) ((GLuint) v.value_int_4[1]);
      case TYPE_UINT:
      params[0] = (GLfloat) ((GLuint) v.value_int_4[0]);
         case TYPE_INT64:
      params[0] = (GLfloat) v.value_int64;
         case TYPE_BOOLEAN:
      params[0] = BOOLEAN_TO_FLOAT(v.value_bool);
         case TYPE_UBYTE:
      params[0] = (GLfloat) v.value_ubyte;
         case TYPE_SHORT:
      params[0] = (GLfloat) v.value_short;
         case TYPE_MATRIX:
      m = *(GLmatrix **) &v;
   for (i = 0; i < 16; i++)
               case TYPE_MATRIX_T:
      m = *(GLmatrix **) &v;
   for (i = 0; i < 16; i++)
               default:
            }
      void GLAPIENTRY
   _mesa_GetDoublei_v(GLenum pname, GLuint index, GLdouble *params)
   {
      int i;
   GLmatrix *m;
   union value v;
   enum value_type type =
            switch (type) {
   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = (GLdouble) v.value_float_4[3];
      case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = (GLdouble) v.value_float_4[2];
      case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = (GLdouble) v.value_float_4[1];
      case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = (GLdouble) v.value_float_4[0];
         case TYPE_DOUBLEN_2:
      params[1] = v.value_double_2[1];
      case TYPE_DOUBLEN:
      params[0] = v.value_double_2[0];
         case TYPE_INT_4:
      params[3] = (GLdouble) v.value_int_4[3];
      case TYPE_INT_3:
      params[2] = (GLdouble) v.value_int_4[2];
      case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = (GLdouble) v.value_int_4[1];
      case TYPE_INT:
   case TYPE_ENUM:
   case TYPE_ENUM16:
      params[0] = (GLdouble) v.value_int_4[0];
         case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
               case TYPE_UINT_4:
      params[3] = (GLdouble) ((GLuint) v.value_int_4[3]);
      case TYPE_UINT_3:
      params[2] = (GLdouble) ((GLuint) v.value_int_4[2]);
      case TYPE_UINT_2:
      params[1] = (GLdouble) ((GLuint) v.value_int_4[1]);
      case TYPE_UINT:
      params[0] = (GLdouble) ((GLuint) v.value_int_4[0]);
         case TYPE_INT64:
      params[0] = (GLdouble) v.value_int64;
         case TYPE_BOOLEAN:
      params[0] = (GLdouble) BOOLEAN_TO_FLOAT(v.value_bool);
         case TYPE_UBYTE:
      params[0] = (GLdouble) v.value_ubyte;
         case TYPE_SHORT:
      params[0] = (GLdouble) v.value_short;
         case TYPE_MATRIX:
      m = *(GLmatrix **) &v;
   for (i = 0; i < 16; i++)
               case TYPE_MATRIX_T:
      m = *(GLmatrix **) &v;
   for (i = 0; i < 16; i++)
               default:
            }
      void GLAPIENTRY
   _mesa_GetUnsignedBytei_vEXT(GLenum target, GLuint index, GLubyte *data)
   {
      GLsizei size;
   union value v;
   enum value_type type;
                     if (!ctx->Extensions.EXT_memory_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unsupported)", func);
               type = find_value_indexed(func, target, index, &v);
            switch (type) {
   case TYPE_UINT:
   case TYPE_INT:
   case TYPE_INT_2:
   case TYPE_UINT_2:
   case TYPE_INT_3:
   case TYPE_UINT_3:
   case TYPE_INT_4:
   case TYPE_UINT_4:
   case TYPE_INT64:
   case TYPE_ENUM16:
   case TYPE_ENUM:
   case TYPE_ENUM_2:
   case TYPE_BOOLEAN:
   case TYPE_UBYTE:
   case TYPE_SHORT:
   case TYPE_FLOAT:
   case TYPE_FLOATN:
   case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
   case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
   case TYPE_FLOAT_8:
   case TYPE_DOUBLEN:
   case TYPE_DOUBLEN_2:
   case TYPE_MATRIX:
   case TYPE_MATRIX_T:
      memcpy(data, &v.value_int, size);
      case TYPE_INT_N:
      memcpy(data, &v.value_int_n.ints, size);
      default:
            }
      void GLAPIENTRY
   _mesa_GetFixedv(GLenum pname, GLfixed *params)
   {
      const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
            d = find_value("glGetDoublev", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
         case TYPE_CONST:
      params[0] = INT_TO_FIXED(d->offset);
         case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_FIXED(((GLfloat *) p)[3]);
      case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_FIXED(((GLfloat *) p)[2]);
      case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_FIXED(((GLfloat *) p)[1]);
      case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = FLOAT_TO_FIXED(((GLfloat *) p)[0]);
         case TYPE_DOUBLEN_2:
      params[1] = FLOAT_TO_FIXED(((GLdouble *) p)[1]);
      case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_FIXED(((GLdouble *) p)[0]);
         case TYPE_INT_4:
      params[3] = INT_TO_FIXED(((GLint *) p)[3]);
      case TYPE_INT_3:
      params[2] = INT_TO_FIXED(((GLint *) p)[2]);
      case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = INT_TO_FIXED(((GLint *) p)[1]);
      case TYPE_INT:
   case TYPE_ENUM:
      params[0] = INT_TO_FIXED(((GLint *) p)[0]);
         case TYPE_UINT_4:
      params[3] = INT_TO_FIXED(((GLuint *) p)[3]);
      case TYPE_UINT_3:
      params[2] = INT_TO_FIXED(((GLuint *) p)[2]);
      case TYPE_UINT_2:
      params[1] = INT_TO_FIXED(((GLuint *) p)[1]);
      case TYPE_UINT:
      params[0] = INT_TO_FIXED(((GLuint *) p)[0]);
         case TYPE_ENUM16:
      params[0] = INT_TO_FIXED((GLint)(((GLenum16 *) p)[0]));
         case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
               case TYPE_INT64:
      params[0] = ((GLint64 *) p)[0];
         case TYPE_BOOLEAN:
      params[0] = BOOLEAN_TO_FIXED(((GLboolean*) p)[0]);
         case TYPE_UBYTE:
      params[0] = INT_TO_FIXED(((GLubyte *) p)[0]);
         case TYPE_SHORT:
      params[0] = INT_TO_FIXED(((GLshort *) p)[0]);
         case TYPE_MATRIX:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
   for (i = 0; i < 16; i++)
               case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
   params[0] = BOOLEAN_TO_FIXED((*(GLbitfield *) p >> shift) & 1);
         }
