   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2022 Roman Stratiienko (r.stratiienko@gmail.com)
   * SPDX-License-Identifier: MIT
   */
      #include "u_gralloc_internal.h"
      #include <assert.h>
   #include <errno.h>
      #include "drm-uapi/drm_fourcc.h"
   #include "util/log.h"
   #include "util/macros.h"
   #include "util/simple_mtx.h"
   #include "util/u_atomic.h"
   #include "util/u_memory.h"
      static simple_mtx_t u_gralloc_mutex = SIMPLE_MTX_INITIALIZER;
      static const struct u_grallocs {
      enum u_gralloc_type type;
      } u_grallocs[] = {
      /* Prefer the CrOS API as it is significantly faster than IMapper4 */
      #ifdef USE_IMAPPER4_METADATA_API
         #endif /* USE_IMAPPER4_METADATA_API */
         };
      static struct u_gralloc_cache {
      struct u_gralloc *u_gralloc;
      } u_gralloc_cache[U_GRALLOC_TYPE_COUNT] = {0};
      struct u_gralloc *
   u_gralloc_create(enum u_gralloc_type type)
   {
                        if (u_gralloc_cache[type].u_gralloc != NULL) {
      u_gralloc_cache[type].refcount++;
   out_gralloc = u_gralloc_cache[type].u_gralloc;
               for (int i = 0; i < ARRAY_SIZE(u_grallocs); i++) {
      if (u_grallocs[i].type != type && type != U_GRALLOC_TYPE_AUTO)
            u_gralloc_cache[type].u_gralloc = u_grallocs[i].create();
   if (u_gralloc_cache[type].u_gralloc) {
                                    out_gralloc = u_gralloc_cache[type].u_gralloc;
               out:
                  }
      void
   u_gralloc_destroy(struct u_gralloc **gralloc)
   {
               if (*gralloc == NULL)
                     for (i = 0; i < ARRAY_SIZE(u_gralloc_cache); i++) {
      if (u_gralloc_cache[i].u_gralloc == *gralloc) {
      u_gralloc_cache[i].refcount--;
   if (u_gralloc_cache[i].refcount == 0) {
      u_gralloc_cache[i].u_gralloc->ops.destroy(
            }
                                       }
      int
   u_gralloc_get_buffer_basic_info(struct u_gralloc *gralloc,
               {
      struct u_gralloc_buffer_basic_info info = {0};
                     if (ret)
                        }
      int
   u_gralloc_get_buffer_color_info(struct u_gralloc *gralloc,
               {
      struct u_gralloc_buffer_color_info info = {0};
            if (!gralloc->ops.get_buffer_color_info)
                     if (ret)
                        }
      int
   u_gralloc_get_front_rendering_usage(struct u_gralloc *gralloc,
         {
      if (!gralloc->ops.get_front_rendering_usage)
               }
      int
   u_gralloc_get_type(struct u_gralloc *gralloc)
   {
         }
