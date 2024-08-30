   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
   * \file dlist.c
   * Display lists management functions.
   */
      #include "api_save.h"
   #include "api_arrayelt.h"
   #include "draw_validate.h"
   #include "arrayobj.h"
   #include "enums.h"
   #include "eval.h"
   #include "hash.h"
   #include "image.h"
   #include "light.h"
   #include "pack.h"
   #include "pbo.h"
   #include "teximage.h"
   #include "texobj.h"
   #include "varray.h"
   #include "glthread_marshal.h"
      #include "main/dispatch.h"
      #include "vbo/vbo_save.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_context.h"
   #include "state_tracker/st_cb_texture.h"
   #include "state_tracker/st_cb_bitmap.h"
   #include "state_tracker/st_sampler_view.h"
      static bool
   _mesa_glthread_should_execute_list(struct gl_context *ctx,
            /**
   * Flush vertices.
   *
   * \param ctx GL context.
   *
   * Checks if dd_function_table::SaveNeedFlush is marked to flush
   * stored (save) vertices, and calls vbo_save_SaveFlushVertices if so.
   */
   #define SAVE_FLUSH_VERTICES(ctx)                     \
      do {                                              \
      if (ctx->Driver.SaveNeedFlush)                 \
               /**
   * Macro to assert that the API call was made outside the
   * glBegin()/glEnd() pair, with return value.
   *
   * \param ctx GL context.
   * \param retval value to return value in case the assertion fails.
   */
   #define ASSERT_OUTSIDE_SAVE_BEGIN_END_WITH_RETVAL(ctx, retval)          \
      do {                                                                 \
      if (ctx->Driver.CurrentSavePrimitive <= PRIM_MAX) {               \
      _mesa_compile_error( ctx, GL_INVALID_OPERATION, "glBegin/End" ); \
               /**
   * Macro to assert that the API call was made outside the
   * glBegin()/glEnd() pair.
   *
   * \param ctx GL context.
   */
   #define ASSERT_OUTSIDE_SAVE_BEGIN_END(ctx)                              \
      do {                                                                 \
      if (ctx->Driver.CurrentSavePrimitive <= PRIM_MAX) {               \
      _mesa_compile_error( ctx, GL_INVALID_OPERATION, "glBegin/End" ); \
               /**
   * Macro to assert that the API call was made outside the
   * glBegin()/glEnd() pair and flush the vertices.
   *
   * \param ctx GL context.
   */
   #define ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx)                    \
      do {                                                                 \
      ASSERT_OUTSIDE_SAVE_BEGIN_END(ctx);                               \
            /**
   * Macro to assert that the API call was made outside the
   * glBegin()/glEnd() pair and flush the vertices, with return value.
   *
   * \param ctx GL context.
   * \param retval value to return value in case the assertion fails.
   */
   #define ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, retval) \
      do {                                                                 \
      ASSERT_OUTSIDE_SAVE_BEGIN_END_WITH_RETVAL(ctx, retval);           \
               /**
   * Display list opcodes.
   */
   typedef enum
   {
      OPCODE_INVALID = -1,         /* Force signed enum */
   OPCODE_ACCUM,
   OPCODE_ALPHA_FUNC,
   OPCODE_BIND_TEXTURE,
   OPCODE_BITMAP,
   OPCODE_BLEND_COLOR,
   OPCODE_BLEND_EQUATION,
   OPCODE_BLEND_EQUATION_SEPARATE,
            OPCODE_BLEND_EQUATION_I,
   OPCODE_BLEND_EQUATION_SEPARATE_I,
   OPCODE_BLEND_FUNC_I,
            OPCODE_CALL_LIST,
   OPCODE_CALL_LISTS,
   OPCODE_CLEAR,
   OPCODE_CLEAR_ACCUM,
   OPCODE_CLEAR_COLOR,
   OPCODE_CLEAR_DEPTH,
   OPCODE_CLEAR_INDEX,
   OPCODE_CLEAR_STENCIL,
   OPCODE_CLEAR_BUFFER_IV,
   OPCODE_CLEAR_BUFFER_UIV,
   OPCODE_CLEAR_BUFFER_FV,
   OPCODE_CLEAR_BUFFER_FI,
   OPCODE_CLIP_PLANE,
   OPCODE_COLOR_MASK,
   OPCODE_COLOR_MASK_INDEXED,
   OPCODE_COLOR_MATERIAL,
   OPCODE_COPY_PIXELS,
   OPCODE_COPY_TEX_IMAGE1D,
   OPCODE_COPY_TEX_IMAGE2D,
   OPCODE_COPY_TEX_SUB_IMAGE1D,
   OPCODE_COPY_TEX_SUB_IMAGE2D,
   OPCODE_COPY_TEX_SUB_IMAGE3D,
   OPCODE_CULL_FACE,
   OPCODE_DEPTH_FUNC,
   OPCODE_DEPTH_MASK,
   OPCODE_DEPTH_RANGE,
   OPCODE_DISABLE,
   OPCODE_DISABLE_INDEXED,
   OPCODE_DRAW_BUFFER,
   OPCODE_DRAW_PIXELS,
   OPCODE_ENABLE,
   OPCODE_ENABLE_INDEXED,
   OPCODE_EVALMESH1,
   OPCODE_EVALMESH2,
   OPCODE_FOG,
   OPCODE_FRONT_FACE,
   OPCODE_FRUSTUM,
   OPCODE_HINT,
   OPCODE_INDEX_MASK,
   OPCODE_INIT_NAMES,
   OPCODE_LIGHT,
   OPCODE_LIGHT_MODEL,
   OPCODE_LINE_STIPPLE,
   OPCODE_LINE_WIDTH,
   OPCODE_LIST_BASE,
   OPCODE_LOAD_IDENTITY,
   OPCODE_LOAD_MATRIX,
   OPCODE_LOAD_NAME,
   OPCODE_LOGIC_OP,
   OPCODE_MAP1,
   OPCODE_MAP2,
   OPCODE_MAPGRID1,
   OPCODE_MAPGRID2,
   OPCODE_MATRIX_MODE,
   OPCODE_MULT_MATRIX,
   OPCODE_ORTHO,
   OPCODE_PASSTHROUGH,
   OPCODE_PIXEL_MAP,
   OPCODE_PIXEL_TRANSFER,
   OPCODE_PIXEL_ZOOM,
   OPCODE_POINT_SIZE,
   OPCODE_POINT_PARAMETERS,
   OPCODE_POLYGON_MODE,
   OPCODE_POLYGON_STIPPLE,
   OPCODE_POLYGON_OFFSET,
   OPCODE_POP_ATTRIB,
   OPCODE_POP_MATRIX,
   OPCODE_POP_NAME,
   OPCODE_PRIORITIZE_TEXTURE,
   OPCODE_PUSH_ATTRIB,
   OPCODE_PUSH_MATRIX,
   OPCODE_PUSH_NAME,
   OPCODE_RASTER_POS,
   OPCODE_READ_BUFFER,
   OPCODE_ROTATE,
   OPCODE_SCALE,
   OPCODE_SCISSOR,
   OPCODE_SELECT_TEXTURE_SGIS,
   OPCODE_SELECT_TEXTURE_COORD_SET,
   OPCODE_SHADE_MODEL,
   OPCODE_STENCIL_FUNC,
   OPCODE_STENCIL_MASK,
   OPCODE_STENCIL_OP,
   OPCODE_TEXENV,
   OPCODE_TEXGEN,
   OPCODE_TEXPARAMETER,
   OPCODE_TEX_IMAGE1D,
   OPCODE_TEX_IMAGE2D,
   OPCODE_TEX_IMAGE3D,
   OPCODE_TEX_SUB_IMAGE1D,
   OPCODE_TEX_SUB_IMAGE2D,
   OPCODE_TEX_SUB_IMAGE3D,
   OPCODE_TRANSLATE,
   OPCODE_VIEWPORT,
   OPCODE_WINDOW_POS,
   /* ARB_viewport_array */
   OPCODE_VIEWPORT_ARRAY_V,
   OPCODE_VIEWPORT_INDEXED_F,
   OPCODE_VIEWPORT_INDEXED_FV,
   OPCODE_SCISSOR_ARRAY_V,
   OPCODE_SCISSOR_INDEXED,
   OPCODE_SCISSOR_INDEXED_V,
   OPCODE_DEPTH_ARRAY_V,
   OPCODE_DEPTH_INDEXED,
   /* GL_ARB_multitexture */
   OPCODE_ACTIVE_TEXTURE,
   /* GL_ARB_texture_compression */
   OPCODE_COMPRESSED_TEX_IMAGE_1D,
   OPCODE_COMPRESSED_TEX_IMAGE_2D,
   OPCODE_COMPRESSED_TEX_IMAGE_3D,
   OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D,
   OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D,
   OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D,
   /* GL_ARB_multisample */
   OPCODE_SAMPLE_COVERAGE,
   /* GL_ARB_window_pos */
   OPCODE_WINDOW_POS_ARB,
   /* GL_ARB_vertex_program */
   OPCODE_BIND_PROGRAM_ARB,
   OPCODE_PROGRAM_LOCAL_PARAMETER_ARB,
   /* GL_EXT_stencil_two_side */
   OPCODE_ACTIVE_STENCIL_FACE_EXT,
   /* GL_EXT_depth_bounds_test */
   OPCODE_DEPTH_BOUNDS_EXT,
   /* GL_ARB_vertex/fragment_program */
   OPCODE_PROGRAM_STRING_ARB,
   OPCODE_PROGRAM_ENV_PARAMETER_ARB,
   /* GL_ARB_occlusion_query */
   OPCODE_BEGIN_QUERY_ARB,
   OPCODE_END_QUERY_ARB,
   /* GL_ARB_draw_buffers */
   OPCODE_DRAW_BUFFERS_ARB,
   /* GL_ATI_fragment_shader */
   OPCODE_BIND_FRAGMENT_SHADER_ATI,
   OPCODE_SET_FRAGMENT_SHADER_CONSTANTS_ATI,
   /* OpenGL 2.0 */
   OPCODE_STENCIL_FUNC_SEPARATE,
   OPCODE_STENCIL_OP_SEPARATE,
   OPCODE_STENCIL_MASK_SEPARATE,
   /* GL_NV_primitive_restart */
   OPCODE_PRIMITIVE_RESTART_NV,
   /* GL_ARB_shader_objects */
   OPCODE_USE_PROGRAM,
   OPCODE_UNIFORM_1F,
   OPCODE_UNIFORM_2F,
   OPCODE_UNIFORM_3F,
   OPCODE_UNIFORM_4F,
   OPCODE_UNIFORM_1FV,
   OPCODE_UNIFORM_2FV,
   OPCODE_UNIFORM_3FV,
   OPCODE_UNIFORM_4FV,
   OPCODE_UNIFORM_1I,
   OPCODE_UNIFORM_2I,
   OPCODE_UNIFORM_3I,
   OPCODE_UNIFORM_4I,
   OPCODE_UNIFORM_1IV,
   OPCODE_UNIFORM_2IV,
   OPCODE_UNIFORM_3IV,
   OPCODE_UNIFORM_4IV,
   OPCODE_UNIFORM_MATRIX22,
   OPCODE_UNIFORM_MATRIX33,
   OPCODE_UNIFORM_MATRIX44,
   OPCODE_UNIFORM_MATRIX23,
   OPCODE_UNIFORM_MATRIX32,
   OPCODE_UNIFORM_MATRIX24,
   OPCODE_UNIFORM_MATRIX42,
   OPCODE_UNIFORM_MATRIX34,
            /* OpenGL 3.0 */
   OPCODE_UNIFORM_1UI,
   OPCODE_UNIFORM_2UI,
   OPCODE_UNIFORM_3UI,
   OPCODE_UNIFORM_4UI,
   OPCODE_UNIFORM_1UIV,
   OPCODE_UNIFORM_2UIV,
   OPCODE_UNIFORM_3UIV,
            /* GL_ARB_gpu_shader_fp64 */
   OPCODE_UNIFORM_1D,
   OPCODE_UNIFORM_2D,
   OPCODE_UNIFORM_3D,
   OPCODE_UNIFORM_4D,
   OPCODE_UNIFORM_1DV,
   OPCODE_UNIFORM_2DV,
   OPCODE_UNIFORM_3DV,
   OPCODE_UNIFORM_4DV,
   OPCODE_UNIFORM_MATRIX22D,
   OPCODE_UNIFORM_MATRIX33D,
   OPCODE_UNIFORM_MATRIX44D,
   OPCODE_UNIFORM_MATRIX23D,
   OPCODE_UNIFORM_MATRIX32D,
   OPCODE_UNIFORM_MATRIX24D,
   OPCODE_UNIFORM_MATRIX42D,
   OPCODE_UNIFORM_MATRIX34D,
            /* GL_ARB_gpu_shader_int64 */
   OPCODE_UNIFORM_1I64,
   OPCODE_UNIFORM_2I64,
   OPCODE_UNIFORM_3I64,
   OPCODE_UNIFORM_4I64,
   OPCODE_UNIFORM_1I64V,
   OPCODE_UNIFORM_2I64V,
   OPCODE_UNIFORM_3I64V,
   OPCODE_UNIFORM_4I64V,
   OPCODE_UNIFORM_1UI64,
   OPCODE_UNIFORM_2UI64,
   OPCODE_UNIFORM_3UI64,
   OPCODE_UNIFORM_4UI64,
   OPCODE_UNIFORM_1UI64V,
   OPCODE_UNIFORM_2UI64V,
   OPCODE_UNIFORM_3UI64V,
   OPCODE_UNIFORM_4UI64V,
   OPCODE_PROGRAM_UNIFORM_1I64,
   OPCODE_PROGRAM_UNIFORM_2I64,
   OPCODE_PROGRAM_UNIFORM_3I64,
   OPCODE_PROGRAM_UNIFORM_4I64,
   OPCODE_PROGRAM_UNIFORM_1I64V,
   OPCODE_PROGRAM_UNIFORM_2I64V,
   OPCODE_PROGRAM_UNIFORM_3I64V,
   OPCODE_PROGRAM_UNIFORM_4I64V,
   OPCODE_PROGRAM_UNIFORM_1UI64,
   OPCODE_PROGRAM_UNIFORM_2UI64,
   OPCODE_PROGRAM_UNIFORM_3UI64,
   OPCODE_PROGRAM_UNIFORM_4UI64,
   OPCODE_PROGRAM_UNIFORM_1UI64V,
   OPCODE_PROGRAM_UNIFORM_2UI64V,
   OPCODE_PROGRAM_UNIFORM_3UI64V,
            /* OpenGL 4.0 / GL_ARB_tessellation_shader */
   OPCODE_PATCH_PARAMETER_I,
   OPCODE_PATCH_PARAMETER_FV_INNER,
            /* OpenGL 4.2 / GL_ARB_separate_shader_objects */
   OPCODE_USE_PROGRAM_STAGES,
   OPCODE_PROGRAM_UNIFORM_1F,
   OPCODE_PROGRAM_UNIFORM_2F,
   OPCODE_PROGRAM_UNIFORM_3F,
   OPCODE_PROGRAM_UNIFORM_4F,
   OPCODE_PROGRAM_UNIFORM_1FV,
   OPCODE_PROGRAM_UNIFORM_2FV,
   OPCODE_PROGRAM_UNIFORM_3FV,
   OPCODE_PROGRAM_UNIFORM_4FV,
   OPCODE_PROGRAM_UNIFORM_1D,
   OPCODE_PROGRAM_UNIFORM_2D,
   OPCODE_PROGRAM_UNIFORM_3D,
   OPCODE_PROGRAM_UNIFORM_4D,
   OPCODE_PROGRAM_UNIFORM_1DV,
   OPCODE_PROGRAM_UNIFORM_2DV,
   OPCODE_PROGRAM_UNIFORM_3DV,
   OPCODE_PROGRAM_UNIFORM_4DV,
   OPCODE_PROGRAM_UNIFORM_1I,
   OPCODE_PROGRAM_UNIFORM_2I,
   OPCODE_PROGRAM_UNIFORM_3I,
   OPCODE_PROGRAM_UNIFORM_4I,
   OPCODE_PROGRAM_UNIFORM_1IV,
   OPCODE_PROGRAM_UNIFORM_2IV,
   OPCODE_PROGRAM_UNIFORM_3IV,
   OPCODE_PROGRAM_UNIFORM_4IV,
   OPCODE_PROGRAM_UNIFORM_1UI,
   OPCODE_PROGRAM_UNIFORM_2UI,
   OPCODE_PROGRAM_UNIFORM_3UI,
   OPCODE_PROGRAM_UNIFORM_4UI,
   OPCODE_PROGRAM_UNIFORM_1UIV,
   OPCODE_PROGRAM_UNIFORM_2UIV,
   OPCODE_PROGRAM_UNIFORM_3UIV,
   OPCODE_PROGRAM_UNIFORM_4UIV,
   OPCODE_PROGRAM_UNIFORM_MATRIX22F,
   OPCODE_PROGRAM_UNIFORM_MATRIX33F,
   OPCODE_PROGRAM_UNIFORM_MATRIX44F,
   OPCODE_PROGRAM_UNIFORM_MATRIX23F,
   OPCODE_PROGRAM_UNIFORM_MATRIX32F,
   OPCODE_PROGRAM_UNIFORM_MATRIX24F,
   OPCODE_PROGRAM_UNIFORM_MATRIX42F,
   OPCODE_PROGRAM_UNIFORM_MATRIX34F,
   OPCODE_PROGRAM_UNIFORM_MATRIX43F,
   OPCODE_PROGRAM_UNIFORM_MATRIX22D,
   OPCODE_PROGRAM_UNIFORM_MATRIX33D,
   OPCODE_PROGRAM_UNIFORM_MATRIX44D,
   OPCODE_PROGRAM_UNIFORM_MATRIX23D,
   OPCODE_PROGRAM_UNIFORM_MATRIX32D,
   OPCODE_PROGRAM_UNIFORM_MATRIX24D,
   OPCODE_PROGRAM_UNIFORM_MATRIX42D,
   OPCODE_PROGRAM_UNIFORM_MATRIX34D,
            /* GL_ARB_clip_control */
            /* GL_ARB_color_buffer_float */
            /* GL_EXT_framebuffer_blit */
            /* Vertex attributes -- fallback for when optimized display
   * list build isn't active.
   */
   OPCODE_ATTR_1F_NV,
   OPCODE_ATTR_2F_NV,
   OPCODE_ATTR_3F_NV,
   OPCODE_ATTR_4F_NV,
   OPCODE_ATTR_1F_ARB,
   OPCODE_ATTR_2F_ARB,
   OPCODE_ATTR_3F_ARB,
   OPCODE_ATTR_4F_ARB,
   OPCODE_ATTR_1I,
   OPCODE_ATTR_2I,
   OPCODE_ATTR_3I,
   OPCODE_ATTR_4I,
   OPCODE_ATTR_1D,
   OPCODE_ATTR_2D,
   OPCODE_ATTR_3D,
   OPCODE_ATTR_4D,
   OPCODE_ATTR_1UI64,
   OPCODE_MATERIAL,
   OPCODE_BEGIN,
   OPCODE_END,
   OPCODE_EVAL_C1,
   OPCODE_EVAL_C2,
   OPCODE_EVAL_P1,
            /* GL_EXT_provoking_vertex */
            /* GL_EXT_transform_feedback */
   OPCODE_BEGIN_TRANSFORM_FEEDBACK,
   OPCODE_END_TRANSFORM_FEEDBACK,
   OPCODE_BIND_TRANSFORM_FEEDBACK,
   OPCODE_PAUSE_TRANSFORM_FEEDBACK,
   OPCODE_RESUME_TRANSFORM_FEEDBACK,
            /* GL_EXT_texture_integer */
   OPCODE_CLEARCOLOR_I,
   OPCODE_CLEARCOLOR_UI,
   OPCODE_TEXPARAMETER_I,
            /* GL_EXT/ARB_instanced_arrays */
            /* GL_NV_texture_barrier */
            /* GL_ARB_sampler_object */
   OPCODE_BIND_SAMPLER,
   OPCODE_SAMPLER_PARAMETERIV,
   OPCODE_SAMPLER_PARAMETERFV,
   OPCODE_SAMPLER_PARAMETERIIV,
            /* ARB_compute_shader */
            /* GL_ARB_sync */
            /* GL_NV_conditional_render */
   OPCODE_BEGIN_CONDITIONAL_RENDER,
            /* ARB_timer_query */
            /* ARB_transform_feedback3 */
   OPCODE_BEGIN_QUERY_INDEXED,
   OPCODE_END_QUERY_INDEXED,
            /* ARB_transform_feedback_instanced */
   OPCODE_DRAW_TRANSFORM_FEEDBACK_INSTANCED,
            /* ARB_uniform_buffer_object */
            /* ARB_shader_subroutines */
            /* EXT_polygon_offset_clamp */
            /* EXT_window_rectangles */
            /* NV_conservative_raster */
            /* NV_conservative_raster_dilate */
            /* NV_conservative_raster_pre_snap_triangles */
            /* EXT_direct_state_access */
   OPCODE_MATRIX_LOAD,
   OPCODE_MATRIX_MULT,
   OPCODE_MATRIX_ROTATE,
   OPCODE_MATRIX_SCALE,
   OPCODE_MATRIX_TRANSLATE,
   OPCODE_MATRIX_LOAD_IDENTITY,
   OPCODE_MATRIX_ORTHO,
   OPCODE_MATRIX_FRUSTUM,
   OPCODE_MATRIX_PUSH,
   OPCODE_MATRIX_POP,
   OPCODE_TEXTUREPARAMETER_F,
   OPCODE_TEXTUREPARAMETER_I,
   OPCODE_TEXTUREPARAMETER_II,
   OPCODE_TEXTUREPARAMETER_IUI,
   OPCODE_TEXTURE_IMAGE1D,
   OPCODE_TEXTURE_IMAGE2D,
   OPCODE_TEXTURE_IMAGE3D,
   OPCODE_TEXTURE_SUB_IMAGE1D,
   OPCODE_TEXTURE_SUB_IMAGE2D,
   OPCODE_TEXTURE_SUB_IMAGE3D,
   OPCODE_COPY_TEXTURE_IMAGE1D,
   OPCODE_COPY_TEXTURE_IMAGE2D,
   OPCODE_COPY_TEXTURE_SUB_IMAGE1D,
   OPCODE_COPY_TEXTURE_SUB_IMAGE2D,
   OPCODE_COPY_TEXTURE_SUB_IMAGE3D,
   OPCODE_BIND_MULTITEXTURE,
   OPCODE_MULTITEXPARAMETER_F,
   OPCODE_MULTITEXPARAMETER_I,
   OPCODE_MULTITEXPARAMETER_II,
   OPCODE_MULTITEXPARAMETER_IUI,
   OPCODE_MULTITEX_IMAGE1D,
   OPCODE_MULTITEX_IMAGE2D,
   OPCODE_MULTITEX_IMAGE3D,
   OPCODE_MULTITEX_SUB_IMAGE1D,
   OPCODE_MULTITEX_SUB_IMAGE2D,
   OPCODE_MULTITEX_SUB_IMAGE3D,
   OPCODE_COPY_MULTITEX_IMAGE1D,
   OPCODE_COPY_MULTITEX_IMAGE2D,
   OPCODE_COPY_MULTITEX_SUB_IMAGE1D,
   OPCODE_COPY_MULTITEX_SUB_IMAGE2D,
   OPCODE_COPY_MULTITEX_SUB_IMAGE3D,
   OPCODE_MULTITEXENV,
   OPCODE_COMPRESSED_TEXTURE_IMAGE_1D,
   OPCODE_COMPRESSED_TEXTURE_IMAGE_2D,
   OPCODE_COMPRESSED_TEXTURE_IMAGE_3D,
   OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_1D,
   OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_2D,
   OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_3D,
   OPCODE_COMPRESSED_MULTITEX_IMAGE_1D,
   OPCODE_COMPRESSED_MULTITEX_IMAGE_2D,
   OPCODE_COMPRESSED_MULTITEX_IMAGE_3D,
   OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_1D,
   OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_2D,
   OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_3D,
   OPCODE_NAMED_PROGRAM_STRING,
            /* GL_ARB_ES3_2_compatibility */
            OPCODE_VERTEX_LIST,
   OPCODE_VERTEX_LIST_LOOPBACK,
            /* The following three are meta instructions */
   OPCODE_ERROR,                /* raise compiled-in error */
   OPCODE_CONTINUE,
      } OpCode;
         typedef union gl_dlist_node Node;
         /** How many 4-byte dwords to store a pointer */
   #define POINTER_DWORDS (sizeof(void *) / 4)
      /* We want to keep sizeof(union gl_dlist_node) == 4 to minimize
   * space for display lists.  The following types and functions are
   * used to help store 4- and 8-byte pointers in 1 or 2 dlist_nodes.
   */
   union pointer
   {
      void *ptr;
      };
         /**
   * Save a 4 or 8-byte pointer at dest (and dest+1).
   */
   static inline void
   save_pointer(Node *dest, void *src)
   {
      union pointer p;
            STATIC_ASSERT(POINTER_DWORDS == 1 || POINTER_DWORDS == 2);
                     for (i = 0; i < POINTER_DWORDS; i++)
      }
         /**
   * Retrieve a 4 or 8-byte pointer from node (node+1).
   */
   static inline void *
   get_pointer(const Node *node)
   {
      union pointer p;
            for (i = 0; i < POINTER_DWORDS; i++)
               }
         /**
   * Used to store a 64-bit uint in a pair of "Nodes" for the sake of 32-bit
   * environment.
   */
   union uint64_pair
   {
      GLuint64 uint64;
      };
         union float64_pair
   {
      GLdouble d;
      };
      union int64_pair
   {
      GLint64 int64;
      };
      #define ASSIGN_DOUBLE_TO_NODES(n, idx, value)                              \
      do {                                                                    \
      union float64_pair tmp;                                              \
   tmp.d = value;                                                       \
   n[idx].ui = tmp.uint32[0];                                           \
            #define ASSIGN_UINT64_TO_NODES(n, idx, value)                              \
      do {                                                                    \
      union uint64_pair tmp;                                               \
   tmp.uint64 = value;                                                  \
   n[idx].ui = tmp.uint32[0];                                           \
            #define ASSIGN_INT64_TO_NODES(n, idx, value)                               \
      do {                                                                    \
      union int64_pair tmp;                                                \
   tmp.int64 = value;                                                   \
   n[idx].i = tmp.int32[0];                                             \
            /**
   * How many nodes to allocate at a time.  Note that bulk vertex data
   * from glBegin/glVertex/glEnd primitives will typically wind up in
   * a VBO, and not directly in the display list itself.
   */
   #define BLOCK_SIZE 256
         void mesa_print_display_list(GLuint list);
         /**
   * Called by display list code when a display list is being deleted.
   */
   static void
   vbo_destroy_vertex_list(struct gl_context *ctx, struct vbo_save_vertex_list *node)
   {
               if (_mesa_bufferobj_mapped(bo, MAP_INTERNAL))
            for (gl_vertex_processing_mode mode = VP_MODE_FF; mode < VP_MODE_MAX; ++mode) {
      _mesa_reference_vao(ctx, &node->cold->VAO[mode], NULL);
   if (node->private_refcount[mode]) {
      assert(node->private_refcount[mode] > 0);
   p_atomic_add(&node->state[mode]->reference.count,
      }
               if (node->modes) {
      free(node->modes);
               _mesa_reference_buffer_object(ctx, &node->cold->ib.obj, NULL);
   free(node->cold->current_data);
            free(node->cold->prims);
      }
      static void
   vbo_print_vertex_list(struct gl_context *ctx, struct vbo_save_vertex_list *node, OpCode op, FILE *f)
   {
      GLuint i;
   struct gl_buffer_object *buffer = node->cold->VAO[0]->BufferBinding[0].BufferObj;
   const GLuint vertex_size = _vbo_save_get_stride(node)/sizeof(GLfloat);
            const char *label[] = {
                  fprintf(f, "%s, %u vertices, %d primitives, %d vertsize, "
         "buffer %p\n",
   label[op - OPCODE_VERTEX_LIST],
            for (i = 0; i < node->cold->prim_count; i++) {
      struct _mesa_prim *prim = &node->cold->prims[i];
   fprintf(f, "   prim %d: %s %d..%d %s %s\n",
         i,
   _mesa_lookup_prim_by_nr(prim->mode),
   prim->start,
   prim->start + prim->count,
         }
         static inline
   Node *get_list_head(struct gl_context *ctx, struct gl_display_list *dlist)
   {
      return dlist->small_list ?
      &ctx->Shared->small_dlist_store.ptr[dlist->start] :
   }
         /**
   * Allocate a gl_display_list object with an initial block of storage.
   * \param count  how many display list nodes/tokens to allocate
   */
   static struct gl_display_list *
   make_list(GLuint name, GLuint count)
   {
      struct gl_display_list *dlist = CALLOC_STRUCT(gl_display_list);
   dlist->Name = name;
   dlist->Head = malloc(sizeof(Node) * count);
   dlist->Head[0].opcode = OPCODE_END_OF_LIST;
      }
         /**
   * Lookup function to just encapsulate casting.
   */
   struct gl_display_list *
   _mesa_lookup_list(struct gl_context *ctx, GLuint list, bool locked)
   {
      return (struct gl_display_list *)
      }
         /**
   * Delete the named display list, but don't remove from hash table.
   * \param dlist - display list pointer
   */
   void
   _mesa_delete_list(struct gl_context *ctx, struct gl_display_list *dlist)
   {
                        if (!n) {
      free(dlist->Label);
   FREE(dlist);
               while (1) {
               switch (opcode) {
            case OPCODE_MAP1:
      free(get_pointer(&n[6]));
      case OPCODE_MAP2:
      free(get_pointer(&n[10]));
      case OPCODE_CALL_LISTS:
      free(get_pointer(&n[3]));
      case OPCODE_DRAW_PIXELS:
      free(get_pointer(&n[5]));
      case OPCODE_BITMAP: {
      struct pipe_resource *tex = get_pointer(&n[7]);
   pipe_resource_reference(&tex, NULL);
      }
   case OPCODE_POLYGON_STIPPLE:
      free(get_pointer(&n[1]));
      case OPCODE_TEX_IMAGE1D:
      free(get_pointer(&n[8]));
      case OPCODE_TEX_IMAGE2D:
      free(get_pointer(&n[9]));
      case OPCODE_TEX_IMAGE3D:
      free(get_pointer(&n[10]));
      case OPCODE_TEX_SUB_IMAGE1D:
      free(get_pointer(&n[7]));
      case OPCODE_TEX_SUB_IMAGE2D:
      free(get_pointer(&n[9]));
      case OPCODE_TEX_SUB_IMAGE3D:
      free(get_pointer(&n[11]));
      case OPCODE_COMPRESSED_TEX_IMAGE_1D:
      free(get_pointer(&n[7]));
      case OPCODE_COMPRESSED_TEX_IMAGE_2D:
      free(get_pointer(&n[8]));
      case OPCODE_COMPRESSED_TEX_IMAGE_3D:
      free(get_pointer(&n[9]));
      case OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D:
      free(get_pointer(&n[7]));
      case OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D:
      free(get_pointer(&n[9]));
      case OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D:
      free(get_pointer(&n[11]));
      case OPCODE_PROGRAM_STRING_ARB:
      free(get_pointer(&n[4]));      /* program string */
      case OPCODE_UNIFORM_1FV:
   case OPCODE_UNIFORM_2FV:
   case OPCODE_UNIFORM_3FV:
   case OPCODE_UNIFORM_4FV:
   case OPCODE_UNIFORM_1DV:
   case OPCODE_UNIFORM_2DV:
   case OPCODE_UNIFORM_3DV:
   case OPCODE_UNIFORM_4DV:
   case OPCODE_UNIFORM_1IV:
   case OPCODE_UNIFORM_2IV:
   case OPCODE_UNIFORM_3IV:
   case OPCODE_UNIFORM_4IV:
   case OPCODE_UNIFORM_1UIV:
   case OPCODE_UNIFORM_2UIV:
   case OPCODE_UNIFORM_3UIV:
   case OPCODE_UNIFORM_4UIV:
   case OPCODE_UNIFORM_1I64V:
   case OPCODE_UNIFORM_2I64V:
   case OPCODE_UNIFORM_3I64V:
   case OPCODE_UNIFORM_4I64V:
   case OPCODE_UNIFORM_1UI64V:
   case OPCODE_UNIFORM_2UI64V:
   case OPCODE_UNIFORM_3UI64V:
   case OPCODE_UNIFORM_4UI64V:
      free(get_pointer(&n[3]));
      case OPCODE_UNIFORM_MATRIX22:
   case OPCODE_UNIFORM_MATRIX33:
   case OPCODE_UNIFORM_MATRIX44:
   case OPCODE_UNIFORM_MATRIX24:
   case OPCODE_UNIFORM_MATRIX42:
   case OPCODE_UNIFORM_MATRIX23:
   case OPCODE_UNIFORM_MATRIX32:
   case OPCODE_UNIFORM_MATRIX34:
   case OPCODE_UNIFORM_MATRIX43:
   case OPCODE_UNIFORM_MATRIX22D:
   case OPCODE_UNIFORM_MATRIX33D:
   case OPCODE_UNIFORM_MATRIX44D:
   case OPCODE_UNIFORM_MATRIX24D:
   case OPCODE_UNIFORM_MATRIX42D:
   case OPCODE_UNIFORM_MATRIX23D:
   case OPCODE_UNIFORM_MATRIX32D:
   case OPCODE_UNIFORM_MATRIX34D:
   case OPCODE_UNIFORM_MATRIX43D:
      free(get_pointer(&n[4]));
      case OPCODE_PROGRAM_UNIFORM_1FV:
   case OPCODE_PROGRAM_UNIFORM_2FV:
   case OPCODE_PROGRAM_UNIFORM_3FV:
   case OPCODE_PROGRAM_UNIFORM_4FV:
   case OPCODE_PROGRAM_UNIFORM_1DV:
   case OPCODE_PROGRAM_UNIFORM_2DV:
   case OPCODE_PROGRAM_UNIFORM_3DV:
   case OPCODE_PROGRAM_UNIFORM_4DV:
   case OPCODE_PROGRAM_UNIFORM_1IV:
   case OPCODE_PROGRAM_UNIFORM_2IV:
   case OPCODE_PROGRAM_UNIFORM_3IV:
   case OPCODE_PROGRAM_UNIFORM_4IV:
   case OPCODE_PROGRAM_UNIFORM_1UIV:
   case OPCODE_PROGRAM_UNIFORM_2UIV:
   case OPCODE_PROGRAM_UNIFORM_3UIV:
   case OPCODE_PROGRAM_UNIFORM_4UIV:
   case OPCODE_PROGRAM_UNIFORM_1I64V:
   case OPCODE_PROGRAM_UNIFORM_2I64V:
   case OPCODE_PROGRAM_UNIFORM_3I64V:
   case OPCODE_PROGRAM_UNIFORM_4I64V:
   case OPCODE_PROGRAM_UNIFORM_1UI64V:
   case OPCODE_PROGRAM_UNIFORM_2UI64V:
   case OPCODE_PROGRAM_UNIFORM_3UI64V:
   case OPCODE_PROGRAM_UNIFORM_4UI64V:
      free(get_pointer(&n[4]));
      case OPCODE_PROGRAM_UNIFORM_MATRIX22F:
   case OPCODE_PROGRAM_UNIFORM_MATRIX33F:
   case OPCODE_PROGRAM_UNIFORM_MATRIX44F:
   case OPCODE_PROGRAM_UNIFORM_MATRIX24F:
   case OPCODE_PROGRAM_UNIFORM_MATRIX42F:
   case OPCODE_PROGRAM_UNIFORM_MATRIX23F:
   case OPCODE_PROGRAM_UNIFORM_MATRIX32F:
   case OPCODE_PROGRAM_UNIFORM_MATRIX34F:
   case OPCODE_PROGRAM_UNIFORM_MATRIX43F:
   case OPCODE_PROGRAM_UNIFORM_MATRIX22D:
   case OPCODE_PROGRAM_UNIFORM_MATRIX33D:
   case OPCODE_PROGRAM_UNIFORM_MATRIX44D:
   case OPCODE_PROGRAM_UNIFORM_MATRIX24D:
   case OPCODE_PROGRAM_UNIFORM_MATRIX42D:
   case OPCODE_PROGRAM_UNIFORM_MATRIX23D:
   case OPCODE_PROGRAM_UNIFORM_MATRIX32D:
   case OPCODE_PROGRAM_UNIFORM_MATRIX34D:
   case OPCODE_PROGRAM_UNIFORM_MATRIX43D:
      free(get_pointer(&n[5]));
      case OPCODE_PIXEL_MAP:
      free(get_pointer(&n[3]));
      case OPCODE_VIEWPORT_ARRAY_V:
   case OPCODE_SCISSOR_ARRAY_V:
   case OPCODE_DEPTH_ARRAY_V:
   case OPCODE_UNIFORM_SUBROUTINES:
   case OPCODE_WINDOW_RECTANGLES:
      free(get_pointer(&n[3]));
      case OPCODE_TEXTURE_IMAGE1D:
   case OPCODE_MULTITEX_IMAGE1D:
      free(get_pointer(&n[9]));
      case OPCODE_TEXTURE_IMAGE2D:
   case OPCODE_MULTITEX_IMAGE2D:
      free(get_pointer(&n[10]));
      case OPCODE_TEXTURE_IMAGE3D:
   case OPCODE_MULTITEX_IMAGE3D:
      free(get_pointer(&n[11]));
      case OPCODE_TEXTURE_SUB_IMAGE1D:
   case OPCODE_MULTITEX_SUB_IMAGE1D:
   case OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_1D:
   case OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_1D:
      free(get_pointer(&n[8]));
      case OPCODE_TEXTURE_SUB_IMAGE2D:
   case OPCODE_MULTITEX_SUB_IMAGE2D:
   case OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_2D:
   case OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_2D:
      free(get_pointer(&n[10]));
      case OPCODE_TEXTURE_SUB_IMAGE3D:
   case OPCODE_MULTITEX_SUB_IMAGE3D:
   case OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_3D:
   case OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_3D:
      free(get_pointer(&n[12]));
      case OPCODE_COMPRESSED_TEXTURE_IMAGE_1D:
   case OPCODE_COMPRESSED_MULTITEX_IMAGE_1D:
      free(get_pointer(&n[8]));
      case OPCODE_COMPRESSED_TEXTURE_IMAGE_2D:
   case OPCODE_COMPRESSED_MULTITEX_IMAGE_2D:
      free(get_pointer(&n[9]));
      case OPCODE_COMPRESSED_TEXTURE_IMAGE_3D:
   case OPCODE_COMPRESSED_MULTITEX_IMAGE_3D:
      free(get_pointer(&n[10]));
      case OPCODE_NAMED_PROGRAM_STRING:
      free(get_pointer(&n[5]));
      case OPCODE_VERTEX_LIST:
   case OPCODE_VERTEX_LIST_LOOPBACK:
   case OPCODE_VERTEX_LIST_COPY_CURRENT:
      vbo_destroy_vertex_list(ctx, (struct vbo_save_vertex_list *) &n[0]);
      case OPCODE_CONTINUE:
      n = (Node *) get_pointer(&n[1]);
   assert (!dlist->small_list);
   free(block);
   block = n;
      case OPCODE_END_OF_LIST:
      if (dlist->small_list) {
      unsigned start = dlist->start;
   for (int i = 0; i < dlist->count; i++) {
      util_idalloc_free(&ctx->Shared->small_dlist_store.free_idx,
         } else {
         }
   free(dlist->Label);
   free(dlist);
      default:
      /* just increment 'n' pointer, below */
            assert(n[0].InstSize > 0);
         }
         /**
   * Destroy a display list and remove from hash table.
   * \param list - display list number
   */
   static void
   destroy_list(struct gl_context *ctx, GLuint list)
   {
               if (list == 0)
            dlist = _mesa_lookup_list(ctx, list, true);
   if (!dlist)
            _mesa_delete_list(ctx, dlist);
      }
         /**
   * Wrapper for _mesa_unpack_image/bitmap() that handles pixel buffer objects.
   * If width < 0 or height < 0 or format or type are invalid we'll just
   * return NULL.  We will not generate an error since OpenGL command
   * arguments aren't error-checked until the command is actually executed
   * (not when they're compiled).
   * But if we run out of memory, GL_OUT_OF_MEMORY will be recorded.
   */
   static GLvoid *
   unpack_image(struct gl_context *ctx, GLuint dimensions,
               GLsizei width, GLsizei height, GLsizei depth,
   {
      if (width <= 0 || height <= 0) {
                  if (_mesa_bytes_per_pixel(format, type) < 0) {
      /* bad format and/or type */
               if (!unpack->BufferObj) {
      /* no PBO */
            image = _mesa_unpack_image(dimensions, width, height, depth,
         if (pixels && !image) {
         }
      }
   else if (_mesa_validate_pbo_access(dimensions, unpack, width, height,
            const GLubyte *map, *src;
            map = (GLubyte *)
      _mesa_bufferobj_map_range(ctx, 0, unpack->BufferObj->Size,
            if (!map) {
      /* unable to map src buffer! */
   _mesa_error(ctx, GL_INVALID_OPERATION, "unable to map PBO");
               src = ADD_POINTERS(map, pixels);
   image = _mesa_unpack_image(dimensions, width, height, depth,
                     if (!image) {
         }
               /* bad access! */
   _mesa_error(ctx, GL_INVALID_OPERATION, "invalid PBO access");
      }
         /** Return copy of memory */
   static void *
   memdup(const void *src, GLsizei bytes)
   {
      void *b = bytes >= 0 ? malloc(bytes) : NULL;
   if (b)
            }
         /**
   * Allocate space for a display list instruction (opcode + payload space).
   * \param opcode  the instruction opcode (OPCODE_* value)
   * \param bytes   instruction payload size (not counting opcode)
   * \param align8  does the payload need to be 8-byte aligned?
   *                This is only relevant in 64-bit environments.
   * \return pointer to allocated memory (the payload will be at pointer+1)
   */
   static Node *
   dlist_alloc(struct gl_context *ctx, OpCode opcode, GLuint bytes, bool align8)
   {
      const GLuint numNodes = 1 + (bytes + sizeof(Node) - 1) / sizeof(Node);
                     /* If this node needs to start on an 8-byte boundary, pad the last node. */
   if (sizeof(void *) == 8 && align8 &&
      ctx->ListState.CurrentPos % 2 == 1) {
   Node *last = ctx->ListState.CurrentBlock + ctx->ListState.CurrentPos -
         last->InstSize++;
               if (ctx->ListState.CurrentPos + numNodes + contNodes > BLOCK_SIZE) {
      /* This block is full.  Allocate a new block and chain to it */
   Node *newblock;
   Node *n = ctx->ListState.CurrentBlock + ctx->ListState.CurrentPos;
   n[0].opcode = OPCODE_CONTINUE;
   newblock = malloc(sizeof(Node) * BLOCK_SIZE);
   if (!newblock) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Building display list");
               /* a fresh block should be 8-byte aligned on 64-bit systems */
            save_pointer(&n[1], newblock);
   ctx->ListState.CurrentBlock = newblock;
               Node *n = ctx->ListState.CurrentBlock + ctx->ListState.CurrentPos;
            n[0].opcode = opcode;
   n[0].InstSize = numNodes;
               }
         void *
   _mesa_dlist_alloc_vertex_list(struct gl_context *ctx, bool copy_to_current)
   {
      Node *n =  dlist_alloc(ctx,
                           if (!n)
            /* Clear all nodes except the header */
   memset(n + 1, 0, sizeof(struct vbo_save_vertex_list) - sizeof(Node));
      }
         /**
   * Allocate space for a display list instruction.  The space is basically
   * an array of Nodes where node[0] holds the opcode, node[1] is the first
   * function parameter, node[2] is the second parameter, etc.
   *
   * \param opcode  one of OPCODE_x
   * \param nparams  number of function parameters
   * \return  pointer to start of instruction space
   */
   static inline Node *
   alloc_instruction(struct gl_context *ctx, OpCode opcode, GLuint nparams)
   {
         }
         /*
   * Display List compilation functions
   */
   void GLAPIENTRY
   save_Accum(GLenum op, GLfloat value)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ACCUM, 2);
   if (n) {
      n[1].e = op;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_AlphaFunc(GLenum func, GLclampf ref)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ALPHA_FUNC, 2);
   if (n) {
      n[1].e = func;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_BindTexture(GLenum target, GLuint texture)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BIND_TEXTURE, 2);
   if (n) {
      n[1].e = target;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Bitmap(GLsizei width, GLsizei height,
               {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
            if (width > 0 && height > 0) {
               if (!tex) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glNewList -> glBitmap");
                  n = alloc_instruction(ctx, OPCODE_BITMAP, 6 + POINTER_DWORDS);
   if (!n) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glNewList -> glBitmap (3)");
   pipe_resource_reference(&tex, NULL);
               n[1].i = (GLint) width;
   n[2].i = (GLint) height;
   n[3].f = xorig;
   n[4].f = yorig;
   n[5].f = xmove;
   n[6].f = ymove;
            if (ctx->ExecuteFlag) {
      ASSERT_OUTSIDE_BEGIN_END(ctx);
         }
         void GLAPIENTRY
   save_BlendEquation(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_EQUATION, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_BlendEquationSeparate(GLenum modeRGB, GLenum modeA)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_EQUATION_SEPARATE, 2);
   if (n) {
      n[1].e = modeRGB;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_BlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_FUNC_SEPARATE, 4);
   if (n) {
      n[1].e = sfactorRGB;
   n[2].e = dfactorRGB;
   n[3].e = sfactorA;
      }
   if (ctx->ExecuteFlag) {
      CALL_BlendFuncSeparate(ctx->Dispatch.Exec,
         }
         void GLAPIENTRY
   save_BlendFunc(GLenum srcfactor, GLenum dstfactor)
   {
         }
         void GLAPIENTRY
   save_BlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_COLOR, 4);
   if (n) {
      n[1].f = red;
   n[2].f = green;
   n[3].f = blue;
      }
   if (ctx->ExecuteFlag) {
            }
      /* GL_ARB_draw_buffers_blend */
   void GLAPIENTRY
   save_BlendFuncSeparateiARB(GLuint buf, GLenum sfactorRGB, GLenum dfactorRGB,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_FUNC_SEPARATE_I, 5);
   if (n) {
      n[1].ui = buf;
   n[2].e = sfactorRGB;
   n[3].e = dfactorRGB;
   n[4].e = sfactorA;
      }
   if (ctx->ExecuteFlag) {
      CALL_BlendFuncSeparateiARB(ctx->Dispatch.Exec, (buf, sfactorRGB, dfactorRGB,
         }
      /* GL_ARB_draw_buffers_blend */
   void GLAPIENTRY
   save_BlendFunciARB(GLuint buf, GLenum sfactor, GLenum dfactor)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_FUNC_I, 3);
   if (n) {
      n[1].ui = buf;
   n[2].e = sfactor;
      }
   if (ctx->ExecuteFlag) {
            }
      /* GL_ARB_draw_buffers_blend */
   void GLAPIENTRY
   save_BlendEquationiARB(GLuint buf, GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_EQUATION_I, 2);
   if (n) {
      n[1].ui = buf;
      }
   if (ctx->ExecuteFlag) {
            }
      /* GL_ARB_draw_buffers_blend */
   void GLAPIENTRY
   save_BlendEquationSeparateiARB(GLuint buf, GLenum modeRGB, GLenum modeA)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_EQUATION_SEPARATE_I, 3);
   if (n) {
      n[1].ui = buf;
   n[2].e = modeRGB;
      }
   if (ctx->ExecuteFlag) {
            }
         /* GL_ARB_draw_instanced. */
   void GLAPIENTRY
   save_DrawArraysInstanced(UNUSED GLenum mode,
                     {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
      }
      void GLAPIENTRY
   save_DrawElementsInstanced(UNUSED GLenum mode,
                           {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
      }
      void GLAPIENTRY
   save_DrawElementsInstancedBaseVertex(UNUSED GLenum mode,
                                 {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
      }
      /* GL_ARB_base_instance. */
   void GLAPIENTRY
   save_DrawArraysInstancedBaseInstance(UNUSED GLenum mode,
                           {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
      }
      void GLAPIENTRY
   save_DrawElementsInstancedBaseInstance(UNUSED GLenum mode,
                                 {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
      }
      void GLAPIENTRY
   save_DrawElementsInstancedBaseVertexBaseInstance(UNUSED GLenum mode,
                                       {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
      }
      void GLAPIENTRY
   save_DrawArraysIndirect(UNUSED GLenum mode,
         {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
      }
      void GLAPIENTRY
   save_DrawElementsIndirect(UNUSED GLenum mode,
               {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
      }
      void GLAPIENTRY
   save_MultiDrawArraysIndirect(UNUSED GLenum mode,
                     {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
      }
      void GLAPIENTRY
   save_MultiDrawElementsIndirect(UNUSED GLenum mode,
                           {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
      }
      /**
   * While building a display list we cache some OpenGL state.
   * Under some circumstances we need to invalidate that state (immediately
   * when we start compiling a list, or after glCallList(s)).
   */
   static void
   invalidate_saved_current_state(struct gl_context *ctx)
   {
               for (i = 0; i < VERT_ATTRIB_MAX; i++)
            for (i = 0; i < MAT_ATTRIB_MAX; i++)
            /* Loopback usage applies recursively, so remember this state */
   bool use_loopback = ctx->ListState.Current.UseLoopback;
   memset(&ctx->ListState.Current, 0, sizeof ctx->ListState.Current);
               }
         static void GLAPIENTRY
   save_CallList(GLuint list)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
            n = alloc_instruction(ctx, OPCODE_CALL_LIST, 1);
   if (n) {
                  /* After this, we don't know what state we're in.  Invalidate all
   * cached information previously gathered:
   */
            if (ctx->ExecuteFlag) {
            }
         static void GLAPIENTRY
   save_CallLists(GLsizei num, GLenum type, const GLvoid * lists)
   {
      GET_CURRENT_CONTEXT(ctx);
   unsigned type_size;
   Node *n;
                     switch (type) {
   case GL_BYTE:
   case GL_UNSIGNED_BYTE:
      type_size = 1;
      case GL_SHORT:
   case GL_UNSIGNED_SHORT:
   case GL_2_BYTES:
      type_size = 2;
      case GL_3_BYTES:
      type_size = 3;
      case GL_INT:
   case GL_UNSIGNED_INT:
   case GL_FLOAT:
   case GL_4_BYTES:
      type_size = 4;
      default:
                  if (num > 0 && type_size > 0) {
      /* create a copy of the array of list IDs to save in the display list */
      } else {
                  n = alloc_instruction(ctx, OPCODE_CALL_LISTS, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = num;
   n[2].e = type;
               /* After this, we don't know what state we're in.  Invalidate all
   * cached information previously gathered:
   */
            if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Clear(GLbitfield mask)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_BUFFER_IV, 6);
   if (n) {
      n[1].e = buffer;
   n[2].i = drawbuffer;
   n[3].i = value[0];
   if (buffer == GL_COLOR) {
      n[4].i = value[1];
   n[5].i = value[2];
      }
   else {
      n[4].i = 0;
   n[5].i = 0;
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_BUFFER_UIV, 6);
   if (n) {
      n[1].e = buffer;
   n[2].i = drawbuffer;
   n[3].ui = value[0];
   if (buffer == GL_COLOR) {
      n[4].ui = value[1];
   n[5].ui = value[2];
      }
   else {
      n[4].ui = 0;
   n[5].ui = 0;
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_BUFFER_FV, 6);
   if (n) {
      n[1].e = buffer;
   n[2].i = drawbuffer;
   n[3].f = value[0];
   if (buffer == GL_COLOR) {
      n[4].f = value[1];
   n[5].f = value[2];
      }
   else {
      n[4].f = 0.0F;
   n[5].f = 0.0F;
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ClearBufferfi(GLenum buffer, GLint drawbuffer,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_BUFFER_FI, 4);
   if (n) {
      n[1].e = buffer;
   n[2].i = drawbuffer;
   n[3].f = depth;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_ACCUM, 4);
   if (n) {
      n[1].f = red;
   n[2].f = green;
   n[3].f = blue;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_COLOR, 4);
   if (n) {
      n[1].f = red;
   n[2].f = green;
   n[3].f = blue;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ClearDepth(GLclampd depth)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_DEPTH, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ClearIndex(GLfloat c)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_INDEX, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ClearStencil(GLint s)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_STENCIL, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ClipPlane(GLenum plane, const GLdouble * equ)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLIP_PLANE, 5);
   if (n) {
      n[1].e = plane;
   n[2].f = (GLfloat) equ[0];
   n[3].f = (GLfloat) equ[1];
   n[4].f = (GLfloat) equ[2];
      }
   if (ctx->ExecuteFlag) {
            }
            void GLAPIENTRY
   save_ColorMask(GLboolean red, GLboolean green,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COLOR_MASK, 4);
   if (n) {
      n[1].b = red;
   n[2].b = green;
   n[3].b = blue;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ColorMaski(GLuint buf, GLboolean red, GLboolean green,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COLOR_MASK_INDEXED, 5);
   if (n) {
      n[1].ui = buf;
   n[2].b = red;
   n[3].b = green;
   n[4].b = blue;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ColorMaterial(GLenum face, GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
            n = alloc_instruction(ctx, OPCODE_COLOR_MATERIAL, 2);
   if (n) {
      n[1].e = face;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_CopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_PIXELS, 5);
   if (n) {
      n[1].i = x;
   n[2].i = y;
   n[3].i = (GLint) width;
   n[4].i = (GLint) height;
      }
   if (ctx->ExecuteFlag) {
            }
            void GLAPIENTRY
   save_CopyTexImage1D(GLenum target, GLint level, GLenum internalformat,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEX_IMAGE1D, 7);
   if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].e = internalformat;
   n[4].i = x;
   n[5].i = y;
   n[6].i = width;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyTexImage1D(ctx->Dispatch.Exec, (target, level, internalformat,
         }
         void GLAPIENTRY
   save_CopyTexImage2D(GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEX_IMAGE2D, 8);
   if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].e = internalformat;
   n[4].i = x;
   n[5].i = y;
   n[6].i = width;
   n[7].i = height;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyTexImage2D(ctx->Dispatch.Exec, (target, level, internalformat,
         }
            void GLAPIENTRY
   save_CopyTexSubImage1D(GLenum target, GLint level,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEX_SUB_IMAGE1D, 6);
   if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = xoffset;
   n[4].i = x;
   n[5].i = y;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyTexSubImage1D(ctx->Dispatch.Exec,
         }
         void GLAPIENTRY
   save_CopyTexSubImage2D(GLenum target, GLint level,
               {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEX_SUB_IMAGE2D, 8);
   if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = xoffset;
   n[4].i = yoffset;
   n[5].i = x;
   n[6].i = y;
   n[7].i = width;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyTexSubImage2D(ctx->Dispatch.Exec, (target, level, xoffset, yoffset,
         }
         void GLAPIENTRY
   save_CopyTexSubImage3D(GLenum target, GLint level,
               {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEX_SUB_IMAGE3D, 9);
   if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = xoffset;
   n[4].i = yoffset;
   n[5].i = zoffset;
   n[6].i = x;
   n[7].i = y;
   n[8].i = width;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyTexSubImage3D(ctx->Dispatch.Exec, (target, level,
               }
         void GLAPIENTRY
   save_CullFace(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CULL_FACE, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_DepthFunc(GLenum func)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DEPTH_FUNC, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_DepthMask(GLboolean mask)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DEPTH_MASK, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_DepthRange(GLclampd nearval, GLclampd farval)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DEPTH_RANGE, 2);
   if (n) {
      n[1].f = (GLfloat) nearval;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Disable(GLenum cap)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DISABLE, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Disablei(GLuint index, GLenum cap)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DISABLE_INDEXED, 2);
   if (n) {
      n[1].ui = index;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_DrawBuffer(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DRAW_BUFFER, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_DrawPixels(GLsizei width, GLsizei height,
         {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_DRAW_PIXELS, 4 + POINTER_DWORDS);
   if (n) {
      n[1].i = width;
   n[2].i = height;
   n[3].e = format;
   n[4].e = type;
   save_pointer(&n[5],
            }
   if (ctx->ExecuteFlag) {
            }
            void GLAPIENTRY
   save_Enable(GLenum cap)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ENABLE, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
            void GLAPIENTRY
   save_Enablei(GLuint index, GLenum cap)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ENABLE_INDEXED, 2);
   if (n) {
      n[1].ui = index;
      }
   if (ctx->ExecuteFlag) {
            }
            void GLAPIENTRY
   save_EvalMesh1(GLenum mode, GLint i1, GLint i2)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_EVALMESH1, 3);
   if (n) {
      n[1].e = mode;
   n[2].i = i1;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_EVALMESH2, 5);
   if (n) {
      n[1].e = mode;
   n[2].i = i1;
   n[3].i = i2;
   n[4].i = j1;
      }
   if (ctx->ExecuteFlag) {
            }
               void GLAPIENTRY
   save_Fogfv(GLenum pname, const GLfloat *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_FOG, 5);
   if (n) {
      n[1].e = pname;
   n[2].f = params[0];
   n[3].f = params[1];
   n[4].f = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Fogf(GLenum pname, GLfloat param)
   {
      GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
      }
         void GLAPIENTRY
   save_Fogiv(GLenum pname, const GLint *params)
   {
      GLfloat p[4];
   switch (pname) {
   case GL_FOG_MODE:
   case GL_FOG_DENSITY:
   case GL_FOG_START:
   case GL_FOG_END:
   case GL_FOG_INDEX:
   case GL_FOG_COORDINATE_SOURCE:
      p[0] = (GLfloat) *params;
   p[1] = 0.0f;
   p[2] = 0.0f;
   p[3] = 0.0f;
      case GL_FOG_COLOR:
      p[0] = INT_TO_FLOAT(params[0]);
   p[1] = INT_TO_FLOAT(params[1]);
   p[2] = INT_TO_FLOAT(params[2]);
   p[3] = INT_TO_FLOAT(params[3]);
      default:
      /* Error will be caught later in gl_Fogfv */
      }
      }
         void GLAPIENTRY
   save_Fogi(GLenum pname, GLint param)
   {
      GLint parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0;
      }
         void GLAPIENTRY
   save_FrontFace(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_FRONT_FACE, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Frustum(GLdouble left, GLdouble right,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_FRUSTUM, 6);
   if (n) {
      n[1].f = (GLfloat) left;
   n[2].f = (GLfloat) right;
   n[3].f = (GLfloat) bottom;
   n[4].f = (GLfloat) top;
   n[5].f = (GLfloat) nearval;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Hint(GLenum target, GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_HINT, 2);
   if (n) {
      n[1].e = target;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_IndexMask(GLuint mask)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_INDEX_MASK, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_InitNames(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_INIT_NAMES, 0);
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Lightfv(GLenum light, GLenum pname, const GLfloat *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LIGHT, 6);
   if (n) {
      GLint i, nParams;
   n[1].e = light;
   n[2].e = pname;
   switch (pname) {
   case GL_AMBIENT:
      nParams = 4;
      case GL_DIFFUSE:
      nParams = 4;
      case GL_SPECULAR:
      nParams = 4;
      case GL_POSITION:
      nParams = 4;
      case GL_SPOT_DIRECTION:
      nParams = 3;
      case GL_SPOT_EXPONENT:
      nParams = 1;
      case GL_SPOT_CUTOFF:
      nParams = 1;
      case GL_CONSTANT_ATTENUATION:
      nParams = 1;
      case GL_LINEAR_ATTENUATION:
      nParams = 1;
      case GL_QUADRATIC_ATTENUATION:
      nParams = 1;
      default:
         }
   for (i = 0; i < nParams; i++) {
            }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Lightf(GLenum light, GLenum pname, GLfloat param)
   {
      GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
      }
         void GLAPIENTRY
   save_Lightiv(GLenum light, GLenum pname, const GLint *params)
   {
      GLfloat fparam[4];
   switch (pname) {
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
      fparam[0] = INT_TO_FLOAT(params[0]);
   fparam[1] = INT_TO_FLOAT(params[1]);
   fparam[2] = INT_TO_FLOAT(params[2]);
   fparam[3] = INT_TO_FLOAT(params[3]);
      case GL_POSITION:
      fparam[0] = (GLfloat) params[0];
   fparam[1] = (GLfloat) params[1];
   fparam[2] = (GLfloat) params[2];
   fparam[3] = (GLfloat) params[3];
      case GL_SPOT_DIRECTION:
      fparam[0] = (GLfloat) params[0];
   fparam[1] = (GLfloat) params[1];
   fparam[2] = (GLfloat) params[2];
      case GL_SPOT_EXPONENT:
   case GL_SPOT_CUTOFF:
   case GL_CONSTANT_ATTENUATION:
   case GL_LINEAR_ATTENUATION:
   case GL_QUADRATIC_ATTENUATION:
      fparam[0] = (GLfloat) params[0];
      default:
      /* error will be caught later in gl_Lightfv */
      }
      }
         void GLAPIENTRY
   save_Lighti(GLenum light, GLenum pname, GLint param)
   {
      GLint parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0;
      }
         void GLAPIENTRY
   save_LightModelfv(GLenum pname, const GLfloat *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LIGHT_MODEL, 5);
   if (n) {
      n[1].e = pname;
   n[2].f = params[0];
   n[3].f = params[1];
   n[4].f = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_LightModelf(GLenum pname, GLfloat param)
   {
      GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
      }
         void GLAPIENTRY
   save_LightModeliv(GLenum pname, const GLint *params)
   {
      GLfloat fparam[4];
   switch (pname) {
   case GL_LIGHT_MODEL_AMBIENT:
      fparam[0] = INT_TO_FLOAT(params[0]);
   fparam[1] = INT_TO_FLOAT(params[1]);
   fparam[2] = INT_TO_FLOAT(params[2]);
   fparam[3] = INT_TO_FLOAT(params[3]);
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
   case GL_LIGHT_MODEL_TWO_SIDE:
   case GL_LIGHT_MODEL_COLOR_CONTROL:
      fparam[0] = (GLfloat) params[0];
   fparam[1] = 0.0F;
   fparam[2] = 0.0F;
   fparam[3] = 0.0F;
      default:
      /* Error will be caught later in gl_LightModelfv */
      }
      }
         void GLAPIENTRY
   save_LightModeli(GLenum pname, GLint param)
   {
      GLint parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0;
      }
         void GLAPIENTRY
   save_LineStipple(GLint factor, GLushort pattern)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LINE_STIPPLE, 2);
   if (n) {
      n[1].i = factor;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_LineWidth(GLfloat width)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LINE_WIDTH, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ListBase(GLuint base)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LIST_BASE, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_LoadIdentity(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_LOAD_IDENTITY, 0);
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_LoadMatrixf(const GLfloat * m)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LOAD_MATRIX, 16);
   if (n) {
      GLuint i;
   for (i = 0; i < 16; i++) {
            }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_LoadMatrixd(const GLdouble * m)
   {
      GLfloat f[16];
   GLint i;
   for (i = 0; i < 16; i++) {
         }
      }
         void GLAPIENTRY
   save_LoadName(GLuint name)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LOAD_NAME, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_LogicOp(GLenum opcode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LOGIC_OP, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Map1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAP1, 5 + POINTER_DWORDS);
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points1d(target, stride, order, points);
   n[1].e = target;
   n[2].f = (GLfloat) u1;
   n[3].f = (GLfloat) u2;
   n[4].i = _mesa_evaluator_components(target);      /* stride */
   n[5].i = order;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Map1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAP1, 5 + POINTER_DWORDS);
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points1f(target, stride, order, points);
   n[1].e = target;
   n[2].f = u1;
   n[3].f = u2;
   n[4].i = _mesa_evaluator_components(target);      /* stride */
   n[5].i = order;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Map2d(GLenum target,
            GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
      {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAP2, 9 + POINTER_DWORDS);
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points2d(target, ustride, uorder,
         n[1].e = target;
   n[2].f = (GLfloat) u1;
   n[3].f = (GLfloat) u2;
   n[4].f = (GLfloat) v1;
   n[5].f = (GLfloat) v2;
   /* XXX verify these strides are correct */
   n[6].i = _mesa_evaluator_components(target) * vorder;     /*ustride */
   n[7].i = _mesa_evaluator_components(target);      /*vstride */
   n[8].i = uorder;
   n[9].i = vorder;
      }
   if (ctx->ExecuteFlag) {
      CALL_Map2d(ctx->Dispatch.Exec, (target,
               }
         void GLAPIENTRY
   save_Map2f(GLenum target,
            GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
      {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAP2, 9 + POINTER_DWORDS);
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points2f(target, ustride, uorder,
         n[1].e = target;
   n[2].f = u1;
   n[3].f = u2;
   n[4].f = v1;
   n[5].f = v2;
   /* XXX verify these strides are correct */
   n[6].i = _mesa_evaluator_components(target) * vorder;     /*ustride */
   n[7].i = _mesa_evaluator_components(target);      /*vstride */
   n[8].i = uorder;
   n[9].i = vorder;
      }
   if (ctx->ExecuteFlag) {
      CALL_Map2f(ctx->Dispatch.Exec, (target, u1, u2, ustride, uorder,
         }
         void GLAPIENTRY
   save_MapGrid1f(GLint un, GLfloat u1, GLfloat u2)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAPGRID1, 3);
   if (n) {
      n[1].i = un;
   n[2].f = u1;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_MapGrid1d(GLint un, GLdouble u1, GLdouble u2)
   {
         }
         void GLAPIENTRY
   save_MapGrid2f(GLint un, GLfloat u1, GLfloat u2,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAPGRID2, 6);
   if (n) {
      n[1].i = un;
   n[2].f = u1;
   n[3].f = u2;
   n[4].i = vn;
   n[5].f = v1;
      }
   if (ctx->ExecuteFlag) {
            }
            void GLAPIENTRY
   save_MapGrid2d(GLint un, GLdouble u1, GLdouble u2,
         {
      save_MapGrid2f(un, (GLfloat) u1, (GLfloat) u2,
      }
         void GLAPIENTRY
   save_MatrixMode(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_MODE, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_MultMatrixf(const GLfloat * m)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MULT_MATRIX, 16);
   if (n) {
      GLuint i;
   for (i = 0; i < 16; i++) {
            }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_MultMatrixd(const GLdouble * m)
   {
      GLfloat f[16];
   GLint i;
   for (i = 0; i < 16; i++) {
         }
      }
         void GLAPIENTRY
   save_NewList(GLuint name, GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   /* It's an error to call this function while building a display list */
   _mesa_error(ctx, GL_INVALID_OPERATION, "glNewList");
   (void) name;
      }
            void GLAPIENTRY
   save_Ortho(GLdouble left, GLdouble right,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ORTHO, 6);
   if (n) {
      n[1].f = (GLfloat) left;
   n[2].f = (GLfloat) right;
   n[3].f = (GLfloat) bottom;
   n[4].f = (GLfloat) top;
   n[5].f = (GLfloat) nearval;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PatchParameteri(GLenum pname, const GLint value)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PATCH_PARAMETER_I, 2);
   if (n) {
      n[1].e = pname;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PatchParameterfv(GLenum pname, const GLfloat *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
            if (pname == GL_PATCH_DEFAULT_OUTER_LEVEL) {
         } else {
      assert(pname == GL_PATCH_DEFAULT_INNER_LEVEL);
      }
   if (n) {
      n[1].e = pname;
   if (pname == GL_PATCH_DEFAULT_OUTER_LEVEL) {
      n[2].f = params[0];
   n[3].f = params[1];
   n[4].f = params[2];
      } else {
      n[2].f = params[0];
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PixelMapfv(GLenum map, GLint mapsize, const GLfloat *values)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PIXEL_MAP, 2 + POINTER_DWORDS);
   if (n) {
      n[1].e = map;
   n[2].i = mapsize;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PixelMapuiv(GLenum map, GLint mapsize, const GLuint *values)
   {
      GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   if (map == GL_PIXEL_MAP_I_TO_I || map == GL_PIXEL_MAP_S_TO_S) {
      for (i = 0; i < mapsize; i++) {
            }
   else {
      for (i = 0; i < mapsize; i++) {
            }
      }
         void GLAPIENTRY
   save_PixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
   {
      GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   if (map == GL_PIXEL_MAP_I_TO_I || map == GL_PIXEL_MAP_S_TO_S) {
      for (i = 0; i < mapsize; i++) {
            }
   else {
      for (i = 0; i < mapsize; i++) {
            }
      }
         void GLAPIENTRY
   save_PixelTransferf(GLenum pname, GLfloat param)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PIXEL_TRANSFER, 2);
   if (n) {
      n[1].e = pname;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PixelTransferi(GLenum pname, GLint param)
   {
         }
         void GLAPIENTRY
   save_PixelZoom(GLfloat xfactor, GLfloat yfactor)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PIXEL_ZOOM, 2);
   if (n) {
      n[1].f = xfactor;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PointParameterfv(GLenum pname, const GLfloat *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_POINT_PARAMETERS, 4);
   if (n) {
      n[1].e = pname;
   n[2].f = params[0];
   n[3].f = params[1];
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PointParameterf(GLenum pname, GLfloat param)
   {
      GLfloat parray[3];
   parray[0] = param;
   parray[1] = parray[2] = 0.0F;
      }
      void GLAPIENTRY
   save_PointParameteri(GLenum pname, GLint param)
   {
      GLfloat parray[3];
   parray[0] = (GLfloat) param;
   parray[1] = parray[2] = 0.0F;
      }
      void GLAPIENTRY
   save_PointParameteriv(GLenum pname, const GLint * param)
   {
      GLfloat parray[3];
   parray[0] = (GLfloat) param[0];
   parray[1] = parray[2] = 0.0F;
      }
         void GLAPIENTRY
   save_PointSize(GLfloat size)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_POINT_SIZE, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PolygonMode(GLenum face, GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_POLYGON_MODE, 2);
   if (n) {
      n[1].e = face;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PolygonStipple(const GLubyte * pattern)
   {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_POLYGON_STIPPLE, POINTER_DWORDS);
   if (n) {
      save_pointer(&n[1],
            }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PolygonOffset(GLfloat factor, GLfloat units)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_POLYGON_OFFSET, 2);
   if (n) {
      n[1].f = factor;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PolygonOffsetClampEXT(GLfloat factor, GLfloat units, GLfloat clamp)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_POLYGON_OFFSET_CLAMP, 3);
   if (n) {
      n[1].f = factor;
   n[2].f = units;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_PopAttrib(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_POP_ATTRIB, 0);
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PopMatrix(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_POP_MATRIX, 0);
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PopName(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_POP_NAME, 0);
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PrioritizeTextures(GLsizei num, const GLuint * textures,
         {
      GET_CURRENT_CONTEXT(ctx);
   GLint i;
            for (i = 0; i < num; i++) {
      Node *n;
   n = alloc_instruction(ctx, OPCODE_PRIORITIZE_TEXTURE, 2);
   if (n) {
      n[1].ui = textures[i];
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PushAttrib(GLbitfield mask)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PUSH_ATTRIB, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PushMatrix(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_PUSH_MATRIX, 0);
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_PushName(GLuint name)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PUSH_NAME, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_RASTER_POS, 4);
   if (n) {
      n[1].f = x;
   n[2].f = y;
   n[3].f = z;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_RasterPos2d(GLdouble x, GLdouble y)
   {
         }
      void GLAPIENTRY
   save_RasterPos2f(GLfloat x, GLfloat y)
   {
         }
      void GLAPIENTRY
   save_RasterPos2i(GLint x, GLint y)
   {
         }
      void GLAPIENTRY
   save_RasterPos2s(GLshort x, GLshort y)
   {
         }
      void GLAPIENTRY
   save_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
   {
         }
      void GLAPIENTRY
   save_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
   {
         }
      void GLAPIENTRY
   save_RasterPos3i(GLint x, GLint y, GLint z)
   {
         }
      void GLAPIENTRY
   save_RasterPos3s(GLshort x, GLshort y, GLshort z)
   {
         }
      void GLAPIENTRY
   save_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
   {
         }
      void GLAPIENTRY
   save_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
   {
         }
      void GLAPIENTRY
   save_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
   {
         }
      void GLAPIENTRY
   save_RasterPos2dv(const GLdouble * v)
   {
         }
      void GLAPIENTRY
   save_RasterPos2fv(const GLfloat * v)
   {
         }
      void GLAPIENTRY
   save_RasterPos2iv(const GLint * v)
   {
         }
      void GLAPIENTRY
   save_RasterPos2sv(const GLshort * v)
   {
         }
      void GLAPIENTRY
   save_RasterPos3dv(const GLdouble * v)
   {
         }
      void GLAPIENTRY
   save_RasterPos3fv(const GLfloat * v)
   {
         }
      void GLAPIENTRY
   save_RasterPos3iv(const GLint * v)
   {
         }
      void GLAPIENTRY
   save_RasterPos3sv(const GLshort * v)
   {
         }
      void GLAPIENTRY
   save_RasterPos4dv(const GLdouble * v)
   {
      save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1],
      }
      void GLAPIENTRY
   save_RasterPos4fv(const GLfloat * v)
   {
         }
      void GLAPIENTRY
   save_RasterPos4iv(const GLint * v)
   {
      save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1],
      }
      void GLAPIENTRY
   save_RasterPos4sv(const GLshort * v)
   {
         }
         void GLAPIENTRY
   save_PassThrough(GLfloat token)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PASSTHROUGH, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ReadBuffer(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_READ_BUFFER, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ROTATE, 4);
   if (n) {
      n[1].f = angle;
   n[2].f = x;
   n[3].f = y;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Rotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
   {
         }
         void GLAPIENTRY
   save_Scalef(GLfloat x, GLfloat y, GLfloat z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SCALE, 3);
   if (n) {
      n[1].f = x;
   n[2].f = y;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Scaled(GLdouble x, GLdouble y, GLdouble z)
   {
         }
         void GLAPIENTRY
   save_Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SCISSOR, 4);
   if (n) {
      n[1].i = x;
   n[2].i = y;
   n[3].i = width;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ShadeModel(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
            if (ctx->ExecuteFlag) {
                  /* Don't compile this call if it's a no-op.
   * By avoiding this state change we have a better chance of
   * coalescing subsequent drawing commands into one batch.
   */
   if (ctx->ListState.Current.ShadeModel == mode)
                              n = alloc_instruction(ctx, OPCODE_SHADE_MODEL, 1);
   if (n) {
            }
         void GLAPIENTRY
   save_StencilFunc(GLenum func, GLint ref, GLuint mask)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_FUNC, 3);
   if (n) {
      n[1].e = func;
   n[2].i = ref;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_StencilMask(GLuint mask)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_MASK, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_OP, 3);
   if (n) {
      n[1].e = fail;
   n[2].e = zfail;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_FUNC_SEPARATE, 4);
   if (n) {
      n[1].e = face;
   n[2].e = func;
   n[3].i = ref;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_StencilFuncSeparateATI(GLenum frontfunc, GLenum backfunc, GLint ref,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   /* GL_FRONT */
   n = alloc_instruction(ctx, OPCODE_STENCIL_FUNC_SEPARATE, 4);
   if (n) {
      n[1].e = GL_FRONT;
   n[2].e = frontfunc;
   n[3].i = ref;
      }
   /* GL_BACK */
   n = alloc_instruction(ctx, OPCODE_STENCIL_FUNC_SEPARATE, 4);
   if (n) {
      n[1].e = GL_BACK;
   n[2].e = backfunc;
   n[3].i = ref;
      }
   if (ctx->ExecuteFlag) {
      CALL_StencilFuncSeparate(ctx->Dispatch.Exec, (GL_FRONT, frontfunc, ref, mask));
         }
         void GLAPIENTRY
   save_StencilMaskSeparate(GLenum face, GLuint mask)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_MASK_SEPARATE, 2);
   if (n) {
      n[1].e = face;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_StencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_OP_SEPARATE, 4);
   if (n) {
      n[1].e = face;
   n[2].e = fail;
   n[3].e = zfail;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_TexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXENV, 6);
   if (n) {
      n[1].e = target;
   n[2].e = pname;
   if (pname == GL_TEXTURE_ENV_COLOR) {
      n[3].f = params[0];
   n[4].f = params[1];
   n[5].f = params[2];
      }
   else {
      n[3].f = params[0];
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_TexEnvf(GLenum target, GLenum pname, GLfloat param)
   {
      GLfloat parray[4];
   parray[0] = (GLfloat) param;
   parray[1] = parray[2] = parray[3] = 0.0F;
      }
         void GLAPIENTRY
   save_TexEnvi(GLenum target, GLenum pname, GLint param)
   {
      GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0F;
      }
         void GLAPIENTRY
   save_TexEnviv(GLenum target, GLenum pname, const GLint * param)
   {
      GLfloat p[4];
   if (pname == GL_TEXTURE_ENV_COLOR) {
      p[0] = INT_TO_FLOAT(param[0]);
   p[1] = INT_TO_FLOAT(param[1]);
   p[2] = INT_TO_FLOAT(param[2]);
      }
   else {
      p[0] = (GLfloat) param[0];
      }
      }
         void GLAPIENTRY
   save_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXGEN, 6);
   if (n) {
      n[1].e = coord;
   n[2].e = pname;
   n[3].f = params[0];
   n[4].f = params[1];
   n[5].f = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_TexGeniv(GLenum coord, GLenum pname, const GLint *params)
   {
      GLfloat p[4];
   p[0] = (GLfloat) params[0];
   p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
   p[3] = (GLfloat) params[3];
      }
         void GLAPIENTRY
   save_TexGend(GLenum coord, GLenum pname, GLdouble param)
   {
      GLfloat parray[4];
   parray[0] = (GLfloat) param;
   parray[1] = parray[2] = parray[3] = 0.0F;
      }
         void GLAPIENTRY
   save_TexGendv(GLenum coord, GLenum pname, const GLdouble *params)
   {
      GLfloat p[4];
   p[0] = (GLfloat) params[0];
   p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
   p[3] = (GLfloat) params[3];
      }
         void GLAPIENTRY
   save_TexGenf(GLenum coord, GLenum pname, GLfloat param)
   {
      GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
      }
         void GLAPIENTRY
   save_TexGeni(GLenum coord, GLenum pname, GLint param)
   {
      GLint parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0;
      }
         void GLAPIENTRY
   save_TexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXPARAMETER, 6);
   if (n) {
      n[1].e = target;
   n[2].e = pname;
   n[3].f = params[0];
   n[4].f = params[1];
   n[5].f = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_TexParameterf(GLenum target, GLenum pname, GLfloat param)
   {
      GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
      }
         void GLAPIENTRY
   save_TexParameteri(GLenum target, GLenum pname, GLint param)
   {
      GLfloat fparam[4];
   fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0F;
      }
         void GLAPIENTRY
   save_TexParameteriv(GLenum target, GLenum pname, const GLint *params)
   {
      GLfloat fparam[4];
   fparam[0] = (GLfloat) params[0];
   fparam[1] = fparam[2] = fparam[3] = 0.0F;
      }
         void GLAPIENTRY
   save_TexImage1D(GLenum target,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D) {
      /* don't compile, execute immediately */
   CALL_TexImage1D(ctx->Dispatch.Exec, (target, level, components, width,
      }
   else {
      Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEX_IMAGE1D, 7 + POINTER_DWORDS);
   if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = components;
   n[4].i = (GLint) width;
   n[5].i = border;
   n[6].e = format;
   n[7].e = type;
   save_pointer(&n[8],
            }
   if (ctx->ExecuteFlag) {
      CALL_TexImage1D(ctx->Dispatch.Exec, (target, level, components, width,
            }
         void GLAPIENTRY
   save_TexImage2D(GLenum target,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_2D) {
      /* don't compile, execute immediately */
   CALL_TexImage2D(ctx->Dispatch.Exec, (target, level, components, width,
      }
   else {
      Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEX_IMAGE2D, 8 + POINTER_DWORDS);
   if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = components;
   n[4].i = (GLint) width;
   n[5].i = (GLint) height;
   n[6].i = border;
   n[7].e = format;
   n[8].e = type;
   save_pointer(&n[9],
            }
   if (ctx->ExecuteFlag) {
      CALL_TexImage2D(ctx->Dispatch.Exec, (target, level, components, width,
            }
         void GLAPIENTRY
   save_TexImage3D(GLenum target,
                  GLint level, GLint internalFormat,
      {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_3D) {
      /* don't compile, execute immediately */
   CALL_TexImage3D(ctx->Dispatch.Exec, (target, level, internalFormat, width,
            }
   else {
      Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEX_IMAGE3D, 9 + POINTER_DWORDS);
   if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = (GLint) internalFormat;
   n[4].i = (GLint) width;
   n[5].i = (GLint) height;
   n[6].i = (GLint) depth;
   n[7].i = border;
   n[8].e = format;
   n[9].e = type;
   save_pointer(&n[10],
            }
   if (ctx->ExecuteFlag) {
      CALL_TexImage3D(ctx->Dispatch.Exec, (target, level, internalFormat, width,
                  }
         void GLAPIENTRY
   save_TexSubImage1D(GLenum target, GLint level, GLint xoffset,
               {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_TEX_SUB_IMAGE1D, 6 + POINTER_DWORDS);
   if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = xoffset;
   n[4].i = (GLint) width;
   n[5].e = format;
   n[6].e = type;
   save_pointer(&n[7],
            }
   if (ctx->ExecuteFlag) {
      CALL_TexSubImage1D(ctx->Dispatch.Exec, (target, level, xoffset, width,
         }
         void GLAPIENTRY
   save_TexSubImage2D(GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_TEX_SUB_IMAGE2D, 8 + POINTER_DWORDS);
   if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = xoffset;
   n[4].i = yoffset;
   n[5].i = (GLint) width;
   n[6].i = (GLint) height;
   n[7].e = format;
   n[8].e = type;
   save_pointer(&n[9],
            }
   if (ctx->ExecuteFlag) {
      CALL_TexSubImage2D(ctx->Dispatch.Exec, (target, level, xoffset, yoffset,
         }
         void GLAPIENTRY
   save_TexSubImage3D(GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_TEX_SUB_IMAGE3D, 10 + POINTER_DWORDS);
   if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = xoffset;
   n[4].i = yoffset;
   n[5].i = zoffset;
   n[6].i = (GLint) width;
   n[7].i = (GLint) height;
   n[8].i = (GLint) depth;
   n[9].e = format;
   n[10].e = type;
   save_pointer(&n[11],
            }
   if (ctx->ExecuteFlag) {
      CALL_TexSubImage3D(ctx->Dispatch.Exec, (target, level,
                     }
         void GLAPIENTRY
   save_Translatef(GLfloat x, GLfloat y, GLfloat z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TRANSLATE, 3);
   if (n) {
      n[1].f = x;
   n[2].f = y;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Translated(GLdouble x, GLdouble y, GLdouble z)
   {
         }
            void GLAPIENTRY
   save_Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_VIEWPORT, 4);
   if (n) {
      n[1].i = x;
   n[2].i = y;
   n[3].i = (GLint) width;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat width,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_VIEWPORT_INDEXED_F, 5);
   if (n) {
      n[1].ui = index;
   n[2].f = x;
   n[3].f = y;
   n[4].f = width;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ViewportIndexedfv(GLuint index, const GLfloat *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_VIEWPORT_INDEXED_FV, 5);
   if (n) {
      n[1].ui = index;
   n[2].f = v[0];
   n[3].f = v[1];
   n[4].f = v[2];
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ViewportArrayv(GLuint first, GLsizei count, const GLfloat *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_VIEWPORT_ARRAY_V, 2 + POINTER_DWORDS);
   if (n) {
      n[1].ui = first;
   n[2].si = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ScissorIndexed(GLuint index, GLint left, GLint bottom, GLsizei width,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SCISSOR_INDEXED, 5);
   if (n) {
      n[1].ui = index;
   n[2].i = left;
   n[3].i = bottom;
   n[4].si = width;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ScissorIndexedv(GLuint index, const GLint *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SCISSOR_INDEXED_V, 5);
   if (n) {
      n[1].ui = index;
   n[2].i = v[0];
   n[3].i = v[1];
   n[4].si = v[2];
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ScissorArrayv(GLuint first, GLsizei count, const GLint *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SCISSOR_ARRAY_V, 2 + POINTER_DWORDS);
   if (n) {
      n[1].ui = first;
   n[2].si = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_DepthRangeIndexed(GLuint index, GLclampd n, GLclampd f)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *node;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   node = alloc_instruction(ctx, OPCODE_DEPTH_INDEXED, 3);
   if (node) {
      node[1].ui = index;
   /* Mesa stores these as floats internally so we deliberately convert
   * them to a float here.
   */
   node[2].f = n;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_DepthRangeArrayv(GLuint first, GLsizei count, const GLclampd *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DEPTH_ARRAY_V, 2 + POINTER_DWORDS);
   if (n) {
      n[1].ui = first;
   n[2].si = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_WindowPos4fMESA(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_WINDOW_POS, 4);
   if (n) {
      n[1].f = x;
   n[2].f = y;
   n[3].f = z;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_WindowPos2d(GLdouble x, GLdouble y)
   {
         }
      void GLAPIENTRY
   save_WindowPos2f(GLfloat x, GLfloat y)
   {
         }
      void GLAPIENTRY
   save_WindowPos2i(GLint x, GLint y)
   {
         }
      void GLAPIENTRY
   save_WindowPos2s(GLshort x, GLshort y)
   {
         }
      void GLAPIENTRY
   save_WindowPos3d(GLdouble x, GLdouble y, GLdouble z)
   {
         }
      void GLAPIENTRY
   save_WindowPos3f(GLfloat x, GLfloat y, GLfloat z)
   {
         }
      void GLAPIENTRY
   save_WindowPos3i(GLint x, GLint y, GLint z)
   {
         }
      void GLAPIENTRY
   save_WindowPos3s(GLshort x, GLshort y, GLshort z)
   {
         }
      void GLAPIENTRY
   save_WindowPos4dMESA(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
   {
         }
      void GLAPIENTRY
   save_WindowPos4iMESA(GLint x, GLint y, GLint z, GLint w)
   {
         }
      void GLAPIENTRY
   save_WindowPos4sMESA(GLshort x, GLshort y, GLshort z, GLshort w)
   {
         }
      void GLAPIENTRY
   save_WindowPos2dv(const GLdouble * v)
   {
         }
      void GLAPIENTRY
   save_WindowPos2fv(const GLfloat * v)
   {
         }
      void GLAPIENTRY
   save_WindowPos2iv(const GLint * v)
   {
         }
      void GLAPIENTRY
   save_WindowPos2sv(const GLshort * v)
   {
         }
      void GLAPIENTRY
   save_WindowPos3dv(const GLdouble * v)
   {
         }
      void GLAPIENTRY
   save_WindowPos3fv(const GLfloat * v)
   {
         }
      void GLAPIENTRY
   save_WindowPos3iv(const GLint * v)
   {
         }
      void GLAPIENTRY
   save_WindowPos3sv(const GLshort * v)
   {
         }
      void GLAPIENTRY
   save_WindowPos4dvMESA(const GLdouble * v)
   {
      save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1],
      }
      void GLAPIENTRY
   save_WindowPos4fvMESA(const GLfloat * v)
   {
         }
      void GLAPIENTRY
   save_WindowPos4ivMESA(const GLint * v)
   {
      save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1],
      }
      void GLAPIENTRY
   save_WindowPos4svMESA(const GLshort * v)
   {
         }
            /* GL_ARB_multitexture */
   void GLAPIENTRY
   save_ActiveTexture(GLenum target)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ACTIVE_TEXTURE, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         /* GL_ARB_transpose_matrix */
      void GLAPIENTRY
   save_LoadTransposeMatrixd(const GLdouble *m)
   {
      GLfloat tm[16];
   _math_transposefd(tm, m);
      }
         void GLAPIENTRY
   save_LoadTransposeMatrixf(const GLfloat *m)
   {
      GLfloat tm[16];
   _math_transposef(tm, m);
      }
         void GLAPIENTRY
   save_MultTransposeMatrixd(const GLdouble *m)
   {
      GLfloat tm[16];
   _math_transposefd(tm, m);
      }
         void GLAPIENTRY
   save_MultTransposeMatrixf(const GLfloat *m)
   {
      GLfloat tm[16];
   _math_transposef(tm, m);
      }
      static GLvoid *copy_data(const GLvoid *data, GLsizei size, const char *func)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (!data)
            image = malloc(size);
   if (!image) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", func);
      }
               }
         /* GL_ARB_texture_compression */
   void GLAPIENTRY
   save_CompressedTexImage1D(GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D) {
      /* don't compile, execute immediately */
   CALL_CompressedTexImage1D(ctx->Dispatch.Exec, (target, level, internalFormat,
            }
   else {
      Node *n;
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_IMAGE_1D,
         if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].e = internalFormat;
   n[4].i = (GLint) width;
   n[5].i = border;
   n[6].i = imageSize;
   save_pointer(&n[7],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTexImage1D(ctx->Dispatch.Exec,
                  }
         void GLAPIENTRY
   save_CompressedTexImage2D(GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_2D) {
      /* don't compile, execute immediately */
   CALL_CompressedTexImage2D(ctx->Dispatch.Exec, (target, level, internalFormat,
            }
   else {
      Node *n;
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_IMAGE_2D,
         if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].e = internalFormat;
   n[4].i = (GLint) width;
   n[5].i = (GLint) height;
   n[6].i = border;
   n[7].i = imageSize;
   save_pointer(&n[8],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTexImage2D(ctx->Dispatch.Exec,
                  }
         void GLAPIENTRY
   save_CompressedTexImage3D(GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_3D) {
      /* don't compile, execute immediately */
   CALL_CompressedTexImage3D(ctx->Dispatch.Exec, (target, level, internalFormat,
            }
   else {
      Node *n;
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_IMAGE_3D,
         if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].e = internalFormat;
   n[4].i = (GLint) width;
   n[5].i = (GLint) height;
   n[6].i = (GLint) depth;
   n[7].i = border;
   n[8].i = imageSize;
   save_pointer(&n[9],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTexImage3D(ctx->Dispatch.Exec,
                        }
         void GLAPIENTRY
   save_CompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset,
               {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D,
         if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = xoffset;
   n[4].i = (GLint) width;
   n[5].e = format;
   n[6].i = imageSize;
   save_pointer(&n[7],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTexSubImage1D(ctx->Dispatch.Exec, (target, level, xoffset,
               }
         void GLAPIENTRY
   save_CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                     {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D,
         if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = xoffset;
   n[4].i = yoffset;
   n[5].i = (GLint) width;
   n[6].i = (GLint) height;
   n[7].e = format;
   n[8].i = imageSize;
   save_pointer(&n[9],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTexSubImage2D(ctx->Dispatch.Exec,
               }
         void GLAPIENTRY
   save_CompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset,
                     {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D,
         if (n) {
      n[1].e = target;
   n[2].i = level;
   n[3].i = xoffset;
   n[4].i = yoffset;
   n[5].i = zoffset;
   n[6].i = (GLint) width;
   n[7].i = (GLint) height;
   n[8].i = (GLint) depth;
   n[9].e = format;
   n[10].i = imageSize;
   save_pointer(&n[11],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTexSubImage3D(ctx->Dispatch.Exec,
                     }
         /* GL_ARB_multisample */
   void GLAPIENTRY
   save_SampleCoverage(GLclampf value, GLboolean invert)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SAMPLE_COVERAGE, 2);
   if (n) {
      n[1].f = value;
      }
   if (ctx->ExecuteFlag) {
            }
         /*
   * GL_ARB_vertex_program
   */
   void GLAPIENTRY
   save_BindProgramARB(GLenum target, GLuint id)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BIND_PROGRAM_ARB, 2);
   if (n) {
      n[1].e = target;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramEnvParameter4fARB(GLenum target, GLuint index,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_ENV_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
   n[2].ui = index;
   n[3].f = x;
   n[4].f = y;
   n[5].f = z;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ProgramEnvParameter4fvARB(GLenum target, GLuint index,
         {
      save_ProgramEnvParameter4fARB(target, index, params[0], params[1],
      }
         void GLAPIENTRY
   save_ProgramEnvParameters4fvEXT(GLenum target, GLuint index, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
            if (count > 0) {
      GLint i;
            for (i = 0 ; i < count ; i++) {
      n = alloc_instruction(ctx, OPCODE_PROGRAM_ENV_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
   n[2].ui = index;
   n[3].f = p[0];
   n[4].f = p[1];
   n[5].f = p[2];
   n[6].f = p[3];
                     if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ProgramEnvParameter4dARB(GLenum target, GLuint index,
         {
      save_ProgramEnvParameter4fARB(target, index,
            }
         void GLAPIENTRY
   save_ProgramEnvParameter4dvARB(GLenum target, GLuint index,
         {
      save_ProgramEnvParameter4fARB(target, index,
                  }
         void GLAPIENTRY
   save_ProgramLocalParameter4fARB(GLenum target, GLuint index,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
   n[2].ui = index;
   n[3].f = x;
   n[4].f = y;
   n[5].f = z;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ProgramLocalParameter4fvARB(GLenum target, GLuint index,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
   n[2].ui = index;
   n[3].f = params[0];
   n[4].f = params[1];
   n[5].f = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ProgramLocalParameters4fvEXT(GLenum target, GLuint index, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
            if (count > 0) {
      GLint i;
            for (i = 0 ; i < count ; i++) {
      n = alloc_instruction(ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
   n[2].ui = index;
   n[3].f = p[0];
   n[4].f = p[1];
   n[5].f = p[2];
   n[6].f = p[3];
                     if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ProgramLocalParameter4dARB(GLenum target, GLuint index,
               {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
   n[2].ui = index;
   n[3].f = (GLfloat) x;
   n[4].f = (GLfloat) y;
   n[5].f = (GLfloat) z;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_ProgramLocalParameter4dvARB(GLenum target, GLuint index,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
   n[2].ui = index;
   n[3].f = (GLfloat) params[0];
   n[4].f = (GLfloat) params[1];
   n[5].f = (GLfloat) params[2];
      }
   if (ctx->ExecuteFlag) {
            }
         /* GL_EXT_stencil_two_side */
   void GLAPIENTRY
   save_ActiveStencilFaceEXT(GLenum face)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ACTIVE_STENCIL_FACE_EXT, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         /* GL_EXT_depth_bounds_test */
   void GLAPIENTRY
   save_DepthBoundsEXT(GLclampd zmin, GLclampd zmax)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DEPTH_BOUNDS_EXT, 2);
   if (n) {
      n[1].f = (GLfloat) zmin;
      }
   if (ctx->ExecuteFlag) {
            }
            void GLAPIENTRY
   save_ProgramStringARB(GLenum target, GLenum format, GLsizei len,
         {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_PROGRAM_STRING_ARB, 3 + POINTER_DWORDS);
   if (n) {
      GLubyte *programCopy = malloc(len);
   if (!programCopy) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramStringARB");
      }
   memcpy(programCopy, string, len);
   n[1].e = target;
   n[2].e = format;
   n[3].i = len;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_BeginQuery(GLenum target, GLuint id)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BEGIN_QUERY_ARB, 2);
   if (n) {
      n[1].e = target;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_EndQuery(GLenum target)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_END_QUERY_ARB, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_QueryCounter(GLuint id, GLenum target)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_QUERY_COUNTER, 2);
   if (n) {
      n[1].ui = id;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_BeginQueryIndexed(GLenum target, GLuint index, GLuint id)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BEGIN_QUERY_INDEXED, 3);
   if (n) {
      n[1].e = target;
   n[2].ui = index;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_EndQueryIndexed(GLenum target, GLuint index)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_END_QUERY_INDEXED, 2);
   if (n) {
      n[1].e = target;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_DrawBuffers(GLsizei count, const GLenum * buffers)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DRAW_BUFFERS_ARB, 1 + MAX_DRAW_BUFFERS);
   if (n) {
      GLint i;
   n[1].i = count;
   if (count > MAX_DRAW_BUFFERS)
         for (i = 0; i < count; i++) {
            }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_BindFragmentShaderATI(GLuint id)
   {
      GET_CURRENT_CONTEXT(ctx);
            n = alloc_instruction(ctx, OPCODE_BIND_FRAGMENT_SHADER_ATI, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_SetFragmentShaderConstantATI(GLuint dst, const GLfloat *value)
   {
      GET_CURRENT_CONTEXT(ctx);
            n = alloc_instruction(ctx, OPCODE_SET_FRAGMENT_SHADER_CONSTANTS_ATI, 5);
   if (n) {
      n[1].ui = dst;
   n[2].f = value[0];
   n[3].f = value[1];
   n[4].f = value[2];
      }
   if (ctx->ExecuteFlag) {
            }
      static void GLAPIENTRY
   save_EvalCoord1f(GLfloat x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_EVAL_C1, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
      static void GLAPIENTRY
   save_EvalCoord1fv(const GLfloat * v)
   {
         }
      static void GLAPIENTRY
   save_EvalCoord2f(GLfloat x, GLfloat y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_EVAL_C2, 2);
   if (n) {
      n[1].f = x;
      }
   if (ctx->ExecuteFlag) {
            }
      static void GLAPIENTRY
   save_EvalCoord2fv(const GLfloat * v)
   {
         }
         static void GLAPIENTRY
   save_EvalPoint1(GLint x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_EVAL_P1, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
      static void GLAPIENTRY
   save_EvalPoint2(GLint x, GLint y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_EVAL_P2, 2);
   if (n) {
      n[1].i = x;
      }
   if (ctx->ExecuteFlag) {
            }
         /**
   * Compare 'count' elements of vectors 'a' and 'b'.
   * \return GL_TRUE if equal, GL_FALSE if different.
   */
   static inline GLboolean
   compare_vec(const GLfloat *a, const GLfloat *b, GLuint count)
   {
         }
         /**
   * This glMaterial function is used for glMaterial calls that are outside
   * a glBegin/End pair.  For glMaterial inside glBegin/End, see the VBO code.
   */
   static void GLAPIENTRY
   save_Materialfv(GLenum face, GLenum pname, const GLfloat * param)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   int args, i;
            switch (face) {
   case GL_BACK:
   case GL_FRONT:
   case GL_FRONT_AND_BACK:
         default:
      _mesa_compile_error(ctx, GL_INVALID_ENUM, "glMaterial(face)");
               switch (pname) {
   case GL_EMISSION:
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
   case GL_AMBIENT_AND_DIFFUSE:
      args = 4;
      case GL_SHININESS:
      args = 1;
      case GL_COLOR_INDEXES:
      args = 3;
      default:
      _mesa_compile_error(ctx, GL_INVALID_ENUM, "glMaterial(pname)");
               if (ctx->ExecuteFlag) {
                           /* Try to eliminate redundant statechanges.  Because it is legal to
   * call glMaterial even inside begin/end calls, don't need to worry
   * about ctx->Driver.CurrentSavePrimitive here.
   */
   for (i = 0; i < MAT_ATTRIB_MAX; i++) {
      if (bitmask & (1 << i)) {
      if (ctx->ListState.ActiveMaterialSize[i] == args &&
      compare_vec(ctx->ListState.CurrentMaterial[i], param, args)) {
   /* no change in material value */
      }
   else {
      ctx->ListState.ActiveMaterialSize[i] = args;
                     /* If this call has no effect, return early */
   if (bitmask == 0)
                     n = alloc_instruction(ctx, OPCODE_MATERIAL, 6);
   if (n) {
      n[1].e = face;
   n[2].e = pname;
   for (i = 0; i < args; i++)
         }
      static void GLAPIENTRY
   save_Begin(GLenum mode)
   {
               if (!_mesa_is_valid_prim_mode(ctx, mode)) {
      /* compile this error into the display list */
      }
   else if (_mesa_inside_dlist_begin_end(ctx)) {
      /* compile this error into the display list */
      }
   else {
                     }
      static void GLAPIENTRY
   save_End(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   SAVE_FLUSH_VERTICES(ctx);
   (void) alloc_instruction(ctx, OPCODE_END, 0);
   ctx->Driver.CurrentSavePrimitive = PRIM_OUTSIDE_BEGIN_END;
   if (ctx->ExecuteFlag) {
            }
      static void GLAPIENTRY
   save_PrimitiveRestartNV(void)
   {
      /* Note: this is used when outside a glBegin/End pair in a display list */
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_PRIMITIVE_RESTART_NV, 0);
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_BlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
               {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLIT_FRAMEBUFFER, 10);
   if (n) {
      n[1].i = srcX0;
   n[2].i = srcY0;
   n[3].i = srcX1;
   n[4].i = srcY1;
   n[5].i = dstX0;
   n[6].i = dstY0;
   n[7].i = dstX1;
   n[8].i = dstY1;
   n[9].i = mask;
      }
   if (ctx->ExecuteFlag) {
      CALL_BlitFramebuffer(ctx->Dispatch.Exec, (srcX0, srcY0, srcX1, srcY1,
               }
         /** GL_EXT_provoking_vertex */
   void GLAPIENTRY
   save_ProvokingVertex(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROVOKING_VERTEX, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
      /*CALL_ProvokingVertex(ctx->Dispatch.Exec, (mode));*/
         }
         /** GL_EXT_transform_feedback */
   void GLAPIENTRY
   save_BeginTransformFeedback(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BEGIN_TRANSFORM_FEEDBACK, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         /** GL_EXT_transform_feedback */
   void GLAPIENTRY
   save_EndTransformFeedback(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_END_TRANSFORM_FEEDBACK, 0);
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_BindTransformFeedback(GLenum target, GLuint name)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BIND_TRANSFORM_FEEDBACK, 2);
   if (n) {
      n[1].e = target;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_PauseTransformFeedback(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_PAUSE_TRANSFORM_FEEDBACK, 0);
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ResumeTransformFeedback(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_RESUME_TRANSFORM_FEEDBACK, 0);
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_DrawTransformFeedback(GLenum mode, GLuint name)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DRAW_TRANSFORM_FEEDBACK, 2);
   if (n) {
      n[1].e = mode;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_DrawTransformFeedbackStream(GLenum mode, GLuint name, GLuint stream)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DRAW_TRANSFORM_FEEDBACK_STREAM, 3);
   if (n) {
      n[1].e = mode;
   n[2].ui = name;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_DrawTransformFeedbackInstanced(GLenum mode, GLuint name,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DRAW_TRANSFORM_FEEDBACK_INSTANCED, 3);
   if (n) {
      n[1].e = mode;
   n[2].ui = name;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_DrawTransformFeedbackStreamInstanced(GLenum mode, GLuint name,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DRAW_TRANSFORM_FEEDBACK_STREAM_INSTANCED, 4);
   if (n) {
      n[1].e = mode;
   n[2].ui = name;
   n[3].ui = stream;
      }
   if (ctx->ExecuteFlag) {
      CALL_DrawTransformFeedbackStreamInstanced(ctx->Dispatch.Exec, (mode, name, stream,
         }
      void GLAPIENTRY
   save_DispatchCompute(GLuint num_groups_x, GLuint num_groups_y,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DISPATCH_COMPUTE, 3);
   if (n) {
      n[1].ui = num_groups_x;
   n[2].ui = num_groups_y;
      }
   if (ctx->ExecuteFlag) {
      CALL_DispatchCompute(ctx->Dispatch.Exec, (num_groups_x, num_groups_y,
         }
      void GLAPIENTRY
   save_DispatchComputeIndirect(GLintptr indirect)
   {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
      }
      static void ALWAYS_INLINE
   save_Attr32bit(struct gl_context *ctx, unsigned attr, unsigned size,
         {
      Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   unsigned base_op;
            /* We don't care about GL_INT vs GL_UNSIGNED_INT. The idea is to get W=1
   * right for 3 or lower number of components, so only distinguish between
   * FLOAT and INT.
   */
   if (type == GL_FLOAT) {
      if (VERT_BIT(attr) & VERT_BIT_GENERIC_ALL) {
      base_op = OPCODE_ATTR_1F_ARB;
      } else {
            } else {
      base_op = OPCODE_ATTR_1I;
               n = alloc_instruction(ctx, base_op + size - 1, 1 + size);
   if (n) {
      n[1].ui = attr;
   n[2].ui = x;
   if (size >= 2) n[3].ui = y;
   if (size >= 3) n[4].ui = z;
               ctx->ListState.ActiveAttribSize[index] = size;
            if (ctx->ExecuteFlag) {
      if (type == GL_FLOAT) {
      if (base_op == OPCODE_ATTR_1F_NV) {
      if (size == 4)
         else if (size == 3)
         else if (size == 2)
         else
      } else {
      if (size == 4)
         else if (size == 3)
         else if (size == 2)
         else
         } else {
      if (size == 4)
         else if (size == 3)
         else if (size == 2)
         else
            }
      static void ALWAYS_INLINE
   save_Attr64bit(struct gl_context *ctx, unsigned attr, unsigned size,
         {
      Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   unsigned base_op;
            if (type == GL_DOUBLE) {
         } else {
      base_op = OPCODE_ATTR_1UI64;
               attr -= VERT_ATTRIB_GENERIC0;
   n = alloc_instruction(ctx, base_op + size - 1, 1 + size * 2);
   if (n) {
      n[1].ui = attr;
   ASSIGN_UINT64_TO_NODES(n, 2, x);
   if (size >= 2) ASSIGN_UINT64_TO_NODES(n, 4, y);
   if (size >= 3) ASSIGN_UINT64_TO_NODES(n, 6, z);
               ctx->ListState.ActiveAttribSize[index] = size;
            if (ctx->ExecuteFlag) {
      uint64_t v[] = {x, y, z, w};
   if (type == GL_DOUBLE) {
      if (size == 4)
         else if (size == 3)
         else if (size == 2)
         else
      } else {
               }
      /**
   * If index=0, does glVertexAttrib*() alias glVertex() to emit a vertex?
   * It depends on a few things, including whether we're inside or outside
   * of glBegin/glEnd.
   */
   static inline bool
   is_vertex_position(const struct gl_context *ctx, GLuint index)
   {
      return (index == 0 &&
            }
      /**
   * This macro is used to implement all the glVertex, glColor, glTexCoord,
   * glVertexAttrib, etc functions.
   * \param A  VBO_ATTRIB_x attribute index
   * \param N  attribute size (1..4)
   * \param T  type (GL_FLOAT, GL_DOUBLE, GL_INT, GL_UNSIGNED_INT)
   * \param C  cast type (uint32_t or uint64_t)
   * \param V0, V1, v2, V3  attribute value
   */
   #define ATTR_UNION(A, N, T, C, V0, V1, V2, V3)                          \
   do {                                                                    \
      if (sizeof(C) == 4) {                                                \
         } else {                                                             \
            } while (0)
      #undef ERROR
   #define ERROR(err) _mesa_error(ctx, err, __func__)
   #define TAG(x) save_##x
      #define VBO_ATTRIB_POS           VERT_ATTRIB_POS
   #define VBO_ATTRIB_NORMAL        VERT_ATTRIB_NORMAL
   #define VBO_ATTRIB_COLOR0        VERT_ATTRIB_COLOR0
   #define VBO_ATTRIB_COLOR1        VERT_ATTRIB_COLOR1
   #define VBO_ATTRIB_FOG           VERT_ATTRIB_FOG
   #define VBO_ATTRIB_COLOR_INDEX   VERT_ATTRIB_COLOR_INDEX
   #define VBO_ATTRIB_EDGEFLAG      VERT_ATTRIB_EDGEFLAG
   #define VBO_ATTRIB_TEX0          VERT_ATTRIB_TEX0
   #define VBO_ATTRIB_GENERIC0      VERT_ATTRIB_GENERIC0
   #define VBO_ATTRIB_MAX           VERT_ATTRIB_MAX
      #include "vbo/vbo_attrib_tmp.h"
      void GLAPIENTRY
   save_UseProgram(GLuint program)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_USE_PROGRAM, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform1f(GLint location, GLfloat x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1F, 2);
   if (n) {
      n[1].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform2f(GLint location, GLfloat x, GLfloat y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2F, 3);
   if (n) {
      n[1].i = location;
   n[2].f = x;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3F, 4);
   if (n) {
      n[1].i = location;
   n[2].f = x;
   n[3].f = y;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4F, 5);
   if (n) {
      n[1].i = location;
   n[2].f = x;
   n[3].f = y;
   n[4].f = z;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform1fv(GLint location, GLsizei count, const GLfloat *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1FV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform2fv(GLint location, GLsizei count, const GLfloat *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2FV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform3fv(GLint location, GLsizei count, const GLfloat *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3FV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform4fv(GLint location, GLsizei count, const GLfloat *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4FV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform1d(GLint location, GLdouble x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1D, 3);
   if (n) {
      n[1].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform2d(GLint location, GLdouble x, GLdouble y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2D, 5);
   if (n) {
      n[1].i = location;
   ASSIGN_DOUBLE_TO_NODES(n, 2, x);
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform3d(GLint location, GLdouble x, GLdouble y, GLdouble z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3D, 7);
   if (n) {
      n[1].i = location;
   ASSIGN_DOUBLE_TO_NODES(n, 2, x);
   ASSIGN_DOUBLE_TO_NODES(n, 4, y);
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform4d(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4D, 9);
   if (n) {
      n[1].i = location;
   ASSIGN_DOUBLE_TO_NODES(n, 2, x);
   ASSIGN_DOUBLE_TO_NODES(n, 4, y);
   ASSIGN_DOUBLE_TO_NODES(n, 6, z);
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform1dv(GLint location, GLsizei count, const GLdouble *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1DV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform2dv(GLint location, GLsizei count, const GLdouble *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2DV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform3dv(GLint location, GLsizei count, const GLdouble *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3DV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform4dv(GLint location, GLsizei count, const GLdouble *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4DV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_Uniform1i(GLint location, GLint x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1I, 2);
   if (n) {
      n[1].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform2i(GLint location, GLint x, GLint y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2I, 3);
   if (n) {
      n[1].i = location;
   n[2].i = x;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform3i(GLint location, GLint x, GLint y, GLint z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3I, 4);
   if (n) {
      n[1].i = location;
   n[2].i = x;
   n[3].i = y;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4I, 5);
   if (n) {
      n[1].i = location;
   n[2].i = x;
   n[3].i = y;
   n[4].i = z;
      }
   if (ctx->ExecuteFlag) {
            }
            void GLAPIENTRY
   save_Uniform1iv(GLint location, GLsizei count, const GLint *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1IV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform2iv(GLint location, GLsizei count, const GLint *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2IV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform3iv(GLint location, GLsizei count, const GLint *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3IV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform4iv(GLint location, GLsizei count, const GLint *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4IV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
            void GLAPIENTRY
   save_Uniform1ui(GLint location, GLuint x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1UI, 2);
   if (n) {
      n[1].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform2ui(GLint location, GLuint x, GLuint y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2UI, 3);
   if (n) {
      n[1].i = location;
   n[2].i = x;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform3ui(GLint location, GLuint x, GLuint y, GLuint z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3UI, 4);
   if (n) {
      n[1].i = location;
   n[2].i = x;
   n[3].i = y;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform4ui(GLint location, GLuint x, GLuint y, GLuint z, GLuint w)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4UI, 5);
   if (n) {
      n[1].i = location;
   n[2].i = x;
   n[3].i = y;
   n[4].i = z;
      }
   if (ctx->ExecuteFlag) {
            }
            void GLAPIENTRY
   save_Uniform1uiv(GLint location, GLsizei count, const GLuint *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1UIV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform2uiv(GLint location, GLsizei count, const GLuint *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2UIV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform3uiv(GLint location, GLsizei count, const GLuint *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3UIV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform4uiv(GLint location, GLsizei count, const GLuint *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4UIV, 2 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
            void GLAPIENTRY
   save_UniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX22, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_UniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX33, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX44, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX23, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX32, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX24, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX42, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX34, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX43, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_UniformMatrix2dv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX22D, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_UniformMatrix3dv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX33D, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_UniformMatrix4dv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX44D, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_UniformMatrix2x3dv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX23D, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_UniformMatrix3x2dv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX32D, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_UniformMatrix2x4dv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX24D, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_UniformMatrix4x2dv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX42D, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_UniformMatrix3x4dv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX34D, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_UniformMatrix4x3dv(GLint location, GLsizei count, GLboolean transpose,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX43D, 3 + POINTER_DWORDS);
   if (n) {
      n[1].i = location;
   n[2].i = count;
   n[3].b = transpose;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform1i64ARB(GLint location, GLint64 x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1I64, 3);
   if (n) {
      n[1].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform2i64ARB(GLint location, GLint64 x, GLint64 y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2I64, 5);
   if (n) {
      n[1].i = location;
   ASSIGN_INT64_TO_NODES(n, 2, x);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform3i64ARB(GLint location, GLint64 x, GLint64 y, GLint64 z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3I64, 7);
   if (n) {
      n[1].i = location;
   ASSIGN_INT64_TO_NODES(n, 2, x);
   ASSIGN_INT64_TO_NODES(n, 4, y);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform4i64ARB(GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4I64, 9);
   if (n) {
      n[1].i = location;
   ASSIGN_INT64_TO_NODES(n, 2, x);
   ASSIGN_INT64_TO_NODES(n, 4, y);
   ASSIGN_INT64_TO_NODES(n, 6, z);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform1i64vARB(GLint location, GLsizei count, const GLint64 *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1I64V, 2 + POINTER_DWORDS);
   if (n) {
   n[1].i = location;
   n[2].i = count;
   save_pointer(&n[3], memdup(v, count * 1 * sizeof(GLint64)));
   }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform2i64vARB(GLint location, GLsizei count, const GLint64 *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2I64V, 2 + POINTER_DWORDS);
   if (n) {
   n[1].i = location;
   n[2].i = count;
   save_pointer(&n[3], memdup(v, count * 2 * sizeof(GLint64)));
   }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform3i64vARB(GLint location, GLsizei count, const GLint64 *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3I64V, 2 + POINTER_DWORDS);
   if (n) {
   n[1].i = location;
   n[2].i = count;
   save_pointer(&n[3], memdup(v, count * 3 * sizeof(GLint64)));
   }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform4i64vARB(GLint location, GLsizei count, const GLint64 *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4I64V, 2 + POINTER_DWORDS);
   if (n) {
   n[1].i = location;
   n[2].i = count;
   save_pointer(&n[3], memdup(v, count * 4 * sizeof(GLint64)));
   }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform1ui64ARB(GLint location, GLuint64 x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1UI64, 3);
   if (n) {
      n[1].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform2ui64ARB(GLint location, GLuint64 x, GLuint64 y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2UI64, 5);
   if (n) {
      n[1].i = location;
   ASSIGN_UINT64_TO_NODES(n, 2, x);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform3ui64ARB(GLint location, GLuint64 x, GLuint64 y, GLuint64 z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3UI64, 7);
   if (n) {
      n[1].i = location;
   ASSIGN_UINT64_TO_NODES(n, 2, x);
   ASSIGN_UINT64_TO_NODES(n, 4, y);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform4ui64ARB(GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4UI64, 9);
   if (n) {
      n[1].i = location;
   ASSIGN_UINT64_TO_NODES(n, 2, x);
   ASSIGN_UINT64_TO_NODES(n, 4, y);
   ASSIGN_UINT64_TO_NODES(n, 6, z);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform1ui64vARB(GLint location, GLsizei count, const GLuint64 *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1UI64V, 2 + POINTER_DWORDS);
   if (n) {
   n[1].i = location;
   n[2].i = count;
   save_pointer(&n[3], memdup(v, count * 1 * sizeof(GLuint64)));
   }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform2ui64vARB(GLint location, GLsizei count, const GLuint64 *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2UI64V, 2 + POINTER_DWORDS);
   if (n) {
   n[1].i = location;
   n[2].i = count;
   save_pointer(&n[3], memdup(v, count * 2 * sizeof(GLuint64)));
   }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform3ui64vARB(GLint location, GLsizei count, const GLuint64 *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3UI64V, 2 + POINTER_DWORDS);
   if (n) {
   n[1].i = location;
   n[2].i = count;
   save_pointer(&n[3], memdup(v, count * 3 * sizeof(GLuint64)));
   }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_Uniform4ui64vARB(GLint location, GLsizei count, const GLuint64 *v)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4UI64V, 2 + POINTER_DWORDS);
   if (n) {
   n[1].i = location;
   n[2].i = count;
   save_pointer(&n[3], memdup(v, count * 4 * sizeof(GLuint64)));
   }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1i64ARB(GLuint program, GLint location, GLint64 x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1I64, 4);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2i64ARB(GLuint program, GLint location, GLint64 x,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2I64, 6);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   ASSIGN_INT64_TO_NODES(n, 3, x);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3i64ARB(GLuint program, GLint location, GLint64 x,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3I64, 8);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   ASSIGN_INT64_TO_NODES(n, 3, x);
   ASSIGN_INT64_TO_NODES(n, 5, y);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4i64ARB(GLuint program, GLint location, GLint64 x,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4I64, 10);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   ASSIGN_INT64_TO_NODES(n, 3, x);
   ASSIGN_INT64_TO_NODES(n, 5, y);
   ASSIGN_INT64_TO_NODES(n, 7, z);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1i64vARB(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1I64V, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2i64vARB(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2I64V, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3i64vARB(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3I64V, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4i64vARB(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4I64V, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1ui64ARB(GLuint program, GLint location, GLuint64 x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1UI64, 4);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2ui64ARB(GLuint program, GLint location, GLuint64 x,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2UI64, 6);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   ASSIGN_UINT64_TO_NODES(n, 3, x);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3ui64ARB(GLuint program, GLint location, GLuint64 x,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3UI64, 8);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   ASSIGN_UINT64_TO_NODES(n, 3, x);
   ASSIGN_UINT64_TO_NODES(n, 5, y);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4ui64ARB(GLuint program, GLint location, GLuint64 x,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4UI64, 10);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   ASSIGN_UINT64_TO_NODES(n, 3, x);
   ASSIGN_UINT64_TO_NODES(n, 5, y);
   ASSIGN_UINT64_TO_NODES(n, 7, z);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1ui64vARB(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1UI64V,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2ui64vARB(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2UI64V,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3ui64vARB(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3UI64V,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4ui64vARB(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4UI64V,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_UseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_USE_PROGRAM_STAGES, 3);
   if (n) {
      n[1].ui = pipeline;
   n[2].ui = stages;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1f(GLuint program, GLint location, GLfloat x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1F, 3);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2f(GLuint program, GLint location, GLfloat x, GLfloat y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2F, 4);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].f = x;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3f(GLuint program, GLint location,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3F, 5);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].f = x;
   n[4].f = y;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4f(GLuint program, GLint location,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4F, 6);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].f = x;
   n[4].f = y;
   n[5].f = z;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1FV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2FV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3FV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4FV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1d(GLuint program, GLint location, GLdouble x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1D, 4);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2d(GLuint program, GLint location, GLdouble x, GLdouble y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2D, 6);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   ASSIGN_DOUBLE_TO_NODES(n, 3, x);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3d(GLuint program, GLint location,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3D, 8);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   ASSIGN_DOUBLE_TO_NODES(n, 3, x);
   ASSIGN_DOUBLE_TO_NODES(n, 5, y);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4d(GLuint program, GLint location,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4D, 10);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   ASSIGN_DOUBLE_TO_NODES(n, 3, x);
   ASSIGN_DOUBLE_TO_NODES(n, 5, y);
   ASSIGN_DOUBLE_TO_NODES(n, 7, z);
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1DV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2DV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3DV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4DV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1i(GLuint program, GLint location, GLint x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1I, 3);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2i(GLuint program, GLint location, GLint x, GLint y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2I, 4);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = x;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3i(GLuint program, GLint location,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3I, 5);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = x;
   n[4].i = y;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4i(GLuint program, GLint location,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4I, 6);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = x;
   n[4].i = y;
   n[5].i = z;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1iv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1IV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2iv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2IV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3iv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3IV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4iv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4IV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1ui(GLuint program, GLint location, GLuint x)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1UI, 3);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2ui(GLuint program, GLint location, GLuint x, GLuint y)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2UI, 4);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].ui = x;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3ui(GLuint program, GLint location,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3UI, 5);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].ui = x;
   n[4].ui = y;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4ui(GLuint program, GLint location,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4UI, 6);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].ui = x;
   n[4].ui = y;
   n[5].ui = z;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform1uiv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_1UIV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform2uiv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_2UIV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform3uiv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_3UIV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniform4uiv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_4UIV, 3 + POINTER_DWORDS);
   if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX22F,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix2fv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX23F,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix2x3fv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX24F,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix2x4fv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX32F,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix3x2fv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX33F,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix3fv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX34F,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix3x4fv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX42F,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix4x2fv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX43F,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix4x3fv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX44F,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix4fv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX22D,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix2dv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX23D,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix2x3dv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX24D,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix2x4dv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX32D,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix3x2dv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX33D,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix3dv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX34D,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix3x4dv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX42D,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix4x2dv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX43D,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix4x3dv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_UNIFORM_MATRIX44D,
         if (n) {
      n[1].ui = program;
   n[2].i = location;
   n[3].i = count;
   n[4].b = transpose;
      }
   if (ctx->ExecuteFlag) {
      CALL_ProgramUniformMatrix4dv(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_ClipControl(GLenum origin, GLenum depth)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLIP_CONTROL, 2);
   if (n) {
      n[1].e = origin;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_ClampColor(GLenum target, GLenum clamp)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLAMP_COLOR, 2);
   if (n) {
      n[1].e = target;
      }
   if (ctx->ExecuteFlag) {
            }
      /** GL_EXT_texture_integer */
   void GLAPIENTRY
   save_ClearColorIiEXT(GLint red, GLint green, GLint blue, GLint alpha)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEARCOLOR_I, 4);
   if (n) {
      n[1].i = red;
   n[2].i = green;
   n[3].i = blue;
      }
   if (ctx->ExecuteFlag) {
            }
      /** GL_EXT_texture_integer */
   void GLAPIENTRY
   save_ClearColorIuiEXT(GLuint red, GLuint green, GLuint blue, GLuint alpha)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEARCOLOR_UI, 4);
   if (n) {
      n[1].ui = red;
   n[2].ui = green;
   n[3].ui = blue;
      }
   if (ctx->ExecuteFlag) {
            }
      /** GL_EXT_texture_integer */
   void GLAPIENTRY
   save_TexParameterIiv(GLenum target, GLenum pname, const GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXPARAMETER_I, 6);
   if (n) {
      n[1].e = target;
   n[2].e = pname;
   n[3].i = params[0];
   n[4].i = params[1];
   n[5].i = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
      /** GL_EXT_texture_integer */
   void GLAPIENTRY
   save_TexParameterIuiv(GLenum target, GLenum pname, const GLuint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXPARAMETER_UI, 6);
   if (n) {
      n[1].e = target;
   n[2].e = pname;
   n[3].ui = params[0];
   n[4].ui = params[1];
   n[5].ui = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
      /* GL_EXT/ARB_instanced_arrays */
   void GLAPIENTRY
   save_VertexAttribDivisor(GLuint index, GLuint divisor)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_VERTEX_ATTRIB_DIVISOR, 2);
   if (n) {
      n[1].ui = index;
      }
   if (ctx->ExecuteFlag) {
            }
         /* GL_NV_texture_barrier */
   void GLAPIENTRY
   save_TextureBarrierNV(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   alloc_instruction(ctx, OPCODE_TEXTURE_BARRIER_NV, 0);
   if (ctx->ExecuteFlag) {
            }
         /* GL_ARB_sampler_objects */
   void GLAPIENTRY
   save_BindSampler(GLuint unit, GLuint sampler)
   {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BIND_SAMPLER, 2);
   if (n) {
      n[1].ui = unit;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_SamplerParameteriv(GLuint sampler, GLenum pname, const GLint *params)
   {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SAMPLER_PARAMETERIV, 6);
   if (n) {
      n[1].ui = sampler;
   n[2].e = pname;
   n[3].i = params[0];
   if (pname == GL_TEXTURE_BORDER_COLOR) {
      n[4].i = params[1];
   n[5].i = params[2];
      }
   else {
            }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_SamplerParameteri(GLuint sampler, GLenum pname, GLint param)
   {
      GLint parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0;
      }
      void GLAPIENTRY
   save_SamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *params)
   {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SAMPLER_PARAMETERFV, 6);
   if (n) {
      n[1].ui = sampler;
   n[2].e = pname;
   n[3].f = params[0];
   if (pname == GL_TEXTURE_BORDER_COLOR) {
      n[4].f = params[1];
   n[5].f = params[2];
      }
   else {
            }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_SamplerParameterf(GLuint sampler, GLenum pname, GLfloat param)
   {
      GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
      }
      void GLAPIENTRY
   save_SamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *params)
   {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SAMPLER_PARAMETERIIV, 6);
   if (n) {
      n[1].ui = sampler;
   n[2].e = pname;
   n[3].i = params[0];
   if (pname == GL_TEXTURE_BORDER_COLOR) {
      n[4].i = params[1];
   n[5].i = params[2];
      }
   else {
            }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_SamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *params)
   {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SAMPLER_PARAMETERUIV, 6);
   if (n) {
      n[1].ui = sampler;
   n[2].e = pname;
   n[3].ui = params[0];
   if (pname == GL_TEXTURE_BORDER_COLOR) {
      n[4].ui = params[1];
   n[5].ui = params[2];
      }
   else {
            }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_WaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
   {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_WAIT_SYNC, 4);
   if (n) {
      union uint64_pair p;
   p.uint64 = timeout;
   n[1].bf = flags;
   n[2].ui = p.uint32[0];
   n[3].ui = p.uint32[1];
      }
   if (ctx->ExecuteFlag) {
            }
         /** GL_NV_conditional_render */
   void GLAPIENTRY
   save_BeginConditionalRender(GLuint queryId, GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BEGIN_CONDITIONAL_RENDER, 2);
   if (n) {
      n[1].i = queryId;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_EndConditionalRender(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   alloc_instruction(ctx, OPCODE_END_CONDITIONAL_RENDER, 0);
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_UniformBlockBinding(GLuint prog, GLuint index, GLuint binding)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_BLOCK_BINDING, 3);
   if (n) {
      n[1].ui = prog;
   n[2].ui = index;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_UniformSubroutinesuiv(GLenum shadertype, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_SUBROUTINES, 2 + POINTER_DWORDS);
   if (n) {
               if (count > 0)
         n[1].e = shadertype;
   n[2].si = count;
      }
   if (ctx->ExecuteFlag) {
            }
      /** GL_EXT_window_rectangles */
   void GLAPIENTRY
   save_WindowRectanglesEXT(GLenum mode, GLsizei count, const GLint *box)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_WINDOW_RECTANGLES, 2 + POINTER_DWORDS);
   if (n) {
               if (count > 0)
         n[1].e = mode;
   n[2].si = count;
      }
   if (ctx->ExecuteFlag) {
            }
         /** GL_NV_conservative_raster */
   void GLAPIENTRY
   save_SubpixelPrecisionBiasNV(GLuint xbits, GLuint ybits)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SUBPIXEL_PRECISION_BIAS, 2);
   if (n) {
      n[1].ui = xbits;
      }
   if (ctx->ExecuteFlag) {
            }
      /** GL_NV_conservative_raster_dilate */
   void GLAPIENTRY
   save_ConservativeRasterParameterfNV(GLenum pname, GLfloat param)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CONSERVATIVE_RASTER_PARAMETER_F, 2);
   if (n) {
      n[1].e = pname;
      }
   if (ctx->ExecuteFlag) {
            }
      /** GL_NV_conservative_raster_pre_snap_triangles */
   void GLAPIENTRY
   save_ConservativeRasterParameteriNV(GLenum pname, GLint param)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CONSERVATIVE_RASTER_PARAMETER_I, 2);
   if (n) {
      n[1].e = pname;
      }
   if (ctx->ExecuteFlag) {
            }
      /** GL_EXT_direct_state_access */
      void GLAPIENTRY
   save_MatrixLoadfEXT(GLenum matrixMode, const GLfloat *m)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_LOAD, 17);
   if (n) {
      n[1].e = matrixMode;
   for (unsigned i = 0; i < 16; i++) {
            }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MatrixLoaddEXT(GLenum matrixMode, const GLdouble *m)
   {
      GLfloat f[16];
   for (unsigned i = 0; i < 16; i++) {
         }
      }
      void GLAPIENTRY
   save_MatrixMultfEXT(GLenum matrixMode, const GLfloat * m)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_MULT, 17);
   if (n) {
      n[1].e = matrixMode;
   for (unsigned i = 0; i < 16; i++) {
            }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MatrixMultdEXT(GLenum matrixMode, const GLdouble * m)
   {
      GLfloat f[16];
   for (unsigned i = 0; i < 16; i++) {
         }
      }
      void GLAPIENTRY
   save_MatrixRotatefEXT(GLenum matrixMode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_ROTATE, 5);
   if (n) {
      n[1].e = matrixMode;
   n[2].f = angle;
   n[3].f = x;
   n[4].f = y;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MatrixRotatedEXT(GLenum matrixMode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
   {
         }
      void GLAPIENTRY
   save_MatrixScalefEXT(GLenum matrixMode, GLfloat x, GLfloat y, GLfloat z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_SCALE, 4);
   if (n) {
      n[1].e = matrixMode;
   n[2].f = x;
   n[3].f = y;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MatrixScaledEXT(GLenum matrixMode, GLdouble x, GLdouble y, GLdouble z)
   {
         }
      void GLAPIENTRY
   save_MatrixTranslatefEXT(GLenum matrixMode, GLfloat x, GLfloat y, GLfloat z)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_TRANSLATE, 4);
   if (n) {
      n[1].e = matrixMode;
   n[2].f = x;
   n[3].f = y;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MatrixTranslatedEXT(GLenum matrixMode, GLdouble x, GLdouble y, GLdouble z)
   {
         }
      void GLAPIENTRY
   save_MatrixLoadIdentityEXT(GLenum matrixMode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_LOAD_IDENTITY, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MatrixOrthoEXT(GLenum matrixMode, GLdouble left, GLdouble right,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_ORTHO, 7);
   if (n) {
      n[1].e = matrixMode;
   n[2].f = (GLfloat) left;
   n[3].f = (GLfloat) right;
   n[4].f = (GLfloat) bottom;
   n[5].f = (GLfloat) top;
   n[6].f = (GLfloat) nearval;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_MatrixFrustumEXT(GLenum matrixMode, GLdouble left, GLdouble right,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_FRUSTUM, 7);
   if (n) {
      n[1].e = matrixMode;
   n[2].f = (GLfloat) left;
   n[3].f = (GLfloat) right;
   n[4].f = (GLfloat) bottom;
   n[5].f = (GLfloat) top;
   n[6].f = (GLfloat) nearval;
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MatrixPushEXT(GLenum matrixMode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node* n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_PUSH, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MatrixPopEXT(GLenum matrixMode)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node* n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_POP, 1);
   if (n) {
         }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MatrixLoadTransposefEXT(GLenum matrixMode, const GLfloat *m)
   {
      GLfloat tm[16];
   _math_transposef(tm, m);
      }
      void GLAPIENTRY
   save_MatrixLoadTransposedEXT(GLenum matrixMode, const GLdouble *m)
   {
      GLfloat tm[16];
   _math_transposefd(tm, m);
      }
      void GLAPIENTRY
   save_MatrixMultTransposefEXT(GLenum matrixMode, const GLfloat *m)
   {
      GLfloat tm[16];
   _math_transposef(tm, m);
      }
      void GLAPIENTRY
   save_MatrixMultTransposedEXT(GLenum matrixMode, const GLdouble *m)
   {
      GLfloat tm[16];
   _math_transposefd(tm, m);
      }
      void GLAPIENTRY
   save_TextureParameterfvEXT(GLuint texture, GLenum target, GLenum pname,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXTUREPARAMETER_F, 7);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].e = pname;
   n[4].f = params[0];
   n[5].f = params[1];
   n[6].f = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_TextureParameterfEXT(GLuint texture, GLenum target, GLenum pname, GLfloat param)
   {
      GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
      }
      void GLAPIENTRY
   save_TextureParameterivEXT(GLuint texture, GLenum target, GLenum pname, const GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXTUREPARAMETER_I, 7);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].e = pname;
   n[4].i = params[0];
   n[5].i = params[1];
   n[6].i = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_TextureParameteriEXT(GLuint texture, GLenum target, GLenum pname, GLint param)
   {
      GLint fparam[4];
   fparam[0] = param;
   fparam[1] = fparam[2] = fparam[3] = 0;
      }
      void GLAPIENTRY
   save_TextureParameterIivEXT(GLuint texture, GLenum target, GLenum pname, const GLint* params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXTUREPARAMETER_II, 7);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].e = pname;
   n[4].i = params[0];
   n[5].i = params[1];
   n[6].i = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_TextureParameterIuivEXT(GLuint texture, GLenum target, GLenum pname, const GLuint* params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXTUREPARAMETER_IUI, 7);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].e = pname;
   n[4].ui = params[0];
   n[5].ui = params[1];
   n[6].ui = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_TextureImage1DEXT(GLuint texture, GLenum target,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D) {
      /* don't compile, execute immediately */
   CALL_TextureImage1DEXT(ctx->Dispatch.Exec, (texture, target, level, components, width,
      }
   else {
      Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXTURE_IMAGE1D, 8 + POINTER_DWORDS);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = components;
   n[5].i = (GLint) width;
   n[6].i = border;
   n[7].e = format;
   n[8].e = type;
   save_pointer(&n[9],
            }
   if (ctx->ExecuteFlag) {
      CALL_TextureImage1DEXT(ctx->Dispatch.Exec, (texture, target, level, components, width,
            }
         void GLAPIENTRY
   save_TextureImage2DEXT(GLuint texture, GLenum target,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_2D) {
      /* don't compile, execute immediately */
   CALL_TextureImage2DEXT(ctx->Dispatch.Exec, (texture, target, level, components, width,
      }
   else {
      Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXTURE_IMAGE2D, 9 + POINTER_DWORDS);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = components;
   n[5].i = (GLint) width;
   n[6].i = (GLint) height;
   n[7].i = border;
   n[8].e = format;
   n[9].e = type;
   save_pointer(&n[10],
            }
   if (ctx->ExecuteFlag) {
      CALL_TextureImage2DEXT(ctx->Dispatch.Exec, (texture, target, level, components, width,
            }
         void GLAPIENTRY
   save_TextureImage3DEXT(GLuint texture, GLenum target,
                           {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_3D) {
      /* don't compile, execute immediately */
   CALL_TextureImage3DEXT(ctx->Dispatch.Exec, (texture, target, level, internalFormat, width,
            }
   else {
      Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXTURE_IMAGE3D, 10 + POINTER_DWORDS);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = (GLint) internalFormat;
   n[5].i = (GLint) width;
   n[6].i = (GLint) height;
   n[7].i = (GLint) depth;
   n[8].i = border;
   n[9].e = format;
   n[10].e = type;
   save_pointer(&n[11],
            }
   if (ctx->ExecuteFlag) {
      CALL_TextureImage3DEXT(ctx->Dispatch.Exec, (texture, target, level, internalFormat,
                  }
         void GLAPIENTRY
   save_TextureSubImage1DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset,
               {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_TEXTURE_SUB_IMAGE1D, 7 + POINTER_DWORDS);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = (GLint) width;
   n[6].e = format;
   n[7].e = type;
   save_pointer(&n[8],
            }
   if (ctx->ExecuteFlag) {
      CALL_TextureSubImage1DEXT(ctx->Dispatch.Exec, (texture, target, level, xoffset, width,
         }
         void GLAPIENTRY
   save_TextureSubImage2DEXT(GLuint texture, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_TEXTURE_SUB_IMAGE2D, 9 + POINTER_DWORDS);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = (GLint) width;
   n[7].i = (GLint) height;
   n[8].e = format;
   n[9].e = type;
   save_pointer(&n[10],
            }
   if (ctx->ExecuteFlag) {
      CALL_TextureSubImage2DEXT(ctx->Dispatch.Exec, (texture, target, level, xoffset, yoffset,
         }
         void GLAPIENTRY
   save_TextureSubImage3DEXT(GLuint texture, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_TEXTURE_SUB_IMAGE3D, 11 + POINTER_DWORDS);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = zoffset;
   n[7].i = (GLint) width;
   n[8].i = (GLint) height;
   n[9].i = (GLint) depth;
   n[10].e = format;
   n[11].e = type;
   save_pointer(&n[12],
            }
   if (ctx->ExecuteFlag) {
      CALL_TextureSubImage3DEXT(ctx->Dispatch.Exec, (texture, target, level,
                     }
      void GLAPIENTRY
   save_CopyTextureImage1DEXT(GLuint texture, GLenum target, GLint level,
               {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEXTURE_IMAGE1D, 8);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].e = internalformat;
   n[5].i = x;
   n[6].i = y;
   n[7].i = width;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyTextureImage1DEXT(ctx->Dispatch.Exec, (texture, target, level,
               }
      void GLAPIENTRY
   save_CopyTextureImage2DEXT(GLuint texture, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEXTURE_IMAGE2D, 9);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].e = internalformat;
   n[5].i = x;
   n[6].i = y;
   n[7].i = width;
   n[8].i = height;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyTextureImage2DEXT(ctx->Dispatch.Exec, (texture, target, level,
               }
      void GLAPIENTRY
   save_CopyTextureSubImage1DEXT(GLuint texture, GLenum target, GLint level,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEXTURE_SUB_IMAGE1D, 7);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = x;
   n[6].i = y;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyTextureSubImage1DEXT(ctx->Dispatch.Exec,
         }
      void GLAPIENTRY
   save_CopyTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level,
               {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEXTURE_SUB_IMAGE2D, 9);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = x;
   n[7].i = y;
   n[8].i = width;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyTextureSubImage2DEXT(ctx->Dispatch.Exec, (texture, target, level,
               }
         void GLAPIENTRY
   save_CopyTextureSubImage3DEXT(GLuint texture, GLenum target, GLint level,
               {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEXTURE_SUB_IMAGE3D, 10);
   if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = zoffset;
   n[7].i = x;
   n[8].i = y;
   n[9].i = width;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyTextureSubImage3DEXT(ctx->Dispatch.Exec, (texture, target, level,
               }
         void GLAPIENTRY
   save_BindMultiTextureEXT(GLenum texunit, GLenum target, GLuint texture)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BIND_MULTITEXTURE, 3);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_MultiTexParameterfvEXT(GLenum texunit, GLenum target, GLenum pname,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MULTITEXPARAMETER_F, 7);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].e = pname;
   n[4].f = params[0];
   n[5].f = params[1];
   n[6].f = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_MultiTexParameterfEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat param)
   {
      GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
      }
      void GLAPIENTRY
   save_MultiTexParameterivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MULTITEXPARAMETER_I, 7);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].e = pname;
   n[4].i = params[0];
   n[5].i = params[1];
   n[6].i = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MultiTexParameterIivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MULTITEXPARAMETER_II, 7);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].e = pname;
   n[4].i = params[0];
   n[5].i = params[1];
   n[6].i = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MultiTexParameterIuivEXT(GLenum texunit, GLenum target, GLenum pname, const GLuint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MULTITEXPARAMETER_IUI, 7);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].e = pname;
   n[4].ui = params[0];
   n[5].ui = params[1];
   n[6].ui = params[2];
      }
   if (ctx->ExecuteFlag) {
            }
      void GLAPIENTRY
   save_MultiTexParameteriEXT(GLenum texunit, GLenum target, GLenum pname, GLint param)
   {
      GLint fparam[4];
   fparam[0] = param;
   fparam[1] = fparam[2] = fparam[3] = 0;
      }
         void GLAPIENTRY
   save_MultiTexImage1DEXT(GLenum texunit, GLenum target,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D) {
      /* don't compile, execute immediately */
   CALL_MultiTexImage1DEXT(ctx->Dispatch.Exec, (texunit, target, level, components, width,
      }
   else {
      Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MULTITEX_IMAGE1D, 8 + POINTER_DWORDS);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = components;
   n[5].i = (GLint) width;
   n[6].i = border;
   n[7].e = format;
   n[8].e = type;
   save_pointer(&n[9],
            }
   if (ctx->ExecuteFlag) {
      CALL_MultiTexImage1DEXT(ctx->Dispatch.Exec, (texunit, target, level, components, width,
            }
         void GLAPIENTRY
   save_MultiTexImage2DEXT(GLenum texunit, GLenum target,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_2D) {
      /* don't compile, execute immediately */
   CALL_MultiTexImage2DEXT(ctx->Dispatch.Exec, (texunit, target, level, components, width,
      }
   else {
      Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MULTITEX_IMAGE2D, 9 + POINTER_DWORDS);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = components;
   n[5].i = (GLint) width;
   n[6].i = (GLint) height;
   n[7].i = border;
   n[8].e = format;
   n[9].e = type;
   save_pointer(&n[10],
            }
   if (ctx->ExecuteFlag) {
      CALL_MultiTexImage2DEXT(ctx->Dispatch.Exec, (texunit, target, level, components, width,
            }
         void GLAPIENTRY
   save_MultiTexImage3DEXT(GLenum texunit, GLenum target,
                           {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_3D) {
      /* don't compile, execute immediately */
   CALL_MultiTexImage3DEXT(ctx->Dispatch.Exec, (texunit, target, level, internalFormat, width,
            }
   else {
      Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MULTITEX_IMAGE3D, 10 + POINTER_DWORDS);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = (GLint) internalFormat;
   n[5].i = (GLint) width;
   n[6].i = (GLint) height;
   n[7].i = (GLint) depth;
   n[8].i = border;
   n[9].e = format;
   n[10].e = type;
   save_pointer(&n[11],
            }
   if (ctx->ExecuteFlag) {
      CALL_MultiTexImage3DEXT(ctx->Dispatch.Exec, (texunit, target, level, internalFormat,
                  }
         void GLAPIENTRY
   save_MultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset,
               {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_MULTITEX_SUB_IMAGE1D, 7 + POINTER_DWORDS);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = (GLint) width;
   n[6].e = format;
   n[7].e = type;
   save_pointer(&n[8],
            }
   if (ctx->ExecuteFlag) {
      CALL_MultiTexSubImage1DEXT(ctx->Dispatch.Exec, (texunit, target, level, xoffset, width,
         }
         void GLAPIENTRY
   save_MultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_MULTITEX_SUB_IMAGE2D, 9 + POINTER_DWORDS);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = (GLint) width;
   n[7].i = (GLint) height;
   n[8].e = format;
   n[9].e = type;
   save_pointer(&n[10],
            }
   if (ctx->ExecuteFlag) {
      CALL_MultiTexSubImage2DEXT(ctx->Dispatch.Exec, (texunit, target, level, xoffset, yoffset,
         }
         void GLAPIENTRY
   save_MultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_MULTITEX_SUB_IMAGE3D, 11 + POINTER_DWORDS);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = zoffset;
   n[7].i = (GLint) width;
   n[8].i = (GLint) height;
   n[9].i = (GLint) depth;
   n[10].e = format;
   n[11].e = type;
   save_pointer(&n[12],
            }
   if (ctx->ExecuteFlag) {
      CALL_MultiTexSubImage3DEXT(ctx->Dispatch.Exec, (texunit, target, level,
                     }
         void GLAPIENTRY
   save_CopyMultiTexImage1DEXT(GLenum texunit, GLenum target, GLint level,
               {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_MULTITEX_IMAGE1D, 8);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].e = internalformat;
   n[5].i = x;
   n[6].i = y;
   n[7].i = width;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyMultiTexImage1DEXT(ctx->Dispatch.Exec, (texunit, target, level,
               }
         void GLAPIENTRY
   save_CopyMultiTexImage2DEXT(GLenum texunit, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_MULTITEX_IMAGE2D, 9);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].e = internalformat;
   n[5].i = x;
   n[6].i = y;
   n[7].i = width;
   n[8].i = height;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyMultiTexImage2DEXT(ctx->Dispatch.Exec, (texunit, target, level,
               }
         void GLAPIENTRY
   save_CopyMultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_MULTITEX_SUB_IMAGE1D, 7);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = x;
   n[6].i = y;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyMultiTexSubImage1DEXT(ctx->Dispatch.Exec,
         }
         void GLAPIENTRY
   save_CopyMultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level,
               {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_MULTITEX_SUB_IMAGE2D, 9);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = x;
   n[7].i = y;
   n[8].i = width;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyMultiTexSubImage2DEXT(ctx->Dispatch.Exec, (texunit, target, level,
               }
         void GLAPIENTRY
   save_CopyMultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level,
               {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_MULTITEX_SUB_IMAGE3D, 10);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = zoffset;
   n[7].i = x;
   n[8].i = y;
   n[9].i = width;
      }
   if (ctx->ExecuteFlag) {
      CALL_CopyMultiTexSubImage3DEXT(ctx->Dispatch.Exec, (texunit, target, level,
               }
         void GLAPIENTRY
   save_MultiTexEnvfvEXT(GLenum texunit, GLenum target, GLenum pname, const GLfloat *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MULTITEXENV, 7);
   if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].e = pname;
   if (pname == GL_TEXTURE_ENV_COLOR) {
      n[4].f = params[0];
   n[5].f = params[1];
   n[6].f = params[2];
      }
   else {
      n[4].f = params[0];
         }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_MultiTexEnvfEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat param)
   {
      GLfloat parray[4];
   parray[0] = (GLfloat) param;
   parray[1] = parray[2] = parray[3] = 0.0F;
      }
         void GLAPIENTRY
   save_MultiTexEnviEXT(GLenum texunit, GLenum target, GLenum pname, GLint param)
   {
      GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0F;
      }
         void GLAPIENTRY
   save_MultiTexEnvivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint * param)
   {
      GLfloat p[4];
   if (pname == GL_TEXTURE_ENV_COLOR) {
      p[0] = INT_TO_FLOAT(param[0]);
   p[1] = INT_TO_FLOAT(param[1]);
   p[2] = INT_TO_FLOAT(param[2]);
      }
   else {
      p[0] = (GLfloat) param[0];
      }
      }
         void GLAPIENTRY
   save_CompressedTextureImage1DEXT(GLuint texture, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D) {
      /* don't compile, execute immediately */
   CALL_CompressedTextureImage1DEXT(ctx->Dispatch.Exec, (texture, target, level,
                  }
   else {
      Node *n;
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEXTURE_IMAGE_1D,
         if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].e = internalFormat;
   n[5].i = (GLint) width;
   n[6].i = border;
   n[7].i = imageSize;
   save_pointer(&n[8],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTextureImage1DEXT(ctx->Dispatch.Exec,
                  }
         void GLAPIENTRY
   save_CompressedTextureImage2DEXT(GLuint texture, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_2D) {
      /* don't compile, execute immediately */
   CALL_CompressedTextureImage2DEXT(ctx->Dispatch.Exec, (texture, target, level,
            }
   else {
      Node *n;
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEXTURE_IMAGE_2D,
         if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].e = internalFormat;
   n[5].i = (GLint) width;
   n[6].i = (GLint) height;
   n[7].i = border;
   n[8].i = imageSize;
   save_pointer(&n[9],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTextureImage2DEXT(ctx->Dispatch.Exec,
                  }
         void GLAPIENTRY
   save_CompressedTextureImage3DEXT(GLuint texture, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_3D) {
      /* don't compile, execute immediately */
   CALL_CompressedTextureImage3DEXT(ctx->Dispatch.Exec, (texture, target, level,
                  }
   else {
      Node *n;
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEXTURE_IMAGE_3D,
         if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].e = internalFormat;
   n[5].i = (GLint) width;
   n[6].i = (GLint) height;
   n[7].i = (GLint) depth;
   n[8].i = border;
   n[9].i = imageSize;
   save_pointer(&n[10],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTextureImage3DEXT(ctx->Dispatch.Exec,
                        }
         void GLAPIENTRY
   save_CompressedTextureSubImage1DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset,
               {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_1D,
         if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = (GLint) width;
   n[6].e = format;
   n[7].i = imageSize;
   save_pointer(&n[8],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTextureSubImage1DEXT(ctx->Dispatch.Exec, (texture, target, level, xoffset,
         }
         void GLAPIENTRY
   save_CompressedTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset,
                     {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_2D,
         if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = (GLint) width;
   n[7].i = (GLint) height;
   n[8].e = format;
   n[9].i = imageSize;
   save_pointer(&n[10],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTextureSubImage2DEXT(ctx->Dispatch.Exec,
               }
         void GLAPIENTRY
   save_CompressedTextureSubImage3DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset,
                     {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_3D,
         if (n) {
      n[1].ui = texture;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = zoffset;
   n[7].i = (GLint) width;
   n[8].i = (GLint) height;
   n[9].i = (GLint) depth;
   n[10].e = format;
   n[11].i = imageSize;
   save_pointer(&n[12],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTextureSubImage3DEXT(ctx->Dispatch.Exec,
                     }
         void GLAPIENTRY
   save_CompressedMultiTexImage1DEXT(GLenum texunit, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D) {
      /* don't compile, execute immediately */
   CALL_CompressedMultiTexImage1DEXT(ctx->Dispatch.Exec, (texunit, target, level,
                  }
   else {
      Node *n;
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_MULTITEX_IMAGE_1D,
         if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].e = internalFormat;
   n[5].i = (GLint) width;
   n[6].i = border;
   n[7].i = imageSize;
   save_pointer(&n[8],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedMultiTexImage1DEXT(ctx->Dispatch.Exec,
                  }
         void GLAPIENTRY
   save_CompressedMultiTexImage2DEXT(GLenum texunit, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_2D) {
      /* don't compile, execute immediately */
   CALL_CompressedMultiTexImage2DEXT(ctx->Dispatch.Exec, (texunit, target, level,
            }
   else {
      Node *n;
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_MULTITEX_IMAGE_2D,
         if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].e = internalFormat;
   n[5].i = (GLint) width;
   n[6].i = (GLint) height;
   n[7].i = border;
   n[8].i = imageSize;
   save_pointer(&n[9],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedMultiTexImage2DEXT(ctx->Dispatch.Exec,
                  }
         void GLAPIENTRY
   save_CompressedMultiTexImage3DEXT(GLenum texunit, GLenum target, GLint level,
                     {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_3D) {
      /* don't compile, execute immediately */
   CALL_CompressedMultiTexImage3DEXT(ctx->Dispatch.Exec, (texunit, target, level,
                  }
   else {
      Node *n;
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_MULTITEX_IMAGE_3D,
         if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].e = internalFormat;
   n[5].i = (GLint) width;
   n[6].i = (GLint) height;
   n[7].i = (GLint) depth;
   n[8].i = border;
   n[9].i = imageSize;
   save_pointer(&n[10],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedMultiTexImage3DEXT(ctx->Dispatch.Exec,
                        }
         void GLAPIENTRY
   save_CompressedMultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset,
               {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_1D,
         if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = (GLint) width;
   n[6].e = format;
   n[7].i = imageSize;
   save_pointer(&n[8],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedMultiTexSubImage1DEXT(ctx->Dispatch.Exec, (texunit, target, level, xoffset,
         }
         void GLAPIENTRY
   save_CompressedMultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset,
                     {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_2D,
         if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = (GLint) width;
   n[7].i = (GLint) height;
   n[8].e = format;
   n[9].i = imageSize;
   save_pointer(&n[10],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedMultiTexSubImage2DEXT(ctx->Dispatch.Exec,
               }
         void GLAPIENTRY
   save_CompressedMultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset,
                     {
      Node *n;
   GET_CURRENT_CONTEXT(ctx);
            n = alloc_instruction(ctx, OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_3D,
         if (n) {
      n[1].e = texunit;
   n[2].e = target;
   n[3].i = level;
   n[4].i = xoffset;
   n[5].i = yoffset;
   n[6].i = zoffset;
   n[7].i = (GLint) width;
   n[8].i = (GLint) height;
   n[9].i = (GLint) depth;
   n[10].e = format;
   n[11].i = imageSize;
   save_pointer(&n[12],
      }
   if (ctx->ExecuteFlag) {
      CALL_CompressedMultiTexSubImage3DEXT(ctx->Dispatch.Exec,
                     }
         void GLAPIENTRY
   save_NamedProgramStringEXT(GLuint program, GLenum target, GLenum format, GLsizei len,
         {
      GET_CURRENT_CONTEXT(ctx);
                     n = alloc_instruction(ctx, OPCODE_NAMED_PROGRAM_STRING, 4 + POINTER_DWORDS);
   if (n) {
      GLubyte *programCopy = malloc(len);
   if (!programCopy) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glNamedProgramStringEXT");
      }
   memcpy(programCopy, string, len);
   n[1].ui = program;
   n[2].e = target;
   n[3].e = format;
   n[4].i = len;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_NamedProgramLocalParameter4fEXT(GLuint program, GLenum target, GLuint index,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_NAMED_PROGRAM_LOCAL_PARAMETER, 7);
   if (n) {
      n[1].ui = program;
   n[2].e = target;
   n[3].ui = index;
   n[4].f = x;
   n[5].f = y;
   n[6].f = z;
      }
   if (ctx->ExecuteFlag) {
            }
         void GLAPIENTRY
   save_NamedProgramLocalParameter4fvEXT(GLuint program, GLenum target, GLuint index,
         {
      save_NamedProgramLocalParameter4fEXT(program, target, index, params[0],
      }
         void GLAPIENTRY
   save_NamedProgramLocalParameter4dEXT(GLuint program, GLenum target, GLuint index,
               {
         save_NamedProgramLocalParameter4fEXT(program, target, index, (GLfloat) x,
   }
         void GLAPIENTRY
   save_NamedProgramLocalParameter4dvEXT(GLuint program, GLenum target, GLuint index,
         {
      save_NamedProgramLocalParameter4fEXT(program, target, index, (GLfloat) params[0],
            }
      void GLAPIENTRY
   save_PrimitiveBoundingBox(float minX, float minY, float minZ, float minW,
         {
      GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PRIMITIVE_BOUNDING_BOX, 8);
   if (n) {
      n[1].f = minX;
   n[2].f = minY;
   n[3].f = minZ;
   n[4].f = minW;
   n[5].f = maxX;
   n[6].f = maxY;
   n[7].f = maxZ;
      }
   if (ctx->ExecuteFlag) {
      CALL_PrimitiveBoundingBox(ctx->Dispatch.Exec, (minX, minY, minZ, minW,
         }
      /**
   * Save an error-generating command into display list.
   *
   * KW: Will appear in the list before the vertex buffer containing the
   * command that provoked the error.  I don't see this as a problem.
   */
   static void
   save_error(struct gl_context *ctx, GLenum error, const char *s)
   {
      Node *n;
   n = alloc_instruction(ctx, OPCODE_ERROR, 1 + POINTER_DWORDS);
   if (n) {
      n[1].e = error;
   save_pointer(&n[2], (void *) s);
   /* note: the data/string here doesn't have to be freed in
   * _mesa_delete_list() since the string is never dynamically
   * allocated.
         }
         /**
   * Compile an error into current display list.
   */
   void
   _mesa_compile_error(struct gl_context *ctx, GLenum error, const char *s)
   {
      if (ctx->CompileFlag)
         if (ctx->ExecuteFlag)
      }
         /**
   * Test if ID names a display list.
   */
   bool
   _mesa_get_list(struct gl_context *ctx, GLuint list,
               {
      struct gl_display_list * dl =
            if (dlist)
               }
            /**********************************************************************/
   /*                     Display list execution                         */
   /**********************************************************************/
         /*
   * Execute a display list.  Note that the ListBase offset must have already
   * been added before calling this function.  I.e. the list argument is
   * the absolute list number, not relative to ListBase.
   * Must be called with ctx->Shared->DisplayList locked.
   * \param list - display list number
   */
   static void
   execute_list(struct gl_context *ctx, GLuint list)
   {
      struct gl_display_list *dlist;
            if (list == 0 || !_mesa_get_list(ctx, list, &dlist, true))
                     while (1) {
               switch (opcode) {
      case OPCODE_ERROR:
      _mesa_error(ctx, n[1].e, "%s", (const char *) get_pointer(&n[2]));
      case OPCODE_ACCUM:
      CALL_Accum(ctx->Dispatch.Exec, (n[1].e, n[2].f));
      case OPCODE_ALPHA_FUNC:
      CALL_AlphaFunc(ctx->Dispatch.Exec, (n[1].e, n[2].f));
      case OPCODE_BIND_TEXTURE:
      CALL_BindTexture(ctx->Dispatch.Exec, (n[1].e, n[2].ui));
      case OPCODE_BITMAP:
      if (_mesa_inside_begin_end(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
      } else {
      _mesa_bitmap(ctx, n[1].i, n[2].i, n[3].f, n[4].f, n[5].f,
      }
      case OPCODE_BLEND_COLOR:
      CALL_BlendColor(ctx->Dispatch.Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
      case OPCODE_BLEND_EQUATION:
      CALL_BlendEquation(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_BLEND_EQUATION_SEPARATE:
      CALL_BlendEquationSeparate(ctx->Dispatch.Exec, (n[1].e, n[2].e));
      case OPCODE_BLEND_FUNC_SEPARATE:
      CALL_BlendFuncSeparate(ctx->Dispatch.Exec,
               case OPCODE_BLEND_FUNC_I:
      /* GL_ARB_draw_buffers_blend */
   CALL_BlendFunciARB(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].e));
      case OPCODE_BLEND_FUNC_SEPARATE_I:
      /* GL_ARB_draw_buffers_blend */
   CALL_BlendFuncSeparateiARB(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].e,
            case OPCODE_BLEND_EQUATION_I:
      /* GL_ARB_draw_buffers_blend */
   CALL_BlendEquationiARB(ctx->Dispatch.Exec, (n[1].ui, n[2].e));
      case OPCODE_BLEND_EQUATION_SEPARATE_I:
      /* GL_ARB_draw_buffers_blend */
   CALL_BlendEquationSeparateiARB(ctx->Dispatch.Exec,
               case OPCODE_CALL_LIST:
      /* Generated by glCallList(), don't add ListBase */
   if (ctx->ListState.CallDepth < MAX_LIST_NESTING) {
      ctx->ListState.CallDepth++;
   execute_list(ctx, n[1].ui);
      }
      case OPCODE_CALL_LISTS:
      if (ctx->ListState.CallDepth < MAX_LIST_NESTING) {
      ctx->ListState.CallDepth++;
   _mesa_HashUnlockMutex(ctx->Shared->DisplayList);
   CALL_CallLists(ctx->Dispatch.Exec, (n[1].i, n[2].e, get_pointer(&n[3])));
   _mesa_HashLockMutex(ctx->Shared->DisplayList);
      }
      case OPCODE_CLEAR:
      CALL_Clear(ctx->Dispatch.Exec, (n[1].bf));
      case OPCODE_CLEAR_BUFFER_IV:
      {
      GLint value[4];
   value[0] = n[3].i;
   value[1] = n[4].i;
   value[2] = n[5].i;
   value[3] = n[6].i;
      }
      case OPCODE_CLEAR_BUFFER_UIV:
      {
      GLuint value[4];
   value[0] = n[3].ui;
   value[1] = n[4].ui;
   value[2] = n[5].ui;
   value[3] = n[6].ui;
      }
      case OPCODE_CLEAR_BUFFER_FV:
      {
      GLfloat value[4];
   value[0] = n[3].f;
   value[1] = n[4].f;
   value[2] = n[5].f;
   value[3] = n[6].f;
      }
      case OPCODE_CLEAR_BUFFER_FI:
      CALL_ClearBufferfi(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].f, n[4].i));
      case OPCODE_CLEAR_COLOR:
      CALL_ClearColor(ctx->Dispatch.Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
      case OPCODE_CLEAR_ACCUM:
      CALL_ClearAccum(ctx->Dispatch.Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
      case OPCODE_CLEAR_DEPTH:
      CALL_ClearDepth(ctx->Dispatch.Exec, ((GLclampd) n[1].f));
      case OPCODE_CLEAR_INDEX:
      CALL_ClearIndex(ctx->Dispatch.Exec, ((GLfloat) n[1].ui));
      case OPCODE_CLEAR_STENCIL:
      CALL_ClearStencil(ctx->Dispatch.Exec, (n[1].i));
      case OPCODE_CLIP_PLANE:
      {
      GLdouble eq[4];
   eq[0] = n[2].f;
   eq[1] = n[3].f;
   eq[2] = n[4].f;
   eq[3] = n[5].f;
      }
      case OPCODE_COLOR_MASK:
      CALL_ColorMask(ctx->Dispatch.Exec, (n[1].b, n[2].b, n[3].b, n[4].b));
      case OPCODE_COLOR_MASK_INDEXED:
      CALL_ColorMaski(ctx->Dispatch.Exec, (n[1].ui, n[2].b, n[3].b,
            case OPCODE_COLOR_MATERIAL:
      CALL_ColorMaterial(ctx->Dispatch.Exec, (n[1].e, n[2].e));
      case OPCODE_COPY_PIXELS:
      CALL_CopyPixels(ctx->Dispatch.Exec, (n[1].i, n[2].i,
                  case OPCODE_COPY_TEX_IMAGE1D:
      CALL_CopyTexImage1D(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].e, n[4].i,
            case OPCODE_COPY_TEX_IMAGE2D:
      CALL_CopyTexImage2D(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].e, n[4].i,
            case OPCODE_COPY_TEX_SUB_IMAGE1D:
      CALL_CopyTexSubImage1D(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].i,
            case OPCODE_COPY_TEX_SUB_IMAGE2D:
      CALL_CopyTexSubImage2D(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].i,
                  case OPCODE_COPY_TEX_SUB_IMAGE3D:
      CALL_CopyTexSubImage3D(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].i,
                  case OPCODE_CULL_FACE:
      CALL_CullFace(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_DEPTH_FUNC:
      CALL_DepthFunc(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_DEPTH_MASK:
      CALL_DepthMask(ctx->Dispatch.Exec, (n[1].b));
      case OPCODE_DEPTH_RANGE:
      CALL_DepthRange(ctx->Dispatch.Exec,
            case OPCODE_DISABLE:
      CALL_Disable(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_DISABLE_INDEXED:
      CALL_Disablei(ctx->Dispatch.Exec, (n[1].ui, n[2].e));
      case OPCODE_DRAW_BUFFER:
      CALL_DrawBuffer(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_DRAW_PIXELS:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_DrawPixels(ctx->Dispatch.Exec, (n[1].i, n[2].i, n[3].e, n[4].e,
            }
      case OPCODE_ENABLE:
      CALL_Enable(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_ENABLE_INDEXED:
      CALL_Enablei(ctx->Dispatch.Exec, (n[1].ui, n[2].e));
      case OPCODE_EVALMESH1:
      CALL_EvalMesh1(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].i));
      case OPCODE_EVALMESH2:
      CALL_EvalMesh2(ctx->Dispatch.Exec,
            case OPCODE_FOG:
      {
      GLfloat p[4];
   p[0] = n[2].f;
   p[1] = n[3].f;
   p[2] = n[4].f;
   p[3] = n[5].f;
      }
      case OPCODE_FRONT_FACE:
      CALL_FrontFace(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_FRUSTUM:
      CALL_Frustum(ctx->Dispatch.Exec,
            case OPCODE_HINT:
      CALL_Hint(ctx->Dispatch.Exec, (n[1].e, n[2].e));
      case OPCODE_INDEX_MASK:
      CALL_IndexMask(ctx->Dispatch.Exec, (n[1].ui));
      case OPCODE_INIT_NAMES:
      CALL_InitNames(ctx->Dispatch.Exec, ());
      case OPCODE_LIGHT:
      {
      GLfloat p[4];
   p[0] = n[3].f;
   p[1] = n[4].f;
   p[2] = n[5].f;
   p[3] = n[6].f;
      }
      case OPCODE_LIGHT_MODEL:
      {
      GLfloat p[4];
   p[0] = n[2].f;
   p[1] = n[3].f;
   p[2] = n[4].f;
   p[3] = n[5].f;
      }
      case OPCODE_LINE_STIPPLE:
      CALL_LineStipple(ctx->Dispatch.Exec, (n[1].i, n[2].us));
      case OPCODE_LINE_WIDTH:
      CALL_LineWidth(ctx->Dispatch.Exec, (n[1].f));
      case OPCODE_LIST_BASE:
      CALL_ListBase(ctx->Dispatch.Exec, (n[1].ui));
      case OPCODE_LOAD_IDENTITY:
      CALL_LoadIdentity(ctx->Dispatch.Exec, ());
      case OPCODE_LOAD_MATRIX:
      STATIC_ASSERT(sizeof(Node) == sizeof(GLfloat));
   CALL_LoadMatrixf(ctx->Dispatch.Exec, (&n[1].f));
      case OPCODE_LOAD_NAME:
      CALL_LoadName(ctx->Dispatch.Exec, (n[1].ui));
      case OPCODE_LOGIC_OP:
      CALL_LogicOp(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_MAP1:
      {
      GLenum target = n[1].e;
   GLint ustride = _mesa_evaluator_components(target);
   GLint uorder = n[5].i;
   GLfloat u1 = n[2].f;
   GLfloat u2 = n[3].f;
   CALL_Map1f(ctx->Dispatch.Exec, (target, u1, u2, ustride, uorder,
      }
      case OPCODE_MAP2:
      {
      GLenum target = n[1].e;
   GLfloat u1 = n[2].f;
   GLfloat u2 = n[3].f;
   GLfloat v1 = n[4].f;
   GLfloat v2 = n[5].f;
   GLint ustride = n[6].i;
   GLint vstride = n[7].i;
   GLint uorder = n[8].i;
   GLint vorder = n[9].i;
   CALL_Map2f(ctx->Dispatch.Exec, (target, u1, u2, ustride, uorder,
            }
      case OPCODE_MAPGRID1:
      CALL_MapGrid1f(ctx->Dispatch.Exec, (n[1].i, n[2].f, n[3].f));
      case OPCODE_MAPGRID2:
      CALL_MapGrid2f(ctx->Dispatch.Exec,
            case OPCODE_MATRIX_MODE:
      CALL_MatrixMode(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_MULT_MATRIX:
      CALL_MultMatrixf(ctx->Dispatch.Exec, (&n[1].f));
      case OPCODE_ORTHO:
      CALL_Ortho(ctx->Dispatch.Exec,
            case OPCODE_PASSTHROUGH:
      CALL_PassThrough(ctx->Dispatch.Exec, (n[1].f));
      case OPCODE_PATCH_PARAMETER_I:
      CALL_PatchParameteri(ctx->Dispatch.Exec, (n[1].e, n[2].i));
      case OPCODE_PATCH_PARAMETER_FV_INNER:
      {
      GLfloat params[2];
   params[0] = n[2].f;
   params[1] = n[3].f;
      }
      case OPCODE_PATCH_PARAMETER_FV_OUTER:
      {
      GLfloat params[4];
   params[0] = n[2].f;
   params[1] = n[3].f;
   params[2] = n[4].f;
   params[3] = n[5].f;
      }
      case OPCODE_PIXEL_MAP:
      CALL_PixelMapfv(ctx->Dispatch.Exec,
            case OPCODE_PIXEL_TRANSFER:
      CALL_PixelTransferf(ctx->Dispatch.Exec, (n[1].e, n[2].f));
      case OPCODE_PIXEL_ZOOM:
      CALL_PixelZoom(ctx->Dispatch.Exec, (n[1].f, n[2].f));
      case OPCODE_POINT_SIZE:
      CALL_PointSize(ctx->Dispatch.Exec, (n[1].f));
      case OPCODE_POINT_PARAMETERS:
      {
      GLfloat params[3];
   params[0] = n[2].f;
   params[1] = n[3].f;
   params[2] = n[4].f;
      }
      case OPCODE_POLYGON_MODE:
      CALL_PolygonMode(ctx->Dispatch.Exec, (n[1].e, n[2].e));
      case OPCODE_POLYGON_STIPPLE:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_PolygonStipple(ctx->Dispatch.Exec, (get_pointer(&n[1])));
      }
      case OPCODE_POLYGON_OFFSET:
      CALL_PolygonOffset(ctx->Dispatch.Exec, (n[1].f, n[2].f));
      case OPCODE_POLYGON_OFFSET_CLAMP:
      CALL_PolygonOffsetClampEXT(ctx->Dispatch.Exec, (n[1].f, n[2].f, n[3].f));
      case OPCODE_POP_ATTRIB:
      CALL_PopAttrib(ctx->Dispatch.Exec, ());
      case OPCODE_POP_MATRIX:
      CALL_PopMatrix(ctx->Dispatch.Exec, ());
      case OPCODE_POP_NAME:
      CALL_PopName(ctx->Dispatch.Exec, ());
      case OPCODE_PRIORITIZE_TEXTURE:
      CALL_PrioritizeTextures(ctx->Dispatch.Exec, (1, &n[1].ui, &n[2].f));
      case OPCODE_PUSH_ATTRIB:
      CALL_PushAttrib(ctx->Dispatch.Exec, (n[1].bf));
      case OPCODE_PUSH_MATRIX:
      CALL_PushMatrix(ctx->Dispatch.Exec, ());
      case OPCODE_PUSH_NAME:
      CALL_PushName(ctx->Dispatch.Exec, (n[1].ui));
      case OPCODE_RASTER_POS:
      CALL_RasterPos4f(ctx->Dispatch.Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
      case OPCODE_READ_BUFFER:
      CALL_ReadBuffer(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_ROTATE:
      CALL_Rotatef(ctx->Dispatch.Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
      case OPCODE_SCALE:
      CALL_Scalef(ctx->Dispatch.Exec, (n[1].f, n[2].f, n[3].f));
      case OPCODE_SCISSOR:
      CALL_Scissor(ctx->Dispatch.Exec, (n[1].i, n[2].i, n[3].i, n[4].i));
      case OPCODE_SHADE_MODEL:
      CALL_ShadeModel(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_PROVOKING_VERTEX:
      CALL_ProvokingVertex(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_STENCIL_FUNC:
      CALL_StencilFunc(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].ui));
      case OPCODE_STENCIL_MASK:
      CALL_StencilMask(ctx->Dispatch.Exec, (n[1].ui));
      case OPCODE_STENCIL_OP:
      CALL_StencilOp(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].e));
      case OPCODE_STENCIL_FUNC_SEPARATE:
      CALL_StencilFuncSeparate(ctx->Dispatch.Exec,
            case OPCODE_STENCIL_MASK_SEPARATE:
      CALL_StencilMaskSeparate(ctx->Dispatch.Exec, (n[1].e, n[2].ui));
      case OPCODE_STENCIL_OP_SEPARATE:
      CALL_StencilOpSeparate(ctx->Dispatch.Exec,
            case OPCODE_TEXENV:
      {
      GLfloat params[4];
   params[0] = n[3].f;
   params[1] = n[4].f;
   params[2] = n[5].f;
   params[3] = n[6].f;
      }
      case OPCODE_TEXGEN:
      {
      GLfloat params[4];
   params[0] = n[3].f;
   params[1] = n[4].f;
   params[2] = n[5].f;
   params[3] = n[6].f;
      }
      case OPCODE_TEXPARAMETER:
      {
      GLfloat params[4];
   params[0] = n[3].f;
   params[1] = n[4].f;
   params[2] = n[5].f;
   params[3] = n[6].f;
      }
      case OPCODE_TEX_IMAGE1D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TexImage1D(ctx->Dispatch.Exec, (n[1].e,      /* target */
                              n[2].i,      /* level */
   n[3].i,      /* components */
         }
      case OPCODE_TEX_IMAGE2D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TexImage2D(ctx->Dispatch.Exec, (n[1].e,      /* target */
                              n[2].i,      /* level */
   n[3].i,      /* components */
   n[4].i,      /* width */
         }
      case OPCODE_TEX_IMAGE3D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TexImage3D(ctx->Dispatch.Exec, (n[1].e,      /* target */
                              n[2].i,      /* level */
   n[3].i,      /* components */
   n[4].i,      /* width */
   n[5].i,      /* height */
         }
      case OPCODE_TEX_SUB_IMAGE1D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TexSubImage1D(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].i,
                  }
      case OPCODE_TEX_SUB_IMAGE2D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TexSubImage2D(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].i,
                        }
      case OPCODE_TEX_SUB_IMAGE3D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TexSubImage3D(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].i,
                        }
      case OPCODE_TRANSLATE:
      CALL_Translatef(ctx->Dispatch.Exec, (n[1].f, n[2].f, n[3].f));
      case OPCODE_VIEWPORT:
      CALL_Viewport(ctx->Dispatch.Exec, (n[1].i, n[2].i,
            case OPCODE_WINDOW_POS:
      CALL_WindowPos4fMESA(ctx->Dispatch.Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
      case OPCODE_VIEWPORT_ARRAY_V:
      CALL_ViewportArrayv(ctx->Dispatch.Exec, (n[1].ui, n[2].si,
            case OPCODE_VIEWPORT_INDEXED_F:
      CALL_ViewportIndexedf(ctx->Dispatch.Exec, (n[1].ui, n[2].f, n[3].f, n[4].f,
            case OPCODE_VIEWPORT_INDEXED_FV: {
      GLfloat v[4];
   v[0] = n[2].f;
   v[1] = n[3].f;
   v[2] = n[4].f;
   v[3] = n[5].f;
   CALL_ViewportIndexedfv(ctx->Dispatch.Exec, (n[1].ui, v));
      }
   case OPCODE_SCISSOR_ARRAY_V:
      CALL_ScissorArrayv(ctx->Dispatch.Exec, (n[1].ui, n[2].si,
            case OPCODE_SCISSOR_INDEXED:
      CALL_ScissorIndexed(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i, n[4].si,
            case OPCODE_SCISSOR_INDEXED_V: {
      GLint v[4];
   v[0] = n[2].i;
   v[1] = n[3].i;
   v[2] = n[4].si;
   v[3] = n[5].si;
   CALL_ScissorIndexedv(ctx->Dispatch.Exec, (n[1].ui, v));
      }
   case OPCODE_DEPTH_ARRAY_V:
      CALL_DepthRangeArrayv(ctx->Dispatch.Exec, (n[1].ui, n[2].si,
            case OPCODE_DEPTH_INDEXED:
      CALL_DepthRangeIndexed(ctx->Dispatch.Exec, (n[1].ui, n[2].f, n[3].f));
      case OPCODE_ACTIVE_TEXTURE:   /* GL_ARB_multitexture */
      CALL_ActiveTexture(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_COMPRESSED_TEX_IMAGE_1D:  /* GL_ARB_texture_compression */
      CALL_CompressedTexImage1D(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].e,
                  case OPCODE_COMPRESSED_TEX_IMAGE_2D:  /* GL_ARB_texture_compression */
      CALL_CompressedTexImage2D(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].e,
                  case OPCODE_COMPRESSED_TEX_IMAGE_3D:  /* GL_ARB_texture_compression */
      CALL_CompressedTexImage3D(ctx->Dispatch.Exec, (n[1].e, n[2].i, n[3].e,
                        case OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D:      /* GL_ARB_texture_compress */
      CALL_CompressedTexSubImage1D(ctx->Dispatch.Exec,
                        case OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D:      /* GL_ARB_texture_compress */
      CALL_CompressedTexSubImage2D(ctx->Dispatch.Exec,
                        case OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D:      /* GL_ARB_texture_compress */
      CALL_CompressedTexSubImage3D(ctx->Dispatch.Exec,
                              case OPCODE_SAMPLE_COVERAGE:  /* GL_ARB_multisample */
      CALL_SampleCoverage(ctx->Dispatch.Exec, (n[1].f, n[2].b));
      case OPCODE_WINDOW_POS_ARB:   /* GL_ARB_window_pos */
      CALL_WindowPos3f(ctx->Dispatch.Exec, (n[1].f, n[2].f, n[3].f));
      case OPCODE_BIND_PROGRAM_ARB:  /* GL_ARB_vertex_program */
      CALL_BindProgramARB(ctx->Dispatch.Exec, (n[1].e, n[2].ui));
      case OPCODE_PROGRAM_LOCAL_PARAMETER_ARB:
      CALL_ProgramLocalParameter4fARB(ctx->Dispatch.Exec,
                  case OPCODE_ACTIVE_STENCIL_FACE_EXT:
      CALL_ActiveStencilFaceEXT(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_DEPTH_BOUNDS_EXT:
      CALL_DepthBoundsEXT(ctx->Dispatch.Exec, (n[1].f, n[2].f));
      case OPCODE_PROGRAM_STRING_ARB:
      CALL_ProgramStringARB(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_ENV_PARAMETER_ARB:
      CALL_ProgramEnvParameter4fARB(ctx->Dispatch.Exec, (n[1].e, n[2].ui, n[3].f,
                  case OPCODE_BEGIN_QUERY_ARB:
      CALL_BeginQuery(ctx->Dispatch.Exec, (n[1].e, n[2].ui));
      case OPCODE_END_QUERY_ARB:
      CALL_EndQuery(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_QUERY_COUNTER:
      CALL_QueryCounter(ctx->Dispatch.Exec, (n[1].ui, n[2].e));
      case OPCODE_BEGIN_QUERY_INDEXED:
      CALL_BeginQueryIndexed(ctx->Dispatch.Exec, (n[1].e, n[2].ui, n[3].ui));
      case OPCODE_END_QUERY_INDEXED:
      CALL_EndQueryIndexed(ctx->Dispatch.Exec, (n[1].e, n[2].ui));
      case OPCODE_DRAW_BUFFERS_ARB:
      {
      GLenum buffers[MAX_DRAW_BUFFERS];
   GLint i, count = MIN2(n[1].i, MAX_DRAW_BUFFERS);
   for (i = 0; i < count; i++)
            }
      case OPCODE_BLIT_FRAMEBUFFER:
      CALL_BlitFramebuffer(ctx->Dispatch.Exec, (n[1].i, n[2].i, n[3].i, n[4].i,
                  case OPCODE_PRIMITIVE_RESTART_NV:
                  case OPCODE_USE_PROGRAM:
      CALL_UseProgram(ctx->Dispatch.Exec, (n[1].ui));
      case OPCODE_UNIFORM_1F:
      CALL_Uniform1f(ctx->Dispatch.Exec, (n[1].i, n[2].f));
      case OPCODE_UNIFORM_2F:
      CALL_Uniform2f(ctx->Dispatch.Exec, (n[1].i, n[2].f, n[3].f));
      case OPCODE_UNIFORM_3F:
      CALL_Uniform3f(ctx->Dispatch.Exec, (n[1].i, n[2].f, n[3].f, n[4].f));
      case OPCODE_UNIFORM_4F:
      CALL_Uniform4f(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_1FV:
      CALL_Uniform1fv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_2FV:
      CALL_Uniform2fv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_3FV:
      CALL_Uniform3fv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_4FV:
      CALL_Uniform4fv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
                                       CALL_Uniform1d(ctx->Dispatch.Exec, (n[1].i, x.d));
      }
   case OPCODE_UNIFORM_2D: {
                     x.uint32[0] = n[2].ui;
   x.uint32[1] = n[3].ui;
                  CALL_Uniform2d(ctx->Dispatch.Exec, (n[1].i, x.d, y.d));
      }
   case OPCODE_UNIFORM_3D: {
      union float64_pair x;
                  x.uint32[0] = n[2].ui;
   x.uint32[1] = n[3].ui;
   y.uint32[0] = n[4].ui;
   y.uint32[1] = n[5].ui;
                  CALL_Uniform3d(ctx->Dispatch.Exec, (n[1].i, x.d, y.d, z.d));
      }
   case OPCODE_UNIFORM_4D: {
      union float64_pair x;
   union float64_pair y;
                  x.uint32[0] = n[2].ui;
   x.uint32[1] = n[3].ui;
   y.uint32[0] = n[4].ui;
   y.uint32[1] = n[5].ui;
   z.uint32[0] = n[6].ui;
   z.uint32[1] = n[7].ui;
                  CALL_Uniform4d(ctx->Dispatch.Exec, (n[1].i, x.d, y.d, z.d, w.d));
      }
   case OPCODE_UNIFORM_1DV:
      CALL_Uniform1dv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_2DV:
      CALL_Uniform2dv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_3DV:
      CALL_Uniform3dv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_4DV:
      CALL_Uniform4dv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_1I:
      CALL_Uniform1i(ctx->Dispatch.Exec, (n[1].i, n[2].i));
      case OPCODE_UNIFORM_2I:
      CALL_Uniform2i(ctx->Dispatch.Exec, (n[1].i, n[2].i, n[3].i));
      case OPCODE_UNIFORM_3I:
      CALL_Uniform3i(ctx->Dispatch.Exec, (n[1].i, n[2].i, n[3].i, n[4].i));
      case OPCODE_UNIFORM_4I:
      CALL_Uniform4i(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_1IV:
      CALL_Uniform1iv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_2IV:
      CALL_Uniform2iv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_3IV:
      CALL_Uniform3iv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_4IV:
      CALL_Uniform4iv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_1UI:
      CALL_Uniform1ui(ctx->Dispatch.Exec, (n[1].i, n[2].i));
      case OPCODE_UNIFORM_2UI:
      CALL_Uniform2ui(ctx->Dispatch.Exec, (n[1].i, n[2].i, n[3].i));
      case OPCODE_UNIFORM_3UI:
      CALL_Uniform3ui(ctx->Dispatch.Exec, (n[1].i, n[2].i, n[3].i, n[4].i));
      case OPCODE_UNIFORM_4UI:
      CALL_Uniform4ui(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_1UIV:
      CALL_Uniform1uiv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_2UIV:
      CALL_Uniform2uiv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_3UIV:
      CALL_Uniform3uiv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_4UIV:
      CALL_Uniform4uiv(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_MATRIX22:
      CALL_UniformMatrix2fv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX33:
      CALL_UniformMatrix3fv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX44:
      CALL_UniformMatrix4fv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX23:
      CALL_UniformMatrix2x3fv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX32:
      CALL_UniformMatrix3x2fv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX24:
      CALL_UniformMatrix2x4fv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX42:
      CALL_UniformMatrix4x2fv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX34:
      CALL_UniformMatrix3x4fv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX43:
      CALL_UniformMatrix4x3fv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX22D:
      CALL_UniformMatrix2dv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX33D:
      CALL_UniformMatrix3dv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX44D:
      CALL_UniformMatrix4dv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX23D:
      CALL_UniformMatrix2x3dv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX32D:
      CALL_UniformMatrix3x2dv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX24D:
      CALL_UniformMatrix2x4dv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX42D:
      CALL_UniformMatrix4x2dv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX34D:
      CALL_UniformMatrix3x4dv(ctx->Dispatch.Exec,
            case OPCODE_UNIFORM_MATRIX43D:
      CALL_UniformMatrix4x3dv(ctx->Dispatch.Exec,
                                                CALL_Uniform1i64ARB(ctx->Dispatch.Exec, (n[1].i, x.int64));
      }
   case OPCODE_UNIFORM_2I64: {
                     x.int32[0] = n[2].i;
   x.int32[1] = n[3].i;
                  CALL_Uniform2i64ARB(ctx->Dispatch.Exec, (n[1].i, x.int64, y.int64));
      }
   case OPCODE_UNIFORM_3I64: {
      union int64_pair x;
                  x.int32[0] = n[2].i;
   x.int32[1] = n[3].i;
   y.int32[0] = n[4].i;
   y.int32[1] = n[5].i;
                     CALL_Uniform3i64ARB(ctx->Dispatch.Exec, (n[1].i, x.int64, y.int64, z.int64));
      }
   case OPCODE_UNIFORM_4I64: {
      union int64_pair x;
   union int64_pair y;
                  x.int32[0] = n[2].i;
   x.int32[1] = n[3].i;
   y.int32[0] = n[4].i;
   y.int32[1] = n[5].i;
   z.int32[0] = n[6].i;
   z.int32[1] = n[7].i;
                  CALL_Uniform4i64ARB(ctx->Dispatch.Exec, (n[1].i, x.int64, y.int64, z.int64, w.int64));
      }
   case OPCODE_UNIFORM_1I64V:
      CALL_Uniform1i64vARB(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_2I64V:
      CALL_Uniform2i64vARB(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_3I64V:
      CALL_Uniform3i64vARB(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
      case OPCODE_UNIFORM_4I64V:
      CALL_Uniform4i64vARB(ctx->Dispatch.Exec, (n[1].i, n[2].i, get_pointer(&n[3])));
                                       CALL_Uniform1ui64ARB(ctx->Dispatch.Exec, (n[1].i, x.uint64));
      }
   case OPCODE_UNIFORM_2UI64: {
                     x.uint32[0] = n[2].ui;
   x.uint32[1] = n[3].ui;
                  CALL_Uniform2ui64ARB(ctx->Dispatch.Exec, (n[1].i, x.uint64, y.uint64));
      }
   case OPCODE_UNIFORM_3UI64: {
      union uint64_pair x;
                  x.uint32[0] = n[2].ui;
   x.uint32[1] = n[3].ui;
   y.uint32[0] = n[4].ui;
   y.uint32[1] = n[5].ui;
                     CALL_Uniform3ui64ARB(ctx->Dispatch.Exec, (n[1].i, x.uint64, y.uint64,
            }
   case OPCODE_UNIFORM_4UI64: {
      union uint64_pair x;
   union uint64_pair y;
                  x.uint32[0] = n[2].ui;
   x.uint32[1] = n[3].ui;
   y.uint32[0] = n[4].ui;
   y.uint32[1] = n[5].ui;
   z.uint32[0] = n[6].ui;
   z.uint32[1] = n[7].ui;
                  CALL_Uniform4ui64ARB(ctx->Dispatch.Exec, (n[1].i, x.uint64, y.uint64,
            }
   case OPCODE_UNIFORM_1UI64V:
      CALL_Uniform1ui64vARB(ctx->Dispatch.Exec, (n[1].i, n[2].i,
            case OPCODE_UNIFORM_2UI64V:
      CALL_Uniform2ui64vARB(ctx->Dispatch.Exec, (n[1].i, n[2].i,
            case OPCODE_UNIFORM_3UI64V:
      CALL_Uniform3ui64vARB(ctx->Dispatch.Exec, (n[1].i, n[2].i,
            case OPCODE_UNIFORM_4UI64V:
      CALL_Uniform4ui64vARB(ctx->Dispatch.Exec, (n[1].i, n[2].i,
                                                CALL_ProgramUniform1i64ARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, x.int64));
      }
   case OPCODE_PROGRAM_UNIFORM_2I64: {
                     x.int32[0] = n[3].i;
   x.int32[1] = n[4].i;
                  CALL_ProgramUniform2i64ARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, x.int64,
            }
   case OPCODE_PROGRAM_UNIFORM_3I64: {
      union int64_pair x;
                  x.int32[0] = n[3].i;
   x.int32[1] = n[4].i;
   y.int32[0] = n[5].i;
   y.int32[1] = n[6].i;
                  CALL_ProgramUniform3i64ARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, x.int64,
            }
   case OPCODE_PROGRAM_UNIFORM_4I64: {
      union int64_pair x;
   union int64_pair y;
                  x.int32[0] = n[3].i;
   x.int32[1] = n[4].i;
   y.int32[0] = n[5].i;
   y.int32[1] = n[6].i;
   z.int32[0] = n[7].i;
   z.int32[1] = n[8].i;
                  CALL_ProgramUniform4i64ARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, x.int64,
            }
   case OPCODE_PROGRAM_UNIFORM_1I64V:
      CALL_ProgramUniform1i64vARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_2I64V:
      CALL_ProgramUniform2i64vARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_3I64V:
      CALL_ProgramUniform3i64vARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_4I64V:
      CALL_ProgramUniform4i64vARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
                                             CALL_ProgramUniform1i64ARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, x.uint64));
      }
   case OPCODE_PROGRAM_UNIFORM_2UI64: {
                     x.uint32[0] = n[3].ui;
   x.uint32[1] = n[4].ui;
                  CALL_ProgramUniform2ui64ARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, x.uint64,
            }
   case OPCODE_PROGRAM_UNIFORM_3UI64: {
      union uint64_pair x;
                  x.uint32[0] = n[3].ui;
   x.uint32[1] = n[4].ui;
   y.uint32[0] = n[5].ui;
   y.uint32[1] = n[6].ui;
                  CALL_ProgramUniform3ui64ARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, x.uint64,
            }
   case OPCODE_PROGRAM_UNIFORM_4UI64: {
      union uint64_pair x;
   union uint64_pair y;
                  x.uint32[0] = n[3].ui;
   x.uint32[1] = n[4].ui;
   y.uint32[0] = n[5].ui;
   y.uint32[1] = n[6].ui;
   z.uint32[0] = n[7].ui;
   z.uint32[1] = n[8].ui;
                  CALL_ProgramUniform4ui64ARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, x.uint64,
            }
   case OPCODE_PROGRAM_UNIFORM_1UI64V:
      CALL_ProgramUniform1ui64vARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_2UI64V:
      CALL_ProgramUniform2ui64vARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_3UI64V:
      CALL_ProgramUniform3ui64vARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_4UI64V:
      CALL_ProgramUniform4ui64vARB(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
               case OPCODE_USE_PROGRAM_STAGES:
      CALL_UseProgramStages(ctx->Dispatch.Exec, (n[1].ui, n[2].ui, n[3].ui));
      case OPCODE_PROGRAM_UNIFORM_1F:
      CALL_ProgramUniform1f(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].f));
      case OPCODE_PROGRAM_UNIFORM_2F:
      CALL_ProgramUniform2f(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].f, n[4].f));
      case OPCODE_PROGRAM_UNIFORM_3F:
      CALL_ProgramUniform3f(ctx->Dispatch.Exec, (n[1].ui, n[2].i,
            case OPCODE_PROGRAM_UNIFORM_4F:
      CALL_ProgramUniform4f(ctx->Dispatch.Exec, (n[1].ui, n[2].i,
            case OPCODE_PROGRAM_UNIFORM_1FV:
      CALL_ProgramUniform1fv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_2FV:
      CALL_ProgramUniform2fv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_3FV:
      CALL_ProgramUniform3fv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_4FV:
      CALL_ProgramUniform4fv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
                                             CALL_ProgramUniform1d(ctx->Dispatch.Exec, (n[1].ui, n[2].i, x.d));
      }
   case OPCODE_PROGRAM_UNIFORM_2D: {
                     x.uint32[0] = n[3].ui;
   x.uint32[1] = n[4].ui;
                  CALL_ProgramUniform2d(ctx->Dispatch.Exec, (n[1].ui, n[2].i, x.d, y.d));
      }
   case OPCODE_PROGRAM_UNIFORM_3D: {
      union float64_pair x;
                  x.uint32[0] = n[3].ui;
   x.uint32[1] = n[4].ui;
   y.uint32[0] = n[5].ui;
   y.uint32[1] = n[6].ui;
                  CALL_ProgramUniform3d(ctx->Dispatch.Exec, (n[1].ui, n[2].i,
            }
   case OPCODE_PROGRAM_UNIFORM_4D: {
      union float64_pair x;
   union float64_pair y;
                  x.uint32[0] = n[3].ui;
   x.uint32[1] = n[4].ui;
   y.uint32[0] = n[5].ui;
   y.uint32[1] = n[6].ui;
   z.uint32[0] = n[7].ui;
   z.uint32[1] = n[8].ui;
                  CALL_ProgramUniform4d(ctx->Dispatch.Exec, (n[1].ui, n[2].i,
            }
   case OPCODE_PROGRAM_UNIFORM_1DV:
      CALL_ProgramUniform1dv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_2DV:
      CALL_ProgramUniform2dv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_3DV:
      CALL_ProgramUniform3dv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_4DV:
      CALL_ProgramUniform4dv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_1I:
      CALL_ProgramUniform1i(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i));
      case OPCODE_PROGRAM_UNIFORM_2I:
      CALL_ProgramUniform2i(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i, n[4].i));
      case OPCODE_PROGRAM_UNIFORM_3I:
      CALL_ProgramUniform3i(ctx->Dispatch.Exec, (n[1].ui, n[2].i,
            case OPCODE_PROGRAM_UNIFORM_4I:
      CALL_ProgramUniform4i(ctx->Dispatch.Exec, (n[1].ui, n[2].i,
            case OPCODE_PROGRAM_UNIFORM_1IV:
      CALL_ProgramUniform1iv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_2IV:
      CALL_ProgramUniform2iv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_3IV:
      CALL_ProgramUniform3iv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_4IV:
      CALL_ProgramUniform4iv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_1UI:
      CALL_ProgramUniform1ui(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].ui));
      case OPCODE_PROGRAM_UNIFORM_2UI:
      CALL_ProgramUniform2ui(ctx->Dispatch.Exec, (n[1].ui, n[2].i,
            case OPCODE_PROGRAM_UNIFORM_3UI:
      CALL_ProgramUniform3ui(ctx->Dispatch.Exec, (n[1].ui, n[2].i,
            case OPCODE_PROGRAM_UNIFORM_4UI:
      CALL_ProgramUniform4ui(ctx->Dispatch.Exec, (n[1].ui, n[2].i,
                  case OPCODE_PROGRAM_UNIFORM_1UIV:
      CALL_ProgramUniform1uiv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_2UIV:
      CALL_ProgramUniform2uiv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_3UIV:
      CALL_ProgramUniform3uiv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_4UIV:
      CALL_ProgramUniform4uiv(ctx->Dispatch.Exec, (n[1].ui, n[2].i, n[3].i,
            case OPCODE_PROGRAM_UNIFORM_MATRIX22F:
      CALL_ProgramUniformMatrix2fv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX23F:
      CALL_ProgramUniformMatrix2x3fv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX24F:
      CALL_ProgramUniformMatrix2x4fv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX32F:
      CALL_ProgramUniformMatrix3x2fv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX33F:
      CALL_ProgramUniformMatrix3fv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX34F:
      CALL_ProgramUniformMatrix3x4fv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX42F:
      CALL_ProgramUniformMatrix4x2fv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX43F:
      CALL_ProgramUniformMatrix4x3fv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX44F:
      CALL_ProgramUniformMatrix4fv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX22D:
      CALL_ProgramUniformMatrix2dv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX23D:
      CALL_ProgramUniformMatrix2x3dv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX24D:
      CALL_ProgramUniformMatrix2x4dv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX32D:
      CALL_ProgramUniformMatrix3x2dv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX33D:
      CALL_ProgramUniformMatrix3dv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX34D:
      CALL_ProgramUniformMatrix3x4dv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX42D:
      CALL_ProgramUniformMatrix4x2dv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX43D:
      CALL_ProgramUniformMatrix4x3dv(ctx->Dispatch.Exec,
                  case OPCODE_PROGRAM_UNIFORM_MATRIX44D:
      CALL_ProgramUniformMatrix4dv(ctx->Dispatch.Exec,
                     case OPCODE_CLIP_CONTROL:
                  case OPCODE_CLAMP_COLOR:
                  case OPCODE_BIND_FRAGMENT_SHADER_ATI:
      CALL_BindFragmentShaderATI(ctx->Dispatch.Exec, (n[1].i));
      case OPCODE_SET_FRAGMENT_SHADER_CONSTANTS_ATI:
      CALL_SetFragmentShaderConstantATI(ctx->Dispatch.Exec, (n[1].ui, &n[2].f));
      case OPCODE_ATTR_1F_NV:
      CALL_VertexAttrib1fNV(ctx->Dispatch.Exec, (n[1].e, n[2].f));
      case OPCODE_ATTR_2F_NV:
      CALL_VertexAttrib2fvNV(ctx->Dispatch.Exec, (n[1].e, &n[2].f));
      case OPCODE_ATTR_3F_NV:
      CALL_VertexAttrib3fvNV(ctx->Dispatch.Exec, (n[1].e, &n[2].f));
      case OPCODE_ATTR_4F_NV:
      CALL_VertexAttrib4fvNV(ctx->Dispatch.Exec, (n[1].e, &n[2].f));
      case OPCODE_ATTR_1F_ARB:
      CALL_VertexAttrib1fARB(ctx->Dispatch.Exec, (n[1].e, n[2].f));
      case OPCODE_ATTR_2F_ARB:
      CALL_VertexAttrib2fvARB(ctx->Dispatch.Exec, (n[1].e, &n[2].f));
      case OPCODE_ATTR_3F_ARB:
      CALL_VertexAttrib3fvARB(ctx->Dispatch.Exec, (n[1].e, &n[2].f));
      case OPCODE_ATTR_4F_ARB:
      CALL_VertexAttrib4fvARB(ctx->Dispatch.Exec, (n[1].e, &n[2].f));
      case OPCODE_ATTR_1I:
      CALL_VertexAttribI1iEXT(ctx->Dispatch.Exec, (n[1].e, n[2].i));
      case OPCODE_ATTR_2I:
      CALL_VertexAttribI2ivEXT(ctx->Dispatch.Exec, (n[1].e, &n[2].i));
      case OPCODE_ATTR_3I:
      CALL_VertexAttribI3ivEXT(ctx->Dispatch.Exec, (n[1].e, &n[2].i));
      case OPCODE_ATTR_4I:
      CALL_VertexAttribI4ivEXT(ctx->Dispatch.Exec, (n[1].e, &n[2].i));
      case OPCODE_ATTR_1D: {
      GLdouble *d = (GLdouble *) &n[2];
   CALL_VertexAttribL1d(ctx->Dispatch.Exec, (n[1].ui, *d));
      }
   case OPCODE_ATTR_2D: {
      GLdouble *d = (GLdouble *) &n[2];
   CALL_VertexAttribL2dv(ctx->Dispatch.Exec, (n[1].ui, d));
      }
   case OPCODE_ATTR_3D: {
      GLdouble *d = (GLdouble *) &n[2];
   CALL_VertexAttribL3dv(ctx->Dispatch.Exec, (n[1].ui, d));
      }
   case OPCODE_ATTR_4D: {
      GLdouble *d = (GLdouble *) &n[2];
   CALL_VertexAttribL4dv(ctx->Dispatch.Exec, (n[1].ui, d));
      }
   case OPCODE_ATTR_1UI64: {
      uint64_t *ui64 = (uint64_t *) &n[2];
   CALL_VertexAttribL1ui64ARB(ctx->Dispatch.Exec, (n[1].ui, *ui64));
      }
   case OPCODE_MATERIAL:
      CALL_Materialfv(ctx->Dispatch.Exec, (n[1].e, n[2].e, &n[3].f));
      case OPCODE_BEGIN:
      CALL_Begin(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_END:
      CALL_End(ctx->Dispatch.Exec, ());
      case OPCODE_EVAL_C1:
      CALL_EvalCoord1f(ctx->Dispatch.Exec, (n[1].f));
      case OPCODE_EVAL_C2:
      CALL_EvalCoord2f(ctx->Dispatch.Exec, (n[1].f, n[2].f));
      case OPCODE_EVAL_P1:
      CALL_EvalPoint1(ctx->Dispatch.Exec, (n[1].i));
      case OPCODE_EVAL_P2:
                  /* GL_EXT_texture_integer */
   case OPCODE_CLEARCOLOR_I:
      CALL_ClearColorIiEXT(ctx->Dispatch.Exec, (n[1].i, n[2].i, n[3].i, n[4].i));
      case OPCODE_CLEARCOLOR_UI:
      CALL_ClearColorIuiEXT(ctx->Dispatch.Exec,
            case OPCODE_TEXPARAMETER_I:
      {
      GLint params[4];
   params[0] = n[3].i;
   params[1] = n[4].i;
   params[2] = n[5].i;
   params[3] = n[6].i;
      }
      case OPCODE_TEXPARAMETER_UI:
      {
      GLuint params[4];
   params[0] = n[3].ui;
   params[1] = n[4].ui;
   params[2] = n[5].ui;
   params[3] = n[6].ui;
                  case OPCODE_VERTEX_ATTRIB_DIVISOR:
      /* GL_EXT/ARB_instanced_arrays */
               case OPCODE_TEXTURE_BARRIER_NV:
                  /* GL_EXT/ARB_transform_feedback */
   case OPCODE_BEGIN_TRANSFORM_FEEDBACK:
      CALL_BeginTransformFeedback(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_END_TRANSFORM_FEEDBACK:
      CALL_EndTransformFeedback(ctx->Dispatch.Exec, ());
      case OPCODE_BIND_TRANSFORM_FEEDBACK:
      CALL_BindTransformFeedback(ctx->Dispatch.Exec, (n[1].e, n[2].ui));
      case OPCODE_PAUSE_TRANSFORM_FEEDBACK:
      CALL_PauseTransformFeedback(ctx->Dispatch.Exec, ());
      case OPCODE_RESUME_TRANSFORM_FEEDBACK:
      CALL_ResumeTransformFeedback(ctx->Dispatch.Exec, ());
      case OPCODE_DRAW_TRANSFORM_FEEDBACK:
      CALL_DrawTransformFeedback(ctx->Dispatch.Exec, (n[1].e, n[2].ui));
      case OPCODE_DRAW_TRANSFORM_FEEDBACK_STREAM:
      CALL_DrawTransformFeedbackStream(ctx->Dispatch.Exec,
            case OPCODE_DRAW_TRANSFORM_FEEDBACK_INSTANCED:
      CALL_DrawTransformFeedbackInstanced(ctx->Dispatch.Exec,
            case OPCODE_DRAW_TRANSFORM_FEEDBACK_STREAM_INSTANCED:
      CALL_DrawTransformFeedbackStreamInstanced(ctx->Dispatch.Exec,
                  case OPCODE_BIND_SAMPLER:
      CALL_BindSampler(ctx->Dispatch.Exec, (n[1].ui, n[2].ui));
      case OPCODE_SAMPLER_PARAMETERIV:
      {
      GLint params[4];
   params[0] = n[3].i;
   params[1] = n[4].i;
   params[2] = n[5].i;
   params[3] = n[6].i;
      }
      case OPCODE_SAMPLER_PARAMETERFV:
      {
      GLfloat params[4];
   params[0] = n[3].f;
   params[1] = n[4].f;
   params[2] = n[5].f;
   params[3] = n[6].f;
      }
      case OPCODE_SAMPLER_PARAMETERIIV:
      {
      GLint params[4];
   params[0] = n[3].i;
   params[1] = n[4].i;
   params[2] = n[5].i;
   params[3] = n[6].i;
      }
      case OPCODE_SAMPLER_PARAMETERUIV:
      {
      GLuint params[4];
   params[0] = n[3].ui;
   params[1] = n[4].ui;
   params[2] = n[5].ui;
   params[3] = n[6].ui;
                  /* ARB_compute_shader */
   case OPCODE_DISPATCH_COMPUTE:
                  /* GL_ARB_sync */
   case OPCODE_WAIT_SYNC:
      {
      union uint64_pair p;
   p.uint32[0] = n[2].ui;
   p.uint32[1] = n[3].ui;
   CALL_WaitSync(ctx->Dispatch.Exec,
                  /* GL_NV_conditional_render */
   case OPCODE_BEGIN_CONDITIONAL_RENDER:
      CALL_BeginConditionalRender(ctx->Dispatch.Exec, (n[1].i, n[2].e));
      case OPCODE_END_CONDITIONAL_RENDER:
                  case OPCODE_UNIFORM_BLOCK_BINDING:
                  case OPCODE_UNIFORM_SUBROUTINES:
      CALL_UniformSubroutinesuiv(ctx->Dispatch.Exec, (n[1].e, n[2].si,
               /* GL_EXT_window_rectangles */
   case OPCODE_WINDOW_RECTANGLES:
      CALL_WindowRectanglesEXT(
               /* GL_NV_conservative_raster */
   case OPCODE_SUBPIXEL_PRECISION_BIAS:
                  /* GL_NV_conservative_raster_dilate */
   case OPCODE_CONSERVATIVE_RASTER_PARAMETER_F:
                  /* GL_NV_conservative_raster_pre_snap_triangles */
   case OPCODE_CONSERVATIVE_RASTER_PARAMETER_I:
                  /* GL_EXT_direct_state_access */
   case OPCODE_MATRIX_LOAD:
      CALL_MatrixLoadfEXT(ctx->Dispatch.Exec, (n[1].e, &n[2].f));
      case OPCODE_MATRIX_MULT:
      CALL_MatrixMultfEXT(ctx->Dispatch.Exec, (n[1].e, &n[2].f));
      case OPCODE_MATRIX_ROTATE:
      CALL_MatrixRotatefEXT(ctx->Dispatch.Exec, (n[1].e, n[2].f, n[3].f, n[4].f, n[5].f));
      case OPCODE_MATRIX_SCALE:
      CALL_MatrixScalefEXT(ctx->Dispatch.Exec, (n[1].e, n[2].f, n[3].f, n[4].f));
      case OPCODE_MATRIX_TRANSLATE:
      CALL_MatrixTranslatefEXT(ctx->Dispatch.Exec, (n[1].e, n[2].f, n[3].f, n[4].f));
      case OPCODE_MATRIX_LOAD_IDENTITY:
      CALL_MatrixLoadIdentityEXT(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_MATRIX_ORTHO:
      CALL_MatrixOrthoEXT(ctx->Dispatch.Exec, (n[1].e,
                  case OPCODE_MATRIX_FRUSTUM:
      CALL_MatrixFrustumEXT(ctx->Dispatch.Exec, (n[1].e,
                  case OPCODE_MATRIX_PUSH:
      CALL_MatrixPushEXT(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_MATRIX_POP:
      CALL_MatrixPopEXT(ctx->Dispatch.Exec, (n[1].e));
      case OPCODE_TEXTUREPARAMETER_F:
      {
      GLfloat params[4];
   params[0] = n[4].f;
   params[1] = n[5].f;
   params[2] = n[6].f;
   params[3] = n[7].f;
      }
      case OPCODE_TEXTUREPARAMETER_I:
      {
      GLint params[4];
   params[0] = n[4].i;
   params[1] = n[5].i;
   params[2] = n[6].i;
   params[3] = n[7].i;
      }
      case OPCODE_TEXTUREPARAMETER_II:
      {
      GLint params[4];
   params[0] = n[4].i;
   params[1] = n[5].i;
   params[2] = n[6].i;
   params[3] = n[7].i;
      }
      case OPCODE_TEXTUREPARAMETER_IUI:
      {
      GLuint params[4];
   params[0] = n[4].ui;
   params[1] = n[5].ui;
   params[2] = n[6].ui;
   params[3] = n[7].ui;
      }
      case OPCODE_TEXTURE_IMAGE1D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TextureImage1DEXT(ctx->Dispatch.Exec, (n[1].ui, /* texture */
                                    n[2].e,  /* target */
   n[3].i,  /* level */
         }
      case OPCODE_TEXTURE_IMAGE2D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TextureImage2DEXT(ctx->Dispatch.Exec, (n[1].ui, /* texture */
                                    n[2].e,  /* target */
   n[3].i,  /* level */
   n[4].i,  /* components */
         }
      case OPCODE_TEXTURE_IMAGE3D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TextureImage3DEXT(ctx->Dispatch.Exec, (n[1].ui, /* texture */
                                    n[2].e,  /* target */
   n[3].i,  /* level */
   n[4].i,  /* components */
   n[5].i,  /* width */
         }
      case OPCODE_TEXTURE_SUB_IMAGE1D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TextureSubImage1DEXT(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].i,
                  }
      case OPCODE_TEXTURE_SUB_IMAGE2D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TextureSubImage2DEXT(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].i,
                        }
      case OPCODE_TEXTURE_SUB_IMAGE3D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_TextureSubImage3DEXT(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].i,
                              }
      case OPCODE_COPY_TEXTURE_IMAGE1D:
      CALL_CopyTextureImage1DEXT(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].i,
                  case OPCODE_COPY_TEXTURE_IMAGE2D:
      CALL_CopyTextureImage2DEXT(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].i,
                  case OPCODE_COPY_TEXTURE_SUB_IMAGE1D:
      CALL_CopyTextureSubImage1DEXT(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].i,
                  case OPCODE_COPY_TEXTURE_SUB_IMAGE2D:
      CALL_CopyTextureSubImage2DEXT(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].i,
                  case OPCODE_COPY_TEXTURE_SUB_IMAGE3D:
      CALL_CopyTextureSubImage3DEXT(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].i,
                        case OPCODE_BIND_MULTITEXTURE:
      CALL_BindMultiTextureEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].ui));
      case OPCODE_MULTITEXPARAMETER_F:
      {
      GLfloat params[4];
   params[0] = n[4].f;
   params[1] = n[5].f;
   params[2] = n[6].f;
   params[3] = n[7].f;
      }
      case OPCODE_MULTITEXPARAMETER_I:
      {
      GLint params[4];
   params[0] = n[4].i;
   params[1] = n[5].i;
   params[2] = n[6].i;
   params[3] = n[7].i;
      }
      case OPCODE_MULTITEXPARAMETER_II:
      {
      GLint params[4];
   params[0] = n[4].i;
   params[1] = n[5].i;
   params[2] = n[6].i;
   params[3] = n[7].i;
      }
      case OPCODE_MULTITEXPARAMETER_IUI:
      {
      GLuint params[4];
   params[0] = n[4].ui;
   params[1] = n[5].ui;
   params[2] = n[6].ui;
   params[3] = n[7].ui;
      }
      case OPCODE_MULTITEX_IMAGE1D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_MultiTexImage1DEXT(ctx->Dispatch.Exec, (n[1].e, /* texture */
                                    n[2].e,  /* target */
   n[3].i,  /* level */
         }
      case OPCODE_MULTITEX_IMAGE2D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_MultiTexImage2DEXT(ctx->Dispatch.Exec, (n[1].e, /* texture */
                                    n[2].e,  /* target */
   n[3].i,  /* level */
   n[4].i,  /* components */
         }
      case OPCODE_MULTITEX_IMAGE3D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_MultiTexImage3DEXT(ctx->Dispatch.Exec, (n[1].e, /* texture */
                                    n[2].e,  /* target */
   n[3].i,  /* level */
   n[4].i,  /* components */
   n[5].i,  /* width */
         }
      case OPCODE_MULTITEX_SUB_IMAGE1D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_MultiTexSubImage1DEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].i,
                  }
      case OPCODE_MULTITEX_SUB_IMAGE2D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_MultiTexSubImage2DEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].i,
                        }
      case OPCODE_MULTITEX_SUB_IMAGE3D:
      {
      const struct gl_pixelstore_attrib save = ctx->Unpack;
   ctx->Unpack = ctx->DefaultPacking;
   CALL_MultiTexSubImage3DEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].i,
                              }
      case OPCODE_COPY_MULTITEX_IMAGE1D:
      CALL_CopyMultiTexImage1DEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].i,
                  case OPCODE_COPY_MULTITEX_IMAGE2D:
      CALL_CopyMultiTexImage2DEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].i,
                  case OPCODE_COPY_MULTITEX_SUB_IMAGE1D:
      CALL_CopyMultiTexSubImage1DEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].i,
                  case OPCODE_COPY_MULTITEX_SUB_IMAGE2D:
      CALL_CopyMultiTexSubImage2DEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].i,
                  case OPCODE_COPY_MULTITEX_SUB_IMAGE3D:
      CALL_CopyMultiTexSubImage3DEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].i,
                        case OPCODE_MULTITEXENV:
      {
      GLfloat params[4];
   params[0] = n[4].f;
   params[1] = n[5].f;
   params[2] = n[6].f;
   params[3] = n[7].f;
      }
      case OPCODE_COMPRESSED_TEXTURE_IMAGE_1D:
      CALL_CompressedTextureImage1DEXT(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].i,
                  case OPCODE_COMPRESSED_TEXTURE_IMAGE_2D:
      CALL_CompressedTextureImage2DEXT(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].i,
                        case OPCODE_COMPRESSED_TEXTURE_IMAGE_3D:
      CALL_CompressedTextureImage3DEXT(ctx->Dispatch.Exec, (n[1].ui, n[2].e, n[3].i,
                        case OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_1D:
      CALL_CompressedTextureSubImage1DEXT(ctx->Dispatch.Exec,
                        case OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_2D:
      CALL_CompressedTextureSubImage2DEXT(ctx->Dispatch.Exec,
                        case OPCODE_COMPRESSED_TEXTURE_SUB_IMAGE_3D:
      CALL_CompressedTextureSubImage3DEXT(ctx->Dispatch.Exec,
                              case OPCODE_COMPRESSED_MULTITEX_IMAGE_1D:
      CALL_CompressedMultiTexImage1DEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].i,
                  case OPCODE_COMPRESSED_MULTITEX_IMAGE_2D:
      CALL_CompressedMultiTexImage2DEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].i,
                        case OPCODE_COMPRESSED_MULTITEX_IMAGE_3D:
      CALL_CompressedMultiTexImage3DEXT(ctx->Dispatch.Exec, (n[1].e, n[2].e, n[3].i,
                        case OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_1D:
      CALL_CompressedMultiTexSubImage1DEXT(ctx->Dispatch.Exec,
                        case OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_2D:
      CALL_CompressedMultiTexSubImage2DEXT(ctx->Dispatch.Exec,
                        case OPCODE_COMPRESSED_MULTITEX_SUB_IMAGE_3D:
      CALL_CompressedMultiTexSubImage3DEXT(ctx->Dispatch.Exec,
                              case OPCODE_NAMED_PROGRAM_STRING:
      CALL_NamedProgramStringEXT(ctx->Dispatch.Exec,
                  case OPCODE_NAMED_PROGRAM_LOCAL_PARAMETER:
      CALL_NamedProgramLocalParameter4fEXT(ctx->Dispatch.Exec,
                     case OPCODE_PRIMITIVE_BOUNDING_BOX:
      CALL_PrimitiveBoundingBox(ctx->Dispatch.Exec,
                  case OPCODE_VERTEX_LIST:
                  case OPCODE_VERTEX_LIST_COPY_CURRENT:
                  case OPCODE_VERTEX_LIST_LOOPBACK:
                  case OPCODE_CONTINUE:
      n = (Node *) get_pointer(&n[1]);
      default:
      {
      char msg[1000];
   snprintf(msg, sizeof(msg), "Error in execute_list: opcode=%d",
            }
      case OPCODE_END_OF_LIST:
               /* increment n to point to next compiled command */
   assert(n[0].InstSize > 0);
         }
            /**********************************************************************/
   /*                           GL functions                             */
   /**********************************************************************/
      /**
   * Test if a display list number is valid.
   */
   GLboolean GLAPIENTRY
   _mesa_IsList(GLuint list)
   {
      GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0, 0);      /* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);
      }
         /**
   * Delete a sequence of consecutive display lists.
   */
   void GLAPIENTRY
   _mesa_DeleteLists(GLuint list, GLsizei range)
   {
      GET_CURRENT_CONTEXT(ctx);
   GLuint i;
   FLUSH_VERTICES(ctx, 0, 0);      /* must be called before assert */
            if (range < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteLists");
               _mesa_HashLockMutex(ctx->Shared->DisplayList);
   for (i = list; i < list + range; i++) {
         }
      }
         /**
   * Return a display list number, n, such that lists n through n+range-1
   * are free.
   */
   GLuint GLAPIENTRY
   _mesa_GenLists(GLsizei range)
   {
      GET_CURRENT_CONTEXT(ctx);
   GLuint base;
   FLUSH_VERTICES(ctx, 0, 0);      /* must be called before assert */
            if (range < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenLists");
      }
   if (range == 0) {
                  /*
   * Make this an atomic operation
   */
            base = _mesa_HashFindFreeKeyBlock(ctx->Shared->DisplayList, range);
   if (base) {
      /* reserve the list IDs by with empty/dummy lists */
   GLint i;
   for (i = 0; i < range; i++) {
      _mesa_HashInsertLocked(ctx->Shared->DisplayList, base + i,
                              }
         /**
   * Begin a new display list.
   */
   void GLAPIENTRY
   _mesa_NewList(GLuint name, GLenum mode)
   {
               FLUSH_CURRENT(ctx, 0);       /* must be called before assert */
            if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glNewList %u %s\n", name,
         if (name == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glNewList");
               if (mode != GL_COMPILE && mode != GL_COMPILE_AND_EXECUTE) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glNewList");
               if (ctx->ListState.CurrentList) {
      /* already compiling a display list */
   _mesa_error(ctx, GL_INVALID_OPERATION, "glNewList");
               ctx->CompileFlag = GL_TRUE;
            /* Reset accumulated list state */
            /* Allocate new display list */
   ctx->ListState.CurrentList = make_list(name, BLOCK_SIZE);
   ctx->ListState.CurrentBlock = ctx->ListState.CurrentList->Head;
   ctx->ListState.CurrentPos = 0;
   ctx->ListState.LastInstSize = 0;
                     ctx->Dispatch.Current = ctx->Dispatch.Save;
   _glapi_set_dispatch(ctx->Dispatch.Current);
   if (!ctx->GLThread.enabled) {
            }
         /**
   * Walk all the opcode from a given list, recursively if OPCODE_CALL_LIST(S) is used,
   * and replace OPCODE_VERTEX_LIST[_COPY_CURRENT] occurences by OPCODE_VERTEX_LIST_LOOPBACK.
   */
   static void
   replace_op_vertex_list_recursively(struct gl_context *ctx, struct gl_display_list *dlist)
   {
      Node *n = get_list_head(ctx, dlist);
   while (true) {
      const OpCode opcode = n[0].opcode;
   switch (opcode) {
      case OPCODE_VERTEX_LIST:
   case OPCODE_VERTEX_LIST_COPY_CURRENT:
      n[0].opcode = OPCODE_VERTEX_LIST_LOOPBACK;
      case OPCODE_CONTINUE:
      n = (Node *)get_pointer(&n[1]);
      case OPCODE_CALL_LIST:
      replace_op_vertex_list_recursively(ctx, _mesa_lookup_list(ctx, (int)n[1].ui, true));
      case OPCODE_CALL_LISTS: {
      GLbyte *bptr;
   GLubyte *ubptr;
   GLshort *sptr;
   GLushort *usptr;
   GLint *iptr;
   GLuint *uiptr;
   GLfloat *fptr;
   switch(n[2].e) {
      case GL_BYTE:
      bptr = (GLbyte *) get_pointer(&n[3]);
   for (unsigned i = 0; i < n[1].i; i++)
            case GL_UNSIGNED_BYTE:
      ubptr = (GLubyte *) get_pointer(&n[3]);
   for (unsigned i = 0; i < n[1].i; i++)
            case GL_SHORT:
      sptr = (GLshort *) get_pointer(&n[3]);
   for (unsigned i = 0; i < n[1].i; i++)
            case GL_UNSIGNED_SHORT:
      usptr = (GLushort *) get_pointer(&n[3]);
   for (unsigned i = 0; i < n[1].i; i++)
            case GL_INT:
      iptr = (GLint *) get_pointer(&n[3]);
   for (unsigned i = 0; i < n[1].i; i++)
            case GL_UNSIGNED_INT:
      uiptr = (GLuint *) get_pointer(&n[3]);
   for (unsigned i = 0; i < n[1].i; i++)
            case GL_FLOAT:
      fptr = (GLfloat *) get_pointer(&n[3]);
   for (unsigned i = 0; i < n[1].i; i++)
            case GL_2_BYTES:
      ubptr = (GLubyte *) get_pointer(&n[3]);
   for (unsigned i = 0; i < n[1].i; i++) {
      replace_op_vertex_list_recursively(ctx,
            }
      case GL_3_BYTES:
      ubptr = (GLubyte *) get_pointer(&n[3]);
   for (unsigned i = 0; i < n[1].i; i++) {
      replace_op_vertex_list_recursively(ctx,
                  }
      case GL_4_BYTES:
      ubptr = (GLubyte *) get_pointer(&n[3]);
   for (unsigned i = 0; i < n[1].i; i++) {
      replace_op_vertex_list_recursively(ctx,
                        }
            }
   case OPCODE_END_OF_LIST:
         default:
      }
         }
         /**
   * End definition of current display list.
   */
   void GLAPIENTRY
   _mesa_EndList(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   SAVE_FLUSH_VERTICES(ctx);
            if (MESA_VERBOSE & VERBOSE_API)
            if (ctx->ExecuteFlag && _mesa_inside_dlist_begin_end(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               /* Check that a list is under construction */
   if (!ctx->ListState.CurrentList) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEndList");
               /* Call before emitting END_OF_LIST, in case the driver wants to
   * emit opcodes itself.
   */
                              if (ctx->ListState.Current.UseLoopback)
            struct gl_dlist_state *list = &ctx->ListState;
   list->CurrentList->execute_glthread =
                  if ((list->CurrentList->Head == list->CurrentBlock) &&
      (list->CurrentPos < BLOCK_SIZE)) {
   /* This list has a low number of commands. Instead of storing them in a malloc-ed block
   * of memory (list->CurrentBlock), we store them in ctx->Shared->small_dlist_store.ptr.
   * This reduces cache misses in execute_list on successive lists since their commands
   * are now stored in the same array instead of being scattered in memory.
   */
   list->CurrentList->small_list = true;
            if (ctx->Shared->small_dlist_store.size == 0) {
                           if ((start + list->CurrentPos) > ctx->Shared->small_dlist_store.size) {
      ctx->Shared->small_dlist_store.size =
         ctx->Shared->small_dlist_store.ptr = realloc(
      ctx->Shared->small_dlist_store.ptr,
   }
   list->CurrentList->start = start;
            memcpy(&ctx->Shared->small_dlist_store.ptr[start],
                              } else {
      /* Keep the mallocated storage */
               /* Destroy old list, if any */
            /* Install the new list */
   _mesa_HashInsertLocked(ctx->Shared->DisplayList,
                  if (MESA_VERBOSE & VERBOSE_DISPLAY_LIST)
                     ctx->ListState.CurrentList = NULL;
   ctx->ListState.CurrentBlock = NULL;
   ctx->ListState.CurrentPos = 0;
   ctx->ListState.LastInstSize = 0;
   ctx->ExecuteFlag = GL_TRUE;
            ctx->Dispatch.Current = ctx->Dispatch.Exec;
   _glapi_set_dispatch(ctx->Dispatch.Current);
   if (!ctx->GLThread.enabled) {
            }
         void GLAPIENTRY
   _mesa_CallList(GLuint list)
   {
      GLboolean save_compile_flag;
   GET_CURRENT_CONTEXT(ctx);
            if (MESA_VERBOSE & VERBOSE_API)
            if (list == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCallList(list==0)");
               if (0)
            /* Save the CompileFlag status, turn it off, execute the display list,
   * and restore the CompileFlag. This is needed for GL_COMPILE_AND_EXECUTE
   * because the call is already recorded and we just need to execute it.
   */
   save_compile_flag = ctx->CompileFlag;
   if (save_compile_flag) {
                  _mesa_HashLockMutex(ctx->Shared->DisplayList);
   execute_list(ctx, list);
   _mesa_HashUnlockMutex(ctx->Shared->DisplayList);
            /* also restore API function pointers to point to "save" versions */
   if (save_compile_flag) {
      ctx->Dispatch.Current = ctx->Dispatch.Save;
   if (!ctx->GLThread.enabled) {
               }
         /**
   * Execute glCallLists:  call multiple display lists.
   */
   void GLAPIENTRY
   _mesa_CallLists(GLsizei n, GLenum type, const GLvoid * lists)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (MESA_VERBOSE & VERBOSE_API)
            if (type < GL_BYTE || type > GL_4_BYTES) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCallLists(type)");
               if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCallLists(n < 0)");
      } else if (n == 0 || lists == NULL) {
      /* nothing to do */
               /* Save the CompileFlag status, turn it off, execute the display lists,
   * and restore the CompileFlag. This is needed for GL_COMPILE_AND_EXECUTE
   * because the call is already recorded and we just need to execute it.
   */
   save_compile_flag = ctx->CompileFlag;
            GLbyte *bptr;
   GLubyte *ubptr;
   GLshort *sptr;
   GLushort *usptr;
   GLint *iptr;
   GLuint *uiptr;
                              /* A loop inside a switch is faster than a switch inside a loop. */
   switch (type) {
   case GL_BYTE:
      bptr = (GLbyte *) lists;
   for (unsigned i = 0; i < n; i++)
            case GL_UNSIGNED_BYTE:
      ubptr = (GLubyte *) lists;
   for (unsigned i = 0; i < n; i++)
            case GL_SHORT:
      sptr = (GLshort *) lists;
   for (unsigned i = 0; i < n; i++)
            case GL_UNSIGNED_SHORT:
      usptr = (GLushort *) lists;
   for (unsigned i = 0; i < n; i++)
            case GL_INT:
      iptr = (GLint *) lists;
   for (unsigned i = 0; i < n; i++)
            case GL_UNSIGNED_INT:
      uiptr = (GLuint *) lists;
   for (unsigned i = 0; i < n; i++)
            case GL_FLOAT:
      fptr = (GLfloat *) lists;
   for (unsigned i = 0; i < n; i++)
            case GL_2_BYTES:
      ubptr = (GLubyte *) lists;
   for (unsigned i = 0; i < n; i++) {
      execute_list(ctx, base +
            }
      case GL_3_BYTES:
      ubptr = (GLubyte *) lists;
   for (unsigned i = 0; i < n; i++) {
      execute_list(ctx, base +
                  }
      case GL_4_BYTES:
      ubptr = (GLubyte *) lists;
   for (unsigned i = 0; i < n; i++) {
      execute_list(ctx, base +
               (int)ubptr[4 * i] * 16777216 +
      }
               _mesa_HashUnlockMutex(ctx->Shared->DisplayList);
            /* also restore API function pointers to point to "save" versions */
   if (save_compile_flag) {
      ctx->Dispatch.Current = ctx->Dispatch.Save;
   if (!ctx->GLThread.enabled) {
               }
         /**
   * Set the offset added to list numbers in glCallLists.
   */
   void GLAPIENTRY
   _mesa_ListBase(GLuint base)
   {
      GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0, GL_LIST_BIT);   /* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END(ctx);
      }
      /**
   * Setup the given dispatch table to point to Mesa's display list
   * building functions.
   */
   void
   _mesa_init_dispatch_save(const struct gl_context *ctx)
   {
      struct _glapi_table *table = ctx->Dispatch.Save;
            /* Initially populate the dispatch table with the contents of the
   * normal-execution dispatch table.  This lets us skip populating functions
   * that should be called directly instead of compiled into display lists.
   */
   memcpy(table, ctx->Dispatch.OutsideBeginEnd,
         #include "api_save_init.h"
   }
            static const char *
   enum_string(GLenum k)
   {
         }
         /**
   * Print the commands in a display list.  For debugging only.
   * TODO: many commands aren't handled yet.
   * \param fname  filename to write display list to.  If null, use stdout.
   */
   static void
   print_list(struct gl_context *ctx, GLuint list, const char *fname)
   {
      struct gl_display_list *dlist;
   Node *n;
            if (fname) {
      f = fopen(fname, "w");
   if (!f)
               if (!_mesa_get_list(ctx, list, &dlist, true)) {
      fprintf(f, "%u is not a display list ID\n", list);
   fflush(f);
   if (fname)
                                       while (1) {
               switch (opcode) {
      case OPCODE_ACCUM:
      fprintf(f, "Accum %s %g\n", enum_string(n[1].e), n[2].f);
      case OPCODE_ACTIVE_TEXTURE:
      fprintf(f, "ActiveTexture(%s)\n", enum_string(n[1].e));
      case OPCODE_BITMAP:
      fprintf(f, "Bitmap %d %d %g %g %g %g %p\n", n[1].i, n[2].i,
         n[3].f, n[4].f, n[5].f, n[6].f,
      case OPCODE_BLEND_COLOR:
      fprintf(f, "BlendColor %f, %f, %f, %f\n",
            case OPCODE_BLEND_EQUATION:
      fprintf(f, "BlendEquation %s\n",
            case OPCODE_BLEND_EQUATION_SEPARATE:
      fprintf(f, "BlendEquationSeparate %s, %s\n",
         enum_string(n[1].e),
      case OPCODE_BLEND_FUNC_SEPARATE:
      fprintf(f, "BlendFuncSeparate %s, %s, %s, %s\n",
         enum_string(n[1].e),
   enum_string(n[2].e),
   enum_string(n[3].e),
      case OPCODE_BLEND_EQUATION_I:
      fprintf(f, "BlendEquationi %u, %s\n",
            case OPCODE_BLEND_EQUATION_SEPARATE_I:
      fprintf(f, "BlendEquationSeparatei %u, %s, %s\n",
            case OPCODE_BLEND_FUNC_I:
      fprintf(f, "BlendFunci %u, %s, %s\n",
            case OPCODE_BLEND_FUNC_SEPARATE_I:
      fprintf(f, "BlendFuncSeparatei %u, %s, %s, %s, %s\n",
         n[1].ui,
   enum_string(n[2].e),
   enum_string(n[3].e),
   enum_string(n[4].e),
      case OPCODE_CALL_LIST:
      fprintf(f, "CallList %d\n", (int) n[1].ui);
      case OPCODE_CALL_LISTS:
      fprintf(f, "CallLists %d, %s\n", n[1].i, enum_string(n[1].e));
      case OPCODE_DISABLE:
      fprintf(f, "Disable %s\n", enum_string(n[1].e));
      case OPCODE_ENABLE:
      fprintf(f, "Enable %s\n", enum_string(n[1].e));
      case OPCODE_FRUSTUM:
      fprintf(f, "Frustum %g %g %g %g %g %g\n",
            case OPCODE_LINE_STIPPLE:
      fprintf(f, "LineStipple %d %x\n", n[1].i, (int) n[2].us);
      case OPCODE_LINE_WIDTH:
      fprintf(f, "LineWidth %f\n", n[1].f);
      case OPCODE_LOAD_IDENTITY:
      fprintf(f, "LoadIdentity\n");
      case OPCODE_LOAD_MATRIX:
      fprintf(f, "LoadMatrix\n");
   fprintf(f, "  %8f %8f %8f %8f\n",
         fprintf(f, "  %8f %8f %8f %8f\n",
         fprintf(f, "  %8f %8f %8f %8f\n",
         fprintf(f, "  %8f %8f %8f %8f\n",
            case OPCODE_MULT_MATRIX:
      fprintf(f, "MultMatrix (or Rotate)\n");
   fprintf(f, "  %8f %8f %8f %8f\n",
         fprintf(f, "  %8f %8f %8f %8f\n",
         fprintf(f, "  %8f %8f %8f %8f\n",
         fprintf(f, "  %8f %8f %8f %8f\n",
            case OPCODE_ORTHO:
      fprintf(f, "Ortho %g %g %g %g %g %g\n",
            case OPCODE_POINT_SIZE:
      fprintf(f, "PointSize %f\n", n[1].f);
      case OPCODE_POP_ATTRIB:
      fprintf(f, "PopAttrib\n");
      case OPCODE_POP_MATRIX:
      fprintf(f, "PopMatrix\n");
      case OPCODE_POP_NAME:
      fprintf(f, "PopName\n");
      case OPCODE_PUSH_ATTRIB:
      fprintf(f, "PushAttrib %x\n", n[1].bf);
      case OPCODE_PUSH_MATRIX:
      fprintf(f, "PushMatrix\n");
      case OPCODE_PUSH_NAME:
      fprintf(f, "PushName %d\n", (int) n[1].ui);
      case OPCODE_RASTER_POS:
      fprintf(f, "RasterPos %g %g %g %g\n",
            case OPCODE_ROTATE:
      fprintf(f, "Rotate %g %g %g %g\n",
            case OPCODE_SCALE:
      fprintf(f, "Scale %g %g %g\n", n[1].f, n[2].f, n[3].f);
      case OPCODE_TRANSLATE:
      fprintf(f, "Translate %g %g %g\n", n[1].f, n[2].f, n[3].f);
      case OPCODE_BIND_TEXTURE:
      fprintf(f, "BindTexture %s %d\n",
            case OPCODE_SHADE_MODEL:
      fprintf(f, "ShadeModel %s\n", _mesa_enum_to_string(n[1].ui));
      case OPCODE_MAP1:
      fprintf(f, "Map1 %s %.3f %.3f %d %d\n",
                  case OPCODE_MAP2:
      fprintf(f, "Map2 %s %.3f %.3f %.3f %.3f %d %d %d %d\n",
               _mesa_enum_to_string(n[1].ui),
      case OPCODE_MAPGRID1:
      fprintf(f, "MapGrid1 %d %.3f %.3f\n", n[1].i, n[2].f, n[3].f);
      case OPCODE_MAPGRID2:
      fprintf(f, "MapGrid2 %d %.3f %.3f, %d %.3f %.3f\n",
            case OPCODE_EVALMESH1:
      fprintf(f, "EvalMesh1 %d %d\n", n[1].i, n[2].i);
      case OPCODE_EVALMESH2:
      fprintf(f, "EvalMesh2 %d %d %d %d\n",
               case OPCODE_ATTR_1F_NV:
      fprintf(f, "ATTR_1F_NV attr %d: %f\n", n[1].i, n[2].f);
      case OPCODE_ATTR_2F_NV:
      fprintf(f, "ATTR_2F_NV attr %d: %f %f\n",
            case OPCODE_ATTR_3F_NV:
      fprintf(f, "ATTR_3F_NV attr %d: %f %f %f\n",
            case OPCODE_ATTR_4F_NV:
      fprintf(f, "ATTR_4F_NV attr %d: %f %f %f %f\n",
            case OPCODE_ATTR_1F_ARB:
      fprintf(f, "ATTR_1F_ARB attr %d: %f\n", n[1].i, n[2].f);
      case OPCODE_ATTR_2F_ARB:
      fprintf(f, "ATTR_2F_ARB attr %d: %f %f\n",
            case OPCODE_ATTR_3F_ARB:
      fprintf(f, "ATTR_3F_ARB attr %d: %f %f %f\n",
            case OPCODE_ATTR_4F_ARB:
      fprintf(f, "ATTR_4F_ARB attr %d: %f %f %f %f\n",
               case OPCODE_MATERIAL:
      fprintf(f, "MATERIAL %x %x: %f %f %f %f\n",
            case OPCODE_BEGIN:
      fprintf(f, "BEGIN %x\n", n[1].i);
      case OPCODE_END:
      fprintf(f, "END\n");
      case OPCODE_EVAL_C1:
      fprintf(f, "EVAL_C1 %f\n", n[1].f);
      case OPCODE_EVAL_C2:
      fprintf(f, "EVAL_C2 %f %f\n", n[1].f, n[2].f);
      case OPCODE_EVAL_P1:
      fprintf(f, "EVAL_P1 %d\n", n[1].i);
      case OPCODE_EVAL_P2:
                  case OPCODE_PROVOKING_VERTEX:
      fprintf(f, "ProvokingVertex %s\n",
                  /*
   * meta opcodes/commands
      case OPCODE_ERROR:
      fprintf(f, "Error: %s %s\n", enum_string(n[1].e),
            case OPCODE_CONTINUE:
      fprintf(f, "DISPLAY-LIST-CONTINUE\n");
   n = (Node *) get_pointer(&n[1]);
      case OPCODE_VERTEX_LIST:
   case OPCODE_VERTEX_LIST_LOOPBACK:
   case OPCODE_VERTEX_LIST_COPY_CURRENT:
      vbo_print_vertex_list(ctx, (struct vbo_save_vertex_list *) &n[0], opcode, f);
      default:
      if (opcode < 0 || opcode > OPCODE_END_OF_LIST) {
      printf
      ("ERROR IN DISPLAY LIST: opcode = %d, address = %p\n",
   } else {
      fprintf(f, "command %d, %u operands\n", opcode,
            }
      case OPCODE_END_OF_LIST:
      fprintf(f, "END-LIST %u\n", list);
   fflush(f);
   if (fname)
                  /* increment n to point to next compiled command */
   assert(n[0].InstSize > 0);
         }
         void
   _mesa_glthread_execute_list(struct gl_context *ctx, GLuint list)
   {
               if (list == 0 ||
      !_mesa_get_list(ctx, list, &dlist, true) ||
   !dlist->execute_glthread)
                  while (1) {
               switch (opcode) {
      case OPCODE_CALL_LIST:
      /* Generated by glCallList(), don't add ListBase */
   if (ctx->GLThread.ListCallDepth < MAX_LIST_NESTING) {
      ctx->GLThread.ListCallDepth++;
   _mesa_glthread_execute_list(ctx, n[1].ui);
      }
      case OPCODE_CALL_LISTS:
      if (ctx->GLThread.ListCallDepth < MAX_LIST_NESTING) {
      ctx->GLThread.ListCallDepth++;
   _mesa_glthread_CallLists(ctx, n[1].i, n[2].e, get_pointer(&n[3]));
      }
      case OPCODE_DISABLE:
      _mesa_glthread_Disable(ctx, n[1].e);
      case OPCODE_ENABLE:
      _mesa_glthread_Enable(ctx, n[1].e);
      case OPCODE_LIST_BASE:
      _mesa_glthread_ListBase(ctx, n[1].ui);
      case OPCODE_MATRIX_MODE:
      _mesa_glthread_MatrixMode(ctx, n[1].e);
      case OPCODE_POP_ATTRIB:
      _mesa_glthread_PopAttrib(ctx);
      case OPCODE_POP_MATRIX:
      _mesa_glthread_PopMatrix(ctx);
      case OPCODE_PUSH_ATTRIB:
      _mesa_glthread_PushAttrib(ctx, n[1].bf);
      case OPCODE_PUSH_MATRIX:
      _mesa_glthread_PushMatrix(ctx);
      case OPCODE_ACTIVE_TEXTURE:   /* GL_ARB_multitexture */
      _mesa_glthread_ActiveTexture(ctx, n[1].e);
      case OPCODE_MATRIX_PUSH:
      _mesa_glthread_MatrixPushEXT(ctx, n[1].e);
      case OPCODE_MATRIX_POP:
      _mesa_glthread_MatrixPopEXT(ctx, n[1].e);
      case OPCODE_CONTINUE:
      n = (Node *)get_pointer(&n[1]);
      case OPCODE_END_OF_LIST:
      ctx->GLThread.ListCallDepth--;
      default:
      /* ignore */
            /* increment n to point to next compiled command */
   assert(n[0].InstSize > 0);
         }
      static bool
   _mesa_glthread_should_execute_list(struct gl_context *ctx,
         {
               while (1) {
               switch (opcode) {
   case OPCODE_CALL_LIST:
   case OPCODE_CALL_LISTS:
   case OPCODE_DISABLE:
   case OPCODE_ENABLE:
   case OPCODE_LIST_BASE:
   case OPCODE_MATRIX_MODE:
   case OPCODE_POP_ATTRIB:
   case OPCODE_POP_MATRIX:
   case OPCODE_PUSH_ATTRIB:
   case OPCODE_PUSH_MATRIX:
   case OPCODE_ACTIVE_TEXTURE:   /* GL_ARB_multitexture */
   case OPCODE_MATRIX_PUSH:
   case OPCODE_MATRIX_POP:
         case OPCODE_CONTINUE:
      n = (Node *)get_pointer(&n[1]);
      case OPCODE_END_OF_LIST:
         default:
      /* ignore */
               /* increment n to point to next compiled command */
   assert(n[0].InstSize > 0);
      }
      }
         /**
   * Clients may call this function to help debug display list problems.
   * This function is _ONLY_FOR_DEBUGGING_PURPOSES_.  It may be removed,
   * changed, or break in the future without notice.
   */
   void
   mesa_print_display_list(GLuint list)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**********************************************************************/
   /*****                      Initialization                        *****/
   /**********************************************************************/
      /**
   * Initialize display list state for given context.
   */
   void
   _mesa_init_display_list(struct gl_context *ctx)
   {
      /* Display list */
   ctx->ListState.CallDepth = 1;
   ctx->ExecuteFlag = GL_TRUE;
   ctx->CompileFlag = GL_FALSE;
   ctx->ListState.CurrentBlock = NULL;
   ctx->ListState.CurrentPos = 0;
            /* Display List group */
      }
         void
   _mesa_init_dispatch_save_begin_end(struct gl_context *ctx)
   {
      struct _glapi_table *tab = ctx->Dispatch.Save;
         #define NAME_AE(x) _mesa_##x
   #define NAME_CALLLIST(x) save_##x
   #define NAME(x) save_##x
   #define NAME_ES(x) save_##x
            }
