   /* -*- mesa-c++  -*-
   *
   * Copyright (c) 2022 Collabora LTD
   *
   * Author: Gert Wollny <gert.wollny@collabora.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "sfn_memorypool.h"
      #include <cassert>
   #include <iostream>
      #ifdef HAVE_MEMORY_RESOURCE
   #include <memory_resource>
   #else
   #include <list>
   #include <stdlib.h>
   #endif
      namespace r600 {
      #ifndef HAVE_MEMORY_RESOURCE
   /* Fallback memory resource if the C++17 memory resource is not
   * available
   */
   struct MemoryBacking {
      ~MemoryBacking();
   void *allocate(size_t size);
   void *allocate(size_t size, size_t align);
      };
   #endif
      struct MemoryPoolImpl {
   public:
      MemoryPoolImpl();
      #ifdef HAVE_MEMORY_RESOURCE
         #endif
         };
      MemoryPool::MemoryPool() noexcept:
         {
   }
      MemoryPool&
   MemoryPool::instance()
   {
      static thread_local MemoryPool me;
      }
      void
   MemoryPool::free()
   {
      delete impl;
      }
      void
   MemoryPool::initialize()
   {
      if (!impl)
      }
      void *
   MemoryPool::allocate(size_t size)
   {
      assert(impl);
      }
      void *
   MemoryPool::allocate(size_t size, size_t align)
   {
      assert(impl);
      }
      void
   MemoryPool::release_all()
   {
         }
      void
   init_pool()
   {
         }
      void
   release_pool()
   {
         }
      void *
   Allocate::operator new(size_t size)
   {
         }
      void
   Allocate::operator delete(void *p, size_t size)
   {
         }
      MemoryPoolImpl::MemoryPoolImpl() { pool = new MemoryBacking(); }
      MemoryPoolImpl::~MemoryPoolImpl() { delete pool; }
      #ifndef HAVE_MEMORY_RESOURCE
   MemoryBacking::~MemoryBacking()
   {
      for (auto p : m_data)
      }
      void *
   MemoryBacking::allocate(size_t size)
   {
      void *retval = malloc(size);
   m_data.push_back(retval);
      }
      void *
   MemoryBacking::allocate(size_t size, size_t align)
   {
      void *retval = aligned_alloc(align, size);
   m_data.push_back(retval);
      }
      #endif
      } // namespace r600
