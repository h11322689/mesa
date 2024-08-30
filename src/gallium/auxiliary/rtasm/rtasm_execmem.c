   /**************************************************************************
   *
   * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
   *
   **************************************************************************/
         /**
   * \file exemem.c
   * Functions for allocating executable memory.
   *
   * \author Keith Whitwell
   */
         #include "util/compiler.h"
   #include "util/simple_mtx.h"
   #include "util/u_debug.h"
   #include "util/u_thread.h"
   #include "util/u_memory.h"
      #include "rtasm_execmem.h"
      #ifndef MAP_ANONYMOUS
   #define MAP_ANONYMOUS MAP_ANON
   #endif
      #if DETECT_OS_WINDOWS
   #include <windows.h>
   #endif
      #if DETECT_OS_UNIX
         /*
   * Allocate a large block of memory which can hold code then dole it out
   * in pieces by means of the generic memory manager code.
   */
      #include <unistd.h>
   #include <sys/mman.h>
   #include "util/u_mm.h"
      #define EXEC_HEAP_SIZE (10*1024*1024)
      static simple_mtx_t exec_mutex = SIMPLE_MTX_INITIALIZER;
      static struct mem_block *exec_heap = NULL;
   static unsigned char *exec_mem = NULL;
         static int
   init_heap(void)
   {
      if (!exec_heap)
            if (!exec_mem)
      exec_mem = (unsigned char *) mmap(NULL, EXEC_HEAP_SIZE,
                  }
         void *
   rtasm_exec_malloc(size_t size)
   {
      struct mem_block *block = NULL;
                     if (!init_heap())
            if (exec_heap) {
      size = (size + 31) & ~31;  /* next multiple of 32 bytes */
               if (block)
         else
         bail:
                  }
         void
   rtasm_exec_free(void *addr)
   {
               if (exec_heap) {
               if (block)
                  }
         #elif DETECT_OS_WINDOWS
         /*
   * Avoid Data Execution Prevention.
   */
      void *
   rtasm_exec_malloc(size_t size)
   {
         }
         void
   rtasm_exec_free(void *addr)
   {
         }
         #else
         /*
   * Just use regular memory.
   */
      void *
   rtasm_exec_malloc(size_t size)
   {
         }
         void
   rtasm_exec_free(void *addr)
   {
         }
         #endif
