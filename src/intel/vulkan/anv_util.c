   /*
   * Copyright © 2015 Intel Corporation
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
      #include <stdarg.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <errno.h>
   #include <assert.h>
      #include "anv_private.h"
   #include "vk_enum_to_str.h"
      void
   __anv_perf_warn(struct anv_device *device,
               {
      va_list ap;
            va_start(ap, format);
   vsnprintf(buffer, sizeof(buffer), format, ap);
            if (object) {
      __vk_log(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
   } else {
      __vk_log(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      }
      void
   anv_dump_pipe_bits(enum anv_pipe_bits bits, FILE *f)
   {
      if (bits & ANV_PIPE_DEPTH_CACHE_FLUSH_BIT)
         if (bits & ANV_PIPE_DATA_CACHE_FLUSH_BIT)
         if (bits & ANV_PIPE_HDC_PIPELINE_FLUSH_BIT)
         if (bits & ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT)
         if (bits & ANV_PIPE_TILE_CACHE_FLUSH_BIT)
         if (bits & ANV_PIPE_STATE_CACHE_INVALIDATE_BIT)
         if (bits & ANV_PIPE_CONSTANT_CACHE_INVALIDATE_BIT)
         if (bits & ANV_PIPE_VF_CACHE_INVALIDATE_BIT)
         if (bits & ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT)
         if (bits & ANV_PIPE_INSTRUCTION_CACHE_INVALIDATE_BIT)
         if (bits & ANV_PIPE_STALL_AT_SCOREBOARD_BIT)
         if (bits & ANV_PIPE_PSS_STALL_SYNC_BIT)
         if (bits & ANV_PIPE_DEPTH_STALL_BIT)
         if (bits & ANV_PIPE_CS_STALL_BIT ||
      bits & ANV_PIPE_END_OF_PIPE_SYNC_BIT)
      if (bits & ANV_PIPE_UNTYPED_DATAPORT_CACHE_FLUSH_BIT)
         if (bits & ANV_PIPE_CCS_CACHE_FLUSH_BIT)
      }
      const char *
   anv_gfx_state_bit_to_str(enum anv_gfx_state_bits state)
   {
   #define NAME(name) case ANV_GFX_STATE_##name: return #name;
      switch (state) {
      NAME(URB);
   NAME(VF_STATISTICS);
   NAME(VF_SGVS);
   NAME(VF_SGVS_2);
   NAME(VF_SGVS_INSTANCING);
   NAME(PRIMITIVE_REPLICATION);
   NAME(MULTISAMPLE);
   NAME(SBE);
   NAME(SBE_SWIZ);
   NAME(SO_DECL_LIST);
   NAME(VS);
   NAME(HS);
   NAME(DS);
   NAME(GS);
   NAME(PS);
   NAME(PS_EXTRA);
   NAME(SBE_MESH);
   NAME(CLIP_MESH);
   NAME(MESH_CONTROL);
   NAME(MESH_SHADER);
   NAME(MESH_DISTRIB);
   NAME(TASK_CONTROL);
   NAME(TASK_SHADER);
   NAME(TASK_REDISTRIB);
   NAME(BLEND_STATE_POINTERS);
   NAME(CLIP);
   NAME(CC_STATE);
   NAME(CPS);
   NAME(DEPTH_BOUNDS);
   NAME(INDEX_BUFFER);
   NAME(LINE_STIPPLE);
   NAME(PS_BLEND);
   NAME(RASTER);
   NAME(SAMPLE_MASK);
   NAME(SAMPLE_PATTERN);
   NAME(SCISSOR);
   NAME(SF);
   NAME(STREAMOUT);
   NAME(TE);
   NAME(VERTEX_INPUT);
   NAME(VF);
   NAME(VF_TOPOLOGY);
   NAME(VFG);
   NAME(VIEWPORT_CC);
   NAME(VIEWPORT_SF_CLIP);
   NAME(WM);
   NAME(WM_DEPTH_STENCIL);
      default: unreachable("invalid state");
      }
