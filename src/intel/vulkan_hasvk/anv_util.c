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
   anv_dump_pipe_bits(enum anv_pipe_bits bits)
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
         if (bits & ANV_PIPE_CS_STALL_BIT)
         if (bits & ANV_PIPE_UNTYPED_DATAPORT_CACHE_FLUSH_BIT)
      }
