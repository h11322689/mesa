   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
         #include "util/u_pointer.h"
   #include "util/u_hash_table.h"
      #if DETECT_OS_UNIX
   #include <sys/stat.h>
   #endif
         static uint32_t
   pointer_hash(const void *key)
   {
         }
         static bool
   pointer_equal(const void *a, const void *b)
   {
         }
         struct hash_table *
   util_hash_table_create_ptr_keys(void)
   {
         }
         static uint32_t hash_fd(const void *key)
   {
   #if DETECT_OS_UNIX
      int fd = pointer_to_intptr(key);
                        #else
         #endif
   }
         static bool equal_fd(const void *key1, const void *key2)
   {
   #if DETECT_OS_UNIX
      int fd1 = pointer_to_intptr(key1);
   int fd2 = pointer_to_intptr(key2);
            fstat(fd1, &stat1);
            return stat1.st_dev == stat2.st_dev &&
            #else
         #endif
   }
         struct hash_table *
   util_hash_table_create_fd_keys(void)
   {
         }
         void *
   util_hash_table_get(struct hash_table *ht,
         {
                  }
         int
   util_hash_table_foreach(struct hash_table *ht,
                     {
      hash_table_foreach(ht, entry) {
      int error = callback((void*)entry->key, entry->data, data);
   if (error != 0)
      }
      }
