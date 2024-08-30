   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
   * Copyright (C) 2008  VMware, Inc.  All Rights Reserved.
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
   * \file context.c
   * Mesa context/visual/framebuffer management functions.
   * \author Brian Paul
   */
      /**
   * \mainpage Mesa Main Module
   *
   * \section MainIntroduction Introduction
   *
   * The Mesa Main module consists of all the files in the main/ directory.
   * Among the features of this module are:
   * <UL>
   * <LI> Structures to represent most GL state </LI>
   * <LI> State set/get functions </LI>
   * <LI> Display lists </LI>
   * <LI> Texture unit, object and image handling </LI>
   * <LI> Matrix and attribute stacks </LI>
   * </UL>
   *
   * Other modules are responsible for API dispatch, vertex transformation,
   * point/line/triangle setup, rasterization, vertex array caching,
   * vertex/fragment programs/shaders, etc.
   *
   *
   * \section AboutDoxygen About Doxygen
   *
   * If you're viewing this information as Doxygen-generated HTML you'll
   * see the documentation index at the top of this page.
   *
   * The first line lists the Mesa source code modules.
   * The second line lists the indexes available for viewing the documentation
   * for each module.
   *
   * Selecting the <b>Main page</b> link will display a summary of the module
   * (this page).
   *
   * Selecting <b>Data Structures</b> will list all C structures.
   *
   * Selecting the <b>File List</b> link will list all the source files in
   * the module.
   * Selecting a filename will show a list of all functions defined in that file.
   *
   * Selecting the <b>Data Fields</b> link will display a list of all
   * documented structure members.
   *
   * Selecting the <b>Globals</b> link will display a list
   * of all functions, structures, global variables and macros in the module.
   *
   */
         #include "util/glheader.h"
      #include "accum.h"
   #include "arrayobj.h"
   #include "attrib.h"
   #include "bbox.h"
   #include "blend.h"
   #include "buffers.h"
   #include "bufferobj.h"
   #include "conservativeraster.h"
   #include "context.h"
   #include "debug.h"
   #include "debug_output.h"
   #include "depth.h"
   #include "dlist.h"
   #include "draw_validate.h"
   #include "eval.h"
   #include "extensions.h"
   #include "fbobject.h"
   #include "feedback.h"
   #include "fog.h"
   #include "formats.h"
   #include "framebuffer.h"
   #include "glthread.h"
   #include "hint.h"
   #include "hash.h"
   #include "light.h"
   #include "lines.h"
   #include "macros.h"
   #include "matrix.h"
   #include "multisample.h"
   #include "performance_monitor.h"
   #include "performance_query.h"
   #include "pipelineobj.h"
   #include "pixel.h"
   #include "pixelstore.h"
   #include "points.h"
   #include "polygon.h"
   #include "queryobj.h"
   #include "syncobj.h"
   #include "rastpos.h"
   #include "remap.h"
   #include "scissor.h"
   #include "shared.h"
   #include "shaderobj.h"
   #include "shaderimage.h"
   #include "state.h"
   #include "util/u_debug.h"
   #include "util/disk_cache.h"
   #include "util/strtod.h"
   #include "util/u_call_once.h"
   #include "stencil.h"
   #include "shaderimage.h"
   #include "texcompress_s3tc.h"
   #include "texstate.h"
   #include "transformfeedback.h"
   #include "mtypes.h"
   #include "varray.h"
   #include "version.h"
   #include "viewport.h"
   #include "texturebindless.h"
   #include "program/program.h"
   #include "math/m_matrix.h"
   #include "main/dispatch.h" /* for _gloffset_COUNT */
   #include "macros.h"
   #include "git_sha1.h"
      #include "compiler/glsl_types.h"
   #include "compiler/glsl/builtin_functions.h"
   #include "compiler/glsl/glsl_parser_extras.h"
   #include <stdbool.h>
   #include "util/u_memory.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_texture.h"
   #include "state_tracker/st_cb_flush.h"
      #ifndef MESA_VERBOSE
   int MESA_VERBOSE = 0;
   #endif
      #ifndef MESA_DEBUG_FLAGS
   int MESA_DEBUG_FLAGS = 0;
   #endif
         /* ubyte -> float conversion */
   GLfloat _mesa_ubyte_to_float_color_tab[256];
         /**********************************************************************/
   /** \name Context allocation, initialization, destroying
   *
   * The purpose of the most initialization functions here is to provide the
   * default state values according to the OpenGL specification.
   */
   /**********************************************************************/
   /*@{*/
         /**
   * Calls all the various one-time-fini functions in Mesa
   */
      static void
   one_time_fini(void)
   {
         }
      /**
   * Calls all the various one-time-init functions in Mesa
   */
      static void
   one_time_init(const char *extensions_override)
   {
               STATIC_ASSERT(sizeof(GLbyte) == 1);
   STATIC_ASSERT(sizeof(GLubyte) == 1);
   STATIC_ASSERT(sizeof(GLshort) == 2);
   STATIC_ASSERT(sizeof(GLushort) == 2);
   STATIC_ASSERT(sizeof(GLint) == 4);
                     const char *env_const = os_get_option("MESA_EXTENSION_OVERRIDE");
   if (env_const) {
      if (extensions_override &&
      strcmp(extensions_override, env_const)) {
      }
                           for (i = 0; i < 256; i++) {
                        #if defined(DEBUG)
      if (MESA_VERBOSE != 0) {
            #endif
         /* Take a glsl type reference for the duration of libGL's life to avoid
   * unecessary creation/destruction of glsl types.
   */
               }
      /**
   * Calls all the various one-time-init functions in Mesa.
   *
   * While holding a global mutex lock, calls several initialization functions,
   * and sets the glapi callbacks if the \c MESA_DEBUG environment variable is
   * defined.
   */
   void
   _mesa_initialize(const char *extensions_override)
   {
      static util_once_flag once = UTIL_ONCE_FLAG_INIT;
   util_call_once_data(&once,
      }
         /**
   * Initialize fields of gl_current_attrib (aka ctx->Current.*)
   */
   static void
   _mesa_init_current(struct gl_context *ctx)
   {
               /* Init all to (0,0,0,1) */
   for (i = 0; i < ARRAY_SIZE(ctx->Current.Attrib); i++) {
                  /* redo special cases: */
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_NORMAL], 0.0, 0.0, 1.0, 1.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_COLOR0], 1.0, 1.0, 1.0, 1.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_COLOR1], 0.0, 0.0, 0.0, 1.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_COLOR_INDEX], 1.0, 0.0, 0.0, 1.0 );
      }
         /**
   * Init vertex/fragment/geometry program limits.
   * Important: drivers should override these with actual limits.
   */
   static void
   init_program_limits(struct gl_constants *consts, gl_shader_stage stage,
         {
      prog->MaxInstructions = MAX_PROGRAM_INSTRUCTIONS;
   prog->MaxAluInstructions = MAX_PROGRAM_INSTRUCTIONS;
   prog->MaxTexInstructions = MAX_PROGRAM_INSTRUCTIONS;
   prog->MaxTexIndirections = MAX_PROGRAM_INSTRUCTIONS;
   prog->MaxTemps = MAX_PROGRAM_TEMPS;
   prog->MaxEnvParams = MAX_PROGRAM_ENV_PARAMS;
   prog->MaxLocalParams = MAX_PROGRAM_LOCAL_PARAMS;
            switch (stage) {
   case MESA_SHADER_VERTEX:
      prog->MaxParameters = MAX_VERTEX_PROGRAM_PARAMS;
   prog->MaxAttribs = MAX_VERTEX_GENERIC_ATTRIBS;
   prog->MaxAddressRegs = MAX_VERTEX_PROGRAM_ADDRESS_REGS;
   prog->MaxUniformComponents = 4 * MAX_UNIFORMS;
   prog->MaxInputComponents = 0; /* value not used */
   prog->MaxOutputComponents = 16 * 4; /* old limit not to break tnl and swrast */
      case MESA_SHADER_FRAGMENT:
      prog->MaxParameters = MAX_FRAGMENT_PROGRAM_PARAMS;
   prog->MaxAttribs = MAX_FRAGMENT_PROGRAM_INPUTS;
   prog->MaxAddressRegs = MAX_FRAGMENT_PROGRAM_ADDRESS_REGS;
   prog->MaxUniformComponents = 4 * MAX_UNIFORMS;
   prog->MaxInputComponents = 16 * 4; /* old limit not to break tnl and swrast */
   prog->MaxOutputComponents = 0; /* value not used */
      case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
   case MESA_SHADER_GEOMETRY:
      prog->MaxParameters = MAX_VERTEX_PROGRAM_PARAMS;
   prog->MaxAttribs = MAX_VERTEX_GENERIC_ATTRIBS;
   prog->MaxAddressRegs = MAX_VERTEX_PROGRAM_ADDRESS_REGS;
   prog->MaxUniformComponents = 4 * MAX_UNIFORMS;
   prog->MaxInputComponents = 16 * 4; /* old limit not to break tnl and swrast */
   prog->MaxOutputComponents = 16 * 4; /* old limit not to break tnl and swrast */
      case MESA_SHADER_COMPUTE:
      prog->MaxParameters = 0; /* not meaningful for compute shaders */
   prog->MaxAttribs = 0; /* not meaningful for compute shaders */
   prog->MaxAddressRegs = 0; /* not meaningful for compute shaders */
   prog->MaxUniformComponents = 4 * MAX_UNIFORMS;
   prog->MaxInputComponents = 0; /* not meaningful for compute shaders */
   prog->MaxOutputComponents = 0; /* not meaningful for compute shaders */
      default:
                  /* Set the native limits to zero.  This implies that there is no native
   * support for shaders.  Let the drivers fill in the actual values.
   */
   prog->MaxNativeInstructions = 0;
   prog->MaxNativeAluInstructions = 0;
   prog->MaxNativeTexInstructions = 0;
   prog->MaxNativeTexIndirections = 0;
   prog->MaxNativeAttribs = 0;
   prog->MaxNativeTemps = 0;
   prog->MaxNativeAddressRegs = 0;
            /* Set GLSL datatype range/precision info assuming IEEE float values.
   * Drivers should override these defaults as needed.
   */
   prog->MediumFloat.RangeMin = 127;
   prog->MediumFloat.RangeMax = 127;
   prog->MediumFloat.Precision = 23;
            /* Assume ints are stored as floats for now, since this is the least-common
   * denominator.  The OpenGL ES spec implies (page 132) that the precision
   * of integer types should be 0.  Practically speaking, IEEE
   * single-precision floating point values can only store integers in the
   * range [-0x01000000, 0x01000000] without loss of precision.
   */
   prog->MediumInt.RangeMin = 24;
   prog->MediumInt.RangeMax = 24;
   prog->MediumInt.Precision = 0;
            prog->MaxUniformBlocks = 12;
   prog->MaxCombinedUniformComponents = (prog->MaxUniformComponents +
                  prog->MaxAtomicBuffers = 0;
               }
         /**
   * Initialize fields of gl_constants (aka ctx->Const.*).
   * Use defaults from config.h.  The device drivers will often override
   * some of these values (such as number of texture units).
   */
   void
   _mesa_init_constants(struct gl_constants *consts, gl_api api)
   {
      int i;
            /* Constants, may be overriden (usually only reduced) by device drivers */
   consts->MaxTextureMbytes = MAX_TEXTURE_MBYTES;
   consts->MaxTextureSize = 1 << (MAX_TEXTURE_LEVELS - 1);
   consts->Max3DTextureLevels = MAX_TEXTURE_LEVELS;
   consts->MaxCubeTextureLevels = MAX_TEXTURE_LEVELS;
   consts->MaxTextureRectSize = MAX_TEXTURE_RECT_SIZE;
   consts->MaxArrayTextureLayers = MAX_ARRAY_TEXTURE_LAYERS;
   consts->MaxTextureCoordUnits = MAX_TEXTURE_COORD_UNITS;
   consts->Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
   consts->MaxTextureUnits = MIN2(consts->MaxTextureCoordUnits,
         consts->MaxTextureMaxAnisotropy = MAX_TEXTURE_MAX_ANISOTROPY;
   consts->MaxTextureLodBias = MAX_TEXTURE_LOD_BIAS;
   consts->MaxTextureBufferSize = 65536;
   consts->TextureBufferOffsetAlignment = 1;
   consts->MaxArrayLockSize = MAX_ARRAY_LOCK_SIZE;
   consts->SubPixelBits = SUB_PIXEL_BITS;
   consts->MinPointSize = MIN_POINT_SIZE;
   consts->MaxPointSize = MAX_POINT_SIZE;
   consts->MinPointSizeAA = MIN_POINT_SIZE;
   consts->MaxPointSizeAA = MAX_POINT_SIZE;
   consts->PointSizeGranularity = (GLfloat) POINT_SIZE_GRANULARITY;
   consts->MinLineWidth = MIN_LINE_WIDTH;
   consts->MaxLineWidth = MAX_LINE_WIDTH;
   consts->MinLineWidthAA = MIN_LINE_WIDTH;
   consts->MaxLineWidthAA = MAX_LINE_WIDTH;
   consts->LineWidthGranularity = (GLfloat) LINE_WIDTH_GRANULARITY;
   consts->MaxClipPlanes = 6;
   consts->MaxLights = MAX_LIGHTS;
   consts->MaxShininess = 128.0;
   consts->MaxSpotExponent = 128.0;
   consts->MaxViewportWidth = 16384;
   consts->MaxViewportHeight = 16384;
            /* Driver must override these values if ARB_viewport_array is supported. */
   consts->MaxViewports = 1;
   consts->ViewportSubpixelBits = 0;
   consts->ViewportBounds.Min = 0;
            /** GL_ARB_uniform_buffer_object */
   consts->MaxCombinedUniformBlocks = 36;
   consts->MaxUniformBufferBindings = 36;
   consts->MaxUniformBlockSize = 16384;
            /** GL_ARB_shader_storage_buffer_object */
   consts->MaxCombinedShaderStorageBlocks = 8;
   consts->MaxShaderStorageBufferBindings = 8;
   consts->MaxShaderStorageBlockSize = 128 * 1024 * 1024; /* 2^27 */
            /* GL_ARB_explicit_uniform_location, GL_MAX_UNIFORM_LOCATIONS */
   consts->MaxUserAssignableUniformLocations =
            for (i = 0; i < MESA_SHADER_STAGES; i++)
            consts->MaxProgramMatrices = MAX_PROGRAM_MATRICES;
            /* Set the absolute minimum possible GLSL version.  API_OPENGL_CORE can
   * mean an OpenGL 3.0 forward-compatible context, so that implies a minimum
   * possible version of 1.30.  Otherwise, the minimum possible version 1.20.
   * Since Mesa unconditionally advertises GL_ARB_shading_language_100 and
   * GL_ARB_shader_objects, every driver has GLSL 1.20... even if they don't
   * advertise any extensions to enable any shader stages (e.g.,
   * GL_ARB_vertex_shader).
   */
   consts->GLSLVersion = api == API_OPENGL_CORE ? 130 : 120;
                     /* GL_ARB_draw_buffers */
            consts->MaxColorAttachments = MAX_COLOR_ATTACHMENTS;
            consts->Program[MESA_SHADER_VERTEX].MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
   consts->MaxCombinedTextureImageUnits = MAX_COMBINED_TEXTURE_IMAGE_UNITS;
   consts->MaxVarying = 16; /* old limit not to break tnl and swrast */
   consts->Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
   consts->MaxGeometryOutputVertices = MAX_GEOMETRY_OUTPUT_VERTICES;
   consts->MaxGeometryTotalOutputComponents = MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS;
         #ifdef DEBUG
         #else
         #endif
         /* GL_ARB_framebuffer_object */
            /* GLSL default if NativeIntegers == FALSE */
            /* GL_ARB_sync */
            /* GL_EXT_provoking_vertex */
            /** GL_ARB_viewport_array */
            /* GL_EXT_transform_feedback */
   consts->MaxTransformFeedbackBuffers = MAX_FEEDBACK_BUFFERS;
   consts->MaxTransformFeedbackSeparateComponents = 4 * MAX_FEEDBACK_ATTRIBS;
   consts->MaxTransformFeedbackInterleavedComponents = 4 * MAX_FEEDBACK_ATTRIBS;
            /* GL 3.2  */
   consts->ProfileMask = api == API_OPENGL_CORE
                  /* GL 4.4 */
            /** GL_EXT_gpu_shader4 */
   consts->MinProgramTexelOffset = -8;
            /* GL_ARB_texture_gather */
   consts->MinProgramTextureGatherOffset = -8;
            /* GL_ARB_robustness */
            /* GL_KHR_robustness */
            /* ES 3.0 or ARB_ES3_compatibility */
            /* GL_ARB_texture_multisample */
   consts->MaxColorTextureSamples = 1;
   consts->MaxDepthTextureSamples = 1;
            /* GL_ARB_shader_atomic_counters */
   consts->MaxAtomicBufferBindings = MAX_COMBINED_ATOMIC_BUFFERS;
   consts->MaxAtomicBufferSize = MAX_ATOMIC_COUNTERS * ATOMIC_COUNTER_SIZE;
   consts->MaxCombinedAtomicBuffers = MAX_COMBINED_ATOMIC_BUFFERS;
            /* GL_ARB_vertex_attrib_binding */
   consts->MaxVertexAttribRelativeOffset = 2047;
            /* GL_ARB_compute_shader */
   consts->MaxComputeWorkGroupCount[0] = 65535;
   consts->MaxComputeWorkGroupCount[1] = 65535;
   consts->MaxComputeWorkGroupCount[2] = 65535;
   consts->MaxComputeWorkGroupSize[0] = 1024;
   consts->MaxComputeWorkGroupSize[1] = 1024;
   consts->MaxComputeWorkGroupSize[2] = 64;
   /* Enables compute support for GLES 3.1 if >= 128 */
            /** GL_ARB_gpu_shader5 */
   consts->MinFragmentInterpolationOffset = MIN_FRAGMENT_INTERPOLATION_OFFSET;
            /** GL_KHR_context_flush_control */
            /** GL_ARB_tessellation_shader */
   consts->MaxTessGenLevel = MAX_TESS_GEN_LEVEL;
   consts->MaxPatchVertices = MAX_PATCH_VERTICES;
   consts->Program[MESA_SHADER_TESS_CTRL].MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
   consts->Program[MESA_SHADER_TESS_EVAL].MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
   consts->MaxTessPatchComponents = MAX_TESS_PATCH_COMPONENTS;
   consts->MaxTessControlTotalOutputComponents = MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS;
            /** GL_ARB_compute_variable_group_size */
   consts->MaxComputeVariableGroupSize[0] = 512;
   consts->MaxComputeVariableGroupSize[1] = 512;
   consts->MaxComputeVariableGroupSize[2] = 64;
            /** GL_NV_conservative_raster */
            /** GL_NV_conservative_raster_dilate */
   consts->ConservativeRasterDilateRange[0] = 0.0;
   consts->ConservativeRasterDilateRange[1] = 0.0;
               }
         /**
   * Do some sanity checks on the limits/constants for the given context.
   * Only called the first time a context is bound.
   */
   static void
   check_context_limits(struct gl_context *ctx)
   {
               /* check that we don't exceed the size of various bitfields */
   assert(VARYING_SLOT_MAX <=
         assert(VARYING_SLOT_MAX <=
            /* shader-related checks */
   assert(ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxLocalParams <= MAX_PROGRAM_LOCAL_PARAMS);
            /* Texture unit checks */
   assert(ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits > 0);
   assert(ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits <= MAX_TEXTURE_IMAGE_UNITS);
   assert(ctx->Const.MaxTextureCoordUnits > 0);
   assert(ctx->Const.MaxTextureCoordUnits <= MAX_TEXTURE_COORD_UNITS);
   assert(ctx->Const.MaxTextureUnits > 0);
   assert(ctx->Const.MaxTextureUnits <= MAX_TEXTURE_IMAGE_UNITS);
   assert(ctx->Const.MaxTextureUnits <= MAX_TEXTURE_COORD_UNITS);
   assert(ctx->Const.MaxTextureUnits == MIN2(ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits,
         assert(ctx->Const.MaxCombinedTextureImageUnits > 0);
   assert(ctx->Const.MaxCombinedTextureImageUnits <= MAX_COMBINED_TEXTURE_IMAGE_UNITS);
   assert(ctx->Const.MaxTextureCoordUnits <= MAX_COMBINED_TEXTURE_IMAGE_UNITS);
   /* number of coord units cannot be greater than number of image units */
               /* Texture size checks */
   assert(ctx->Const.MaxTextureSize <= (1 << (MAX_TEXTURE_LEVELS - 1)));
   assert(ctx->Const.Max3DTextureLevels <= MAX_TEXTURE_LEVELS);
   assert(ctx->Const.MaxCubeTextureLevels <= MAX_TEXTURE_LEVELS);
            /* Max texture size should be <= max viewport size (render to texture) */
   assert(ctx->Const.MaxTextureSize <= ctx->Const.MaxViewportWidth);
                     /* if this fails, add more enum values to gl_buffer_index */
               }
         /**
   * Initialize the attribute groups in a GL context.
   *
   * \param ctx GL context.
   *
   * Initializes all the attributes, calling the respective <tt>init*</tt>
   * functions for the more complex data structures.
   */
   static GLboolean
   init_attrib_groups(struct gl_context *ctx)
   {
               /* Constants */
            /* Extensions */
            /* Attribute Groups */
   _mesa_init_accum( ctx );
   _mesa_init_attrib( ctx );
   _mesa_init_bbox( ctx );
   _mesa_init_buffer_objects( ctx );
   _mesa_init_color( ctx );
   _mesa_init_conservative_raster( ctx );
   _mesa_init_current( ctx );
   _mesa_init_depth( ctx );
   _mesa_init_debug( ctx );
   _mesa_init_debug_output( ctx );
   _mesa_init_display_list( ctx );
   _mesa_init_eval( ctx );
   _mesa_init_feedback( ctx );
   _mesa_init_fog( ctx );
   _mesa_init_hint( ctx );
   _mesa_init_image_units( ctx );
   _mesa_init_line( ctx );
   _mesa_init_lighting( ctx );
   _mesa_init_matrix( ctx );
   _mesa_init_multisample( ctx );
   _mesa_init_performance_monitors( ctx );
   _mesa_init_performance_queries( ctx );
   _mesa_init_pipeline( ctx );
   _mesa_init_pixel( ctx );
   _mesa_init_pixelstore( ctx );
   _mesa_init_point( ctx );
   _mesa_init_polygon( ctx );
   _mesa_init_varray( ctx ); /* should be before _mesa_init_program */
   _mesa_init_program( ctx );
   _mesa_init_queryobj( ctx );
   _mesa_init_sync( ctx );
   _mesa_init_rastpos( ctx );
   _mesa_init_scissor( ctx );
   _mesa_init_shader_state( ctx );
   _mesa_init_stencil( ctx );
   _mesa_init_transform( ctx );
   _mesa_init_transform_feedback( ctx );
   _mesa_init_viewport( ctx );
            if (!_mesa_init_texture( ctx ))
            /* Miscellaneous */
   ctx->TileRasterOrderIncreasingX = GL_TRUE;
   ctx->TileRasterOrderIncreasingY = GL_TRUE;
   ctx->NewState = _NEW_ALL;
   ctx->NewDriverState = ST_ALL_STATES_MASK;
   ctx->ErrorValue = GL_NO_ERROR;
   ctx->ShareGroupReset = false;
               }
         /**
   * Update default objects in a GL context with respect to shared state.
   *
   * \param ctx GL context.
   *
   * Removes references to old default objects, (texture objects, program
   * objects, etc.) and changes to reference those from the current shared
   * state.
   */
   static GLboolean
   update_default_objects(struct gl_context *ctx)
   {
               _mesa_update_default_objects_program(ctx);
   _mesa_update_default_objects_texture(ctx);
               }
         /* XXX this is temporary and should be removed at some point in the
   * future when there's a reasonable expectation that the libGL library
   * contains the _glapi_new_nop_table() and _glapi_set_nop_handler()
   * functions which were added in Mesa 10.6.
   */
   #if !defined(_WIN32)
   /* Avoid libGL / driver ABI break */
   #define USE_GLAPI_NOP_FEATURES 0
   #else
   #define USE_GLAPI_NOP_FEATURES 1
   #endif
         /**
   * This function is called by the glapi no-op functions.  For each OpenGL
   * function/entrypoint there's a simple no-op function.  These "no-op"
   * functions call this function.
   *
   * If there's a current OpenGL context for the calling thread, we record a
   * GL_INVALID_OPERATION error.  This can happen either because the app's
   * calling an unsupported extension function, or calling an illegal function
   * (such as glClear between glBegin/glEnd).
   *
   * If there's no current OpenGL context for the calling thread, we can
   * print a message to stderr.
   *
   * \param name  the name of the OpenGL function
   */
   #if USE_GLAPI_NOP_FEATURES
   static void
   nop_handler(const char *name)
   {
      GET_CURRENT_CONTEXT(ctx);
   if (ctx) {
            #ifndef NDEBUG
      else if (getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG")) {
      fprintf(stderr,
         "GL User Error: gl%s called without a rendering context\n",
         #endif
   }
   #endif
         /**
   * Special no-op glFlush, see below.
   */
   #if defined(_WIN32)
   static void GLAPIENTRY
   nop_glFlush(void)
   {
         }
   #endif
         #if !USE_GLAPI_NOP_FEATURES
   static int
   generic_nop(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
                  }
   #endif
         static int
   glthread_nop(void)
   {
      /* This writes the error into the glthread command buffer if glthread is
   * enabled.
   */
   CALL_InternalSetError(GET_DISPATCH(), (GL_INVALID_OPERATION));
      }
         /**
   * Create a new API dispatch table in which all entries point to the
   * generic_nop() function.  This will not work on Windows because of
   * the __stdcall convention which requires the callee to clean up the
   * call stack.  That's impossible with one generic no-op function.
   */
   struct _glapi_table *
   _mesa_new_nop_table(unsigned numEntries, bool glthread)
   {
            #if !USE_GLAPI_NOP_FEATURES
      table = malloc(numEntries * sizeof(_glapi_proc));
   if (table) {
      _glapi_proc *entry = (_glapi_proc *) table;
   unsigned i;
   for (i = 0; i < numEntries; i++) {
               #else
         #endif
         if (glthread) {
      _glapi_proc *entry = (_glapi_proc *) table;
   for (unsigned i = 0; i < numEntries; i++)
                  }
         /**
   * Allocate and initialize a new dispatch table.  The table will be
   * populated with pointers to "no-op" functions.  In turn, the no-op
   * functions will call nop_handler() above.
   */
   struct _glapi_table *
   _mesa_alloc_dispatch_table(bool glthread)
   {
      /* Find the larger of Mesa's dispatch table and libGL's dispatch table.
   * In practice, this'll be the same for stand-alone Mesa.  But for DRI
   * Mesa we do this to accommodate different versions of libGL and various
   * DRI drivers.
   */
                  #if defined(_WIN32)
      if (table) {
      /* This is a special case for Windows in the event that
   * wglGetProcAddress is called between glBegin/End().
   *
   * The MS opengl32.dll library apparently calls glFlush from
   * wglGetProcAddress().  If we're inside glBegin/End(), glFlush
   * will dispatch to _mesa_generic_nop() and we'll generate a
   * GL_INVALID_OPERATION error.
   *
   * The specific case which hits this is piglit's primitive-restart
   * test which calls glPrimitiveRestartNV() inside glBegin/End.  The
   * first time we call glPrimitiveRestartNV() Piglit's API dispatch
   * code will try to resolve the function by calling wglGetProcAddress.
   * This raises GL_INVALID_OPERATION and an assert(glGetError()==0)
   * will fail causing the test to fail.  By suppressing the error, the
   * assertion passes and the test continues.
   */
         #endif
      #if USE_GLAPI_NOP_FEATURES
         #endif
            }
      /**
   * Allocate dispatch tables and set all functions to nop.
   * It also makes the OutsideBeginEnd dispatch table current within gl_dispatch.
   *
   * \param glthread Whether to set nop dispatch for glthread or regular dispatch
   */
   bool
   _mesa_alloc_dispatch_tables(gl_api api, struct gl_dispatch *d, bool glthread)
   {
      d->OutsideBeginEnd = _mesa_alloc_dispatch_table(glthread);
   if (!d->OutsideBeginEnd)
            if (api == API_OPENGL_COMPAT) {
      d->BeginEnd = _mesa_alloc_dispatch_table(glthread);
   d->Save = _mesa_alloc_dispatch_table(glthread);
   if (!d->BeginEnd || !d->Save)
               d->Current = d->Exec = d->OutsideBeginEnd;
      }
      static void
   _mesa_free_dispatch_tables(struct gl_dispatch *d)
   {
      free(d->OutsideBeginEnd);
   free(d->BeginEnd);
   free(d->HWSelectModeBeginEnd);
   free(d->Save);
      }
      bool
   _mesa_initialize_dispatch_tables(struct gl_context *ctx)
   {
      if (!_mesa_alloc_dispatch_tables(ctx->API, &ctx->Dispatch, false))
            /* Do the code-generated initialization of dispatch tables. */
   _mesa_init_dispatch(ctx);
            if (_mesa_is_desktop_gl_compat(ctx)) {
      _mesa_init_dispatch_save(ctx);
               /* This binds the dispatch table to the context, but MakeCurrent will
   * bind it for the user. If glthread is enabled, it will override it.
   */
   ctx->GLApi = ctx->Dispatch.Current;
      }
      /**
   * Initialize a struct gl_context struct (rendering context).
   *
   * This includes allocating all the other structs and arrays which hang off of
   * the context by pointers.
   * Note that the driver needs to pass in its dd_function_table here since
   * we need to at least call st_NewTextureObject to create the
   * default texture objects.
   *
   * Called by _mesa_create_context().
   *
   * Performs the imports and exports callback tables initialization, and
   * miscellaneous one-time initializations. If no shared context is supplied one
   * is allocated, and increase its reference count.  Setups the GL API dispatch
   * tables.  Initialize the TNL module. Sets the maximum Z buffer depth.
   * Finally queries the \c MESA_DEBUG and \c MESA_VERBOSE environment variables
   * for debug flags.
   *
   * \param ctx the context to initialize
   * \param api the GL API type to create the context for
   * \param visual describes the visual attributes for this context or NULL to
   *               create a configless context
   * \param share_list points to context to share textures, display lists,
   *        etc with, or NULL
   * \param driverFunctions table of device driver functions for this context
   *        to use
   */
   GLboolean
   _mesa_initialize_context(struct gl_context *ctx,
                           gl_api api,
   {
      struct gl_shared_state *shared;
            switch (api) {
   case API_OPENGL_COMPAT:
   case API_OPENGL_CORE:
      if (!HAVE_OPENGL)
            case API_OPENGLES2:
      if (!HAVE_OPENGL_ES_2)
            case API_OPENGLES:
      if (!HAVE_OPENGL_ES_1)
            default:
                  ctx->API = api;
   ctx->DrawBuffer = NULL;
   ctx->ReadBuffer = NULL;
   ctx->WinSysDrawBuffer = NULL;
            if (visual) {
      ctx->Visual = *visual;
      }
   else {
      memset(&ctx->Visual, 0, sizeof ctx->Visual);
                        /* misc one-time initializations */
            /* Plug in driver functions and context pointer here.
   * This is important because when we call alloc_shared_state() below
   * we'll call ctx->Driver.NewTextureObject() to create the default
   * textures.
   */
            if (share_list) {
      /* share state with another context */
      }
   else {
      /* allocate new, unshared state */
   shared = _mesa_alloc_shared_state(ctx);
   if (!shared)
               /* all supported by default */
                     if (!init_attrib_groups( ctx ))
            if (no_error)
                     /* Mesa core handles all the formats that mesa core knows about.
   * Drivers will want to override this list with just the formats
   * they can handle.
   */
   memset(&ctx->TextureFormatSupported, GL_TRUE,
            switch (ctx->API) {
   case API_OPENGL_COMPAT:
   case API_OPENGL_CORE:
   case API_OPENGLES2:
         case API_OPENGLES:
      /**
   * GL_OES_texture_cube_map says
   * "Initially all texture generation modes are set to REFLECTION_MAP_OES"
   */
   for (i = 0; i < ARRAY_SIZE(ctx->Texture.FixedFuncUnit); i++) {
                     texUnit->GenS.Mode = GL_REFLECTION_MAP_NV;
   texUnit->GenT.Mode = GL_REFLECTION_MAP_NV;
   texUnit->GenR.Mode = GL_REFLECTION_MAP_NV;
   texUnit->GenS._ModeBit = TEXGEN_REFLECTION_MAP_NV;
   texUnit->GenT._ModeBit = TEXGEN_REFLECTION_MAP_NV;
      }
      }
   ctx->VertexProgram.PointSizeEnabled = _mesa_is_gles2(ctx);
                           fail:
      _mesa_reference_shared_state(ctx, &ctx->Shared, NULL);
      }
         /**
   * Free the data associated with the given context.
   *
   * But doesn't free the struct gl_context struct itself.
   *
   * \sa _mesa_initialize_context() and init_attrib_groups().
   */
   void
   _mesa_free_context_data(struct gl_context *ctx, bool destroy_debug_output)
   {
      if (!_mesa_get_current_context()){
      /* No current context, but we may need one in order to delete
   * texture objs, etc.  So temporarily bind the context now.
   */
               /* unreference WinSysDraw/Read buffers */
   _mesa_reference_framebuffer(&ctx->WinSysDrawBuffer, NULL);
   _mesa_reference_framebuffer(&ctx->WinSysReadBuffer, NULL);
   _mesa_reference_framebuffer(&ctx->DrawBuffer, NULL);
            _mesa_reference_program(ctx, &ctx->VertexProgram.Current, NULL);
   _mesa_reference_program(ctx, &ctx->VertexProgram._Current, NULL);
            _mesa_reference_program(ctx, &ctx->TessCtrlProgram._Current, NULL);
   _mesa_reference_program(ctx, &ctx->TessEvalProgram._Current, NULL);
            _mesa_reference_program(ctx, &ctx->FragmentProgram.Current, NULL);
   _mesa_reference_program(ctx, &ctx->FragmentProgram._Current, NULL);
                     _mesa_reference_vao(ctx, &ctx->Array.VAO, NULL);
   _mesa_reference_vao(ctx, &ctx->Array.DefaultVAO, NULL);
            _mesa_free_attrib_data(ctx);
   _mesa_free_eval_data( ctx );
   _mesa_free_feedback(ctx);
   _mesa_free_texture_data( ctx );
   _mesa_free_image_textures(ctx);
   _mesa_free_matrix_data( ctx );
   _mesa_free_pipeline_data(ctx);
   _mesa_free_program_data(ctx);
   _mesa_free_shader_state(ctx);
   _mesa_free_queryobj_data(ctx);
   _mesa_free_sync_data(ctx);
   _mesa_free_varray_data(ctx);
   _mesa_free_transform_feedback(ctx);
   _mesa_free_performance_monitors(ctx);
   _mesa_free_performance_queries(ctx);
   _mesa_free_perfomance_monitor_groups(ctx);
            _mesa_reference_buffer_object(ctx, &ctx->Pack.BufferObj, NULL);
   _mesa_reference_buffer_object(ctx, &ctx->Unpack.BufferObj, NULL);
   _mesa_reference_buffer_object(ctx, &ctx->DefaultPacking.BufferObj, NULL);
            /* This must be called after all buffers are unbound because global buffer
   * references that this context holds will be removed.
   */
            /* free dispatch tables */
   _mesa_free_dispatch_tables(&ctx->Dispatch);
            /* Shared context state (display lists, textures, etc) */
            if (destroy_debug_output)
                                       /* unbind the context if it's currently bound */
   if (ctx == _mesa_get_current_context()) {
                  /* Do this after unbinding context to ensure any thread is finished. */
   if (ctx->shader_builtin_ref) {
      _mesa_glsl_builtin_functions_decref();
               free(ctx->Const.SpirVExtensions);
      }
         /**
   * Copy attribute groups from one context to another.
   *
   * \param src source context
   * \param dst destination context
   * \param mask bitwise OR of GL_*_BIT flags
   *
   * According to the bits specified in \p mask, copies the corresponding
   * attributes from \p src into \p dst.  For many of the attributes a simple \c
   * memcpy is not enough due to the existence of internal pointers in their data
   * structures.
   */
   void
   _mesa_copy_context( const struct gl_context *src, struct gl_context *dst,
         {
      if (mask & GL_ACCUM_BUFFER_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_COLOR_BUFFER_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_CURRENT_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_DEPTH_BUFFER_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_ENABLE_BIT) {
         }
   if (mask & GL_EVAL_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_FOG_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_HINT_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_LIGHTING_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_LINE_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_LIST_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_PIXEL_MODE_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_POINT_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_POLYGON_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_POLYGON_STIPPLE_BIT) {
      /* Use loop instead of memcpy due to problem with Portland Group's
   * C compiler.  Reported by John Stone.
   */
   GLuint i;
   for (i = 0; i < 32; i++) {
            }
   if (mask & GL_SCISSOR_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_STENCIL_BUFFER_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_TEXTURE_BIT) {
      /* Cannot memcpy because of pointers */
      }
   if (mask & GL_TRANSFORM_BIT) {
      /* OK to memcpy */
      }
   if (mask & GL_VIEWPORT_BIT) {
      unsigned i;
   for (i = 0; i < src->Const.MaxViewports; i++) {
      /* OK to memcpy */
                  /* XXX FIXME:  Call callbacks?
   */
   dst->NewState = _NEW_ALL;
      }
         /**
   * Check if the given context can render into the given framebuffer
   * by checking visual attributes.
   *
   * \return GL_TRUE if compatible, GL_FALSE otherwise.
   */
   static GLboolean
   check_compatible(const struct gl_context *ctx,
         {
      const struct gl_config *ctxvis = &ctx->Visual;
            if (buffer == _mesa_get_incomplete_framebuffer())
         #define check_component(foo)           \
      if (ctxvis->foo && bufvis->foo &&   \
      ctxvis->foo != bufvis->foo)     \
         check_component(redShift);
   check_component(greenShift);
   check_component(blueShift);
   check_component(redBits);
   check_component(greenBits);
   check_component(blueBits);
   check_component(depthBits);
         #undef check_component
            }
         /**
   * Check if the viewport/scissor size has not yet been initialized.
   * Initialize the size if the given width and height are non-zero.
   */
   static void
   check_init_viewport(struct gl_context *ctx, GLuint width, GLuint height)
   {
      if (!ctx->ViewportInitialized && width > 0 && height > 0) {
               /* Note: set flag here, before calling _mesa_set_viewport(), to prevent
   * potential infinite recursion.
   */
            /* Note: ctx->Const.MaxViewports may not have been set by the driver
   * yet, so just initialize all of them.
   */
   for (i = 0; i < MAX_VIEWPORTS; i++) {
      _mesa_set_viewport(ctx, i, 0, 0, width, height);
            }
         static void
   handle_first_current(struct gl_context *ctx)
   {
      if (ctx->Version == 0 || !ctx->DrawBuffer) {
      /* probably in the process of tearing down the context */
                                 /* According to GL_MESA_configless_context the default value of
   * glDrawBuffers depends on the config of the first surface it is bound to.
   * For GLES it is always GL_BACK which has a magic interpretation.
   */
   if (!ctx->HasConfig && _mesa_is_desktop_gl(ctx)) {
      if (ctx->DrawBuffer != _mesa_get_incomplete_framebuffer()) {
               if (ctx->DrawBuffer->Visual.doubleBufferMode)
                        _mesa_drawbuffers(ctx, ctx->DrawBuffer, 1, &buffer,
               if (ctx->ReadBuffer != _mesa_get_incomplete_framebuffer()) {
                     if (ctx->ReadBuffer->Visual.doubleBufferMode) {
      buffer = GL_BACK;
      }
   else {
      buffer = GL_FRONT;
                              /* Determine if generic vertex attribute 0 aliases the conventional
   * glVertex position.
   */
   {
      const bool is_forward_compatible_context =
            /* In OpenGL 3.1 attribute 0 becomes non-magic, just like in OpenGL ES
   * 2.0.  Note that we cannot just check for API_OPENGL_COMPAT here because
   * that will erroneously allow this usage in a 3.0 forward-compatible
   * context too.
   */
   ctx->_AttribZeroAliasesVertex = (_mesa_is_gles1(ctx)
                     /* We can use this to help debug user's problems.  Tell them to set
   * the MESA_INFO env variable before running their app.  Then the
   * first time each context is made current we'll print some useful
   * information.
   */
   if (getenv("MESA_INFO")) {
            }
      /**
   * Bind the given context to the given drawBuffer and readBuffer and
   * make it the current context for the calling thread.
   * We'll render into the drawBuffer and read pixels from the
   * readBuffer (i.e. glRead/CopyPixels, glCopyTexImage, etc).
   *
   * We check that the context's and framebuffer's visuals are compatible
   * and return immediately if they're not.
   *
   * \param newCtx  the new GL context. If NULL then there will be no current GL
   *                context.
   * \param drawBuffer  the drawing framebuffer
   * \param readBuffer  the reading framebuffer
   */
   GLboolean
   _mesa_make_current( struct gl_context *newCtx,
               {
               if (MESA_VERBOSE & VERBOSE_API)
            /* Check that the context's and framebuffer's visuals are compatible.
   */
   if (newCtx && drawBuffer && newCtx->WinSysDrawBuffer != drawBuffer) {
      if (!check_compatible(newCtx, drawBuffer)) {
      _mesa_warning(newCtx,
               }
   if (newCtx && readBuffer && newCtx->WinSysReadBuffer != readBuffer) {
      if (!check_compatible(newCtx, readBuffer)) {
      _mesa_warning(newCtx,
                        if (curCtx &&
      /* make sure this context is valid for flushing */
   curCtx != newCtx &&
   curCtx->Const.ContextReleaseBehavior ==
   GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH) {
   FLUSH_VERTICES(curCtx, 0, 0);
   if (curCtx->st){
                     if (!newCtx) {
      _glapi_set_dispatch(NULL);  /* none current */
   /* We need old ctx to correctly release Draw/ReadBuffer
   * and avoid a surface leak in st_renderbuffer_delete.
   * Therefore, first drop buffers then set new ctx to NULL.
   */
   if (curCtx) {
      _mesa_reference_framebuffer(&curCtx->WinSysDrawBuffer, NULL);
      }
   _glapi_set_context(NULL);
      }
   else {
      _glapi_set_context((void *) newCtx);
   assert(_mesa_get_current_context() == newCtx);
            if (drawBuffer && readBuffer) {
      assert(_mesa_is_winsys_fbo(drawBuffer));
   assert(_mesa_is_winsys_fbo(readBuffer));
                  /*
   * Only set the context's Draw/ReadBuffer fields if they're NULL
   * or not bound to a user-created FBO.
   */
   if (!newCtx->DrawBuffer || _mesa_is_winsys_fbo(newCtx->DrawBuffer)) {
      _mesa_reference_framebuffer(&newCtx->DrawBuffer, drawBuffer);
   /* Update the FBO's list of drawbuffers/renderbuffers.
   * For winsys FBOs this comes from the GL state (which may have
   * changed since the last time this FBO was bound).
   */
   _mesa_update_draw_buffers(newCtx);
   _mesa_update_allow_draw_out_of_order(newCtx);
      }
   if (!newCtx->ReadBuffer || _mesa_is_winsys_fbo(newCtx->ReadBuffer)) {
      _mesa_reference_framebuffer(&newCtx->ReadBuffer, readBuffer);
   /* In _mesa_initialize_window_framebuffer, for single-buffered
   * visuals, the ColorReadBuffer is set to be GL_FRONT, even with
   * GLES contexts. When calling read_buffer, we verify we are reading
   * from GL_BACK in is_legal_es3_readbuffer_enum.  But the default is
   * incorrect, and certain dEQP tests check this.  So fix it here.
   */
   if (_mesa_is_gles(newCtx) &&
      !newCtx->ReadBuffer->Visual.doubleBufferMode)
                  /* XXX only set this flag if we're really changing the draw/read
   * framebuffer bindings.
                              if (newCtx->FirstTimeCurrent) {
      handle_first_current(newCtx);
                     }
         /**
   * Make context 'ctx' share the display lists, textures and programs
   * that are associated with 'ctxToShare'.
   * Any display lists, textures or programs associated with 'ctx' will
   * be deleted if nobody else is sharing them.
   */
   GLboolean
   _mesa_share_state(struct gl_context *ctx, struct gl_context *ctxToShare)
   {
      if (ctx && ctxToShare && ctx->Shared && ctxToShare->Shared) {
               /* save ref to old state to prevent it from being deleted immediately */
            /* update ctx's Shared pointer */
                     /* release the old shared state */
               }
   else {
            }
            /**
   * \return pointer to the current GL context for this thread.
   *
   * Calls _glapi_get_context(). This isn't the fastest way to get the current
   * context.  If you need speed, see the #GET_CURRENT_CONTEXT macro in
   * context.h.
   */
   struct gl_context *
   _mesa_get_current_context( void )
   {
         }
      /*@}*/
         /**********************************************************************/
   /** \name Miscellaneous functions                                     */
   /**********************************************************************/
   /*@{*/
   /**
   * Flush commands.
   */
   void
   _mesa_flush(struct gl_context *ctx)
   {
      bool async = !ctx->Shared->HasExternallySharedImages;
               }
            /**
   * Flush commands and wait for completion.
   *
   * Calls the #ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH macro and the
   * dd_function_table::Finish driver callback, if not NULL.
   */
   void GLAPIENTRY
   _mesa_Finish(void)
   {
      GET_CURRENT_CONTEXT(ctx);
                        }
         /**
   * Execute glFlush().
   *
   * Calls the #ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH macro and the
   * dd_function_table::Flush driver callback, if not NULL.
   */
   void GLAPIENTRY
   _mesa_Flush(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);
      }
         /*@}*/
