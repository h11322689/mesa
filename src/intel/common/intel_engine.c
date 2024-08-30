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
      #include <stdlib.h>
      #include "util/macros.h"
      #include "intel_engine.h"
   #include "i915/intel_engine.h"
   #include "xe/intel_engine.h"
      struct intel_query_engine_info *
   intel_engine_get_info(int fd, enum intel_kmd_type type)
   {
      switch (type) {
   case INTEL_KMD_TYPE_I915:
         case INTEL_KMD_TYPE_XE:
         default:
      unreachable("Missing");
         }
      int
   intel_engines_count(const struct intel_query_engine_info *info,
         {
               for (int i = 0; i < info->num_engines; i++) {
      if (info->engines[i].engine_class == engine_class)
                  }
      const char *
   intel_engines_class_to_string(enum intel_engine_class engine_class)
   {
      switch (engine_class) {
   case INTEL_ENGINE_CLASS_RENDER:
         case INTEL_ENGINE_CLASS_COPY:
         case INTEL_ENGINE_CLASS_VIDEO:
         case INTEL_ENGINE_CLASS_VIDEO_ENHANCE:
         case INTEL_ENGINE_CLASS_COMPUTE:
         default:
            }
