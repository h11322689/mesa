   /*
   * Copyright 2022 Yonggang Luo
   * SPDX-License-Identifier: MIT
   *
   */
      #include "simple_mtx.h"
      #if !UTIL_FUTEX_SUPPORTED
      void _simple_mtx_plain_init_once(simple_mtx_t *mtx)
   {
         }
      void
   simple_mtx_init(simple_mtx_t *mtx, ASSERTED int type)
   {
      const util_once_flag flag = UTIL_ONCE_FLAG_INIT;
   assert(type == mtx_plain);
   mtx->flag = flag;
      }
      void
   simple_mtx_destroy(simple_mtx_t *mtx)
   {
      if (mtx->flag.called) {
            }
      #endif /* !UTIL_FUTEX_SUPPORTED */
