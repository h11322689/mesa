   /*
   * Copyright 2003 VMware, Inc.
   * Copyright © 2006 Intel Corporation
   * Copyright © 2017 Broadcom
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
   */
      /**
   * \file v3d_debug.c
   *
   * Support for the V3D_DEBUG environment variable, along with other
   * miscellaneous debugging code.
   */
      #include <stdlib.h>
      #include "common/v3d_debug.h"
   #include "util/macros.h"
   #include "util/u_debug.h"
   #include "c11/threads.h"
      uint32_t v3d_mesa_debug = 0;
      static const struct debug_named_value debug_control[] = {
         { "cl",          V3D_DEBUG_CL,
         { "cl_nobin",    V3D_DEBUG_CL_NO_BIN,
         { "clif",        V3D_DEBUG_CLIF,
         { "qpu",         V3D_DEBUG_QPU,
         { "vir",         V3D_DEBUG_VIR,
         { "nir",         V3D_DEBUG_NIR,
         { "tgsi",        V3D_DEBUG_TGSI,
         /* `shaderdb` is *not* used by shader-db, but is here so that any other
      * game/app can dump its stats in the shader-db format, allowing them
   * to be compared using shader-db's report.py tool.
      { "shaderdb",    V3D_DEBUG_SHADERDB,
         { "surface",     V3D_DEBUG_SURFACE,
      /* FIXME: evaluate to implement it on v3dv */
      { "perf",        V3D_DEBUG_PERF,
         { "norast",      V3D_DEBUG_NORAST,
      /* FIXME: evaluate to implement on v3dv*/
      { "fs",          V3D_DEBUG_FS,
         { "gs",          V3D_DEBUG_GS,
         { "vs",          V3D_DEBUG_VS,
         { "cs",          V3D_DEBUG_CS,
         { "always_flush", V3D_DEBUG_ALWAYS_FLUSH,
         { "precompile",  V3D_DEBUG_PRECOMPILE,
         { "ra",          V3D_DEBUG_RA,
         { "dump_spirv",  V3D_DEBUG_DUMP_SPIRV,
         { "tmu32",  V3D_DEBUG_TMU_32BIT,
         /* This can lead to incorrect behavior for applications that do
      * require full 32-bit precision, but can improve performance
   * for those that don't.
      { "tmu16",  V3D_DEBUG_TMU_16BIT,
         { "noloopunroll",  V3D_DEBUG_NO_LOOP_UNROLL,
         { "db", V3D_DEBUG_DOUBLE_BUFFER,
   #ifdef ENABLE_SHADER_CACHE
         { "cache", V3D_DEBUG_CACHE,
   #endif
         { "no_merge_jobs", V3D_DEBUG_NO_MERGE_JOBS,
         { "opt_compile_time", V3D_DEBUG_OPT_COMPILE_TIME,
         /* disable_tfu is v3dv only because v3d has some uses of the TFU without alternative codepaths */
   { "disable_tfu", V3D_DEBUG_DISABLE_TFU,
         };
      DEBUG_GET_ONCE_FLAGS_OPTION(v3d_debug, "V3D_DEBUG", debug_control, 0)
      bool
   v3d_debug_flag_for_shader_stage(gl_shader_stage stage)
   {
         uint32_t flags[] = {
            [MESA_SHADER_VERTEX] = V3D_DEBUG_VS,
   [MESA_SHADER_TESS_CTRL] = 0,
   [MESA_SHADER_TESS_EVAL] = 0,
   [MESA_SHADER_GEOMETRY] = V3D_DEBUG_GS,
      };
   STATIC_ASSERT(MESA_SHADER_STAGES == 6);
   }
      void
   v3d_process_debug_variable(void)
   {
         }
