   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2010 LunarG Inc.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Chia-I Wu <olv@lunarg.com>
   */
      #include <stdlib.h>
   #include <string.h>
   #include <assert.h>
   #include "c11/threads.h"
      #include "util/macros.h"
   #include "util/simple_mtx.h"
   #include "u_current.h"
   #include "entry.h"
   #include "stub.h"
   #include "table.h"
         struct mapi_stub {
      size_t name_offset;
      };
      /* define public_string_pool and public_stubs */
   #define MAPI_TMP_PUBLIC_STUBS
   #include "mapi_tmp.h"
      void
   stub_init_once(void)
   {
      static once_flag flag = ONCE_FLAG_INIT;
      }
      static int
   stub_compare(const void *key, const void *elem)
   {
      const char *name = (const char *) key;
   const struct mapi_stub *stub = (const struct mapi_stub *) elem;
                        }
      /**
   * Return the public stub with the given name.
   */
   const struct mapi_stub *
   stub_find_public(const char *name)
   {
      return (const struct mapi_stub *) bsearch(name, public_stubs,
      }
         static const struct mapi_stub *
   search_table_by_slot(const struct mapi_stub *table, size_t num_entries,
         {
      size_t i;
   for (i = 0; i < num_entries; ++i) {
      if (table[i].slot == slot)
      }
      }
      const struct mapi_stub *
   stub_find_by_slot(int slot)
   {
      const struct mapi_stub *stub =
            }
      /**
   * Return the name of a stub.
   */
   const char *
   stub_get_name(const struct mapi_stub *stub)
   {
         }
      /**
   * Return the slot of a stub.
   */
   int
   stub_get_slot(const struct mapi_stub *stub)
   {
         }
      /**
   * Return the address of a stub.
   */
   mapi_func
   stub_get_addr(const struct mapi_stub *stub)
   {
         }
