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
      /**
   * @file
   * Memory debugging.
   *
   * @author José Fonseca <jfonseca@vmware.com>
   */
      #include "util/detect.h"
      #define DEBUG_MEMORY_IMPLEMENTATION
      #include "util/u_thread.h"
      #include "util/simple_mtx.h"
   #include "util/u_debug.h"
   #include "util/u_debug_stack.h"
   #include "util/list.h"
   #include "util/os_memory.h"
   #include "util/os_memory_debug.h"
         #define DEBUG_MEMORY_MAGIC 0x6e34090aU
   #define DEBUG_MEMORY_STACK 0 /* XXX: disabled until we have symbol lookup */
      /**
   * Set to 1 to enable checking of freed blocks of memory.
   * Basically, don't really deallocate freed memory; keep it in the list
   * but mark it as freed and do extra checking in debug_memory_check().
   * This can detect some cases of use-after-free.  But note that since we
   * never really free anything this will use a lot of memory.
   */
   #define DEBUG_FREED_MEMORY 0
   #define DEBUG_FREED_BYTE 0x33
         struct debug_memory_header
   {
               unsigned long no;
   const char *file;
   unsigned line;
      #if DEBUG_MEMORY_STACK
         #endif
         #if DEBUG_FREED_MEMORY
         #endif
         unsigned magic;
      };
      struct debug_memory_footer
   {
         };
         static struct list_head list = { &list, &list };
      static simple_mtx_t list_mutex = SIMPLE_MTX_INITIALIZER;
      static unsigned long last_no = 0;
         static inline struct debug_memory_header *
   header_from_data(void *data)
   {
      if (data)
         else
      }
      static inline void *
   data_from_header(struct debug_memory_header *hdr)
   {
      if (hdr)
         else
      }
      static inline struct debug_memory_footer *
   footer_from_header(struct debug_memory_header *hdr)
   {
      if (hdr)
         else
      }
         void *
   debug_malloc(const char *file, unsigned line, const char *function,
         {
      struct debug_memory_header *hdr;
            hdr = os_malloc(sizeof(*hdr) + size + sizeof(*ftr));
   if (!hdr) {
      debug_printf("%s:%u:%s: out of memory when trying to allocate %lu bytes\n",
                           hdr->no = last_no++;
   hdr->file = file;
   hdr->line = line;
   hdr->function = function;
   hdr->size = size;
   hdr->magic = DEBUG_MEMORY_MAGIC;
      #if DEBUG_FREED_MEMORY
         #endif
      #if DEBUG_MEMORY_STACK
         #endif
         ftr = footer_from_header(hdr);
            simple_mtx_lock(&list_mutex);
   list_addtail(&hdr->head, &list);
               }
      void
   debug_free(const char *file, unsigned line, const char *function,
         {
      struct debug_memory_header *hdr;
            if (!ptr)
            hdr = header_from_data(ptr);
   if (hdr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: freeing bad or corrupted memory %p\n",
               assert(0);
               ftr = footer_from_header(hdr);
   if (ftr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: buffer overflow %p\n",
                        #if DEBUG_FREED_MEMORY
      /* Check for double-free */
   assert(!hdr->freed);
   /* Mark the block as freed but don't really free it */
   hdr->freed = true;
   /* Save file/line where freed */
   hdr->file = file;
   hdr->line = line;
   /* set freed memory to special value */
      #else
      simple_mtx_lock(&list_mutex);
   list_del(&hdr->head);
   simple_mtx_unlock(&list_mutex);
   hdr->magic = 0;
               #endif
   }
      void *
   debug_calloc(const char *file, unsigned line, const char *function,
         {
      void *ptr = debug_malloc( file, line, function, count * size );
   if (ptr)
            }
      void *
   debug_realloc(const char *file, unsigned line, const char *function,
         {
      struct debug_memory_header *old_hdr, *new_hdr;
   struct debug_memory_footer *old_ftr, *new_ftr;
            if (!old_ptr)
            if (!new_size) {
      debug_free( file, line, function, old_ptr );
               old_hdr = header_from_data(old_ptr);
   if (old_hdr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: reallocating bad or corrupted memory %p\n",
               assert(0);
               old_ftr = footer_from_header(old_hdr);
   if (old_ftr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: buffer overflow %p\n",
                           /* alloc new */
   new_hdr = os_malloc(sizeof(*new_hdr) + new_size + sizeof(*new_ftr));
   if (!new_hdr) {
      debug_printf("%s:%u:%s: out of memory when trying to allocate %lu bytes\n",
                  }
   new_hdr->no = old_hdr->no;
   new_hdr->file = old_hdr->file;
   new_hdr->line = old_hdr->line;
   new_hdr->function = old_hdr->function;
   new_hdr->size = new_size;
   new_hdr->magic = DEBUG_MEMORY_MAGIC;
      #if DEBUG_FREED_MEMORY
         #endif
         new_ftr = footer_from_header(new_hdr);
            simple_mtx_lock(&list_mutex);
   list_replace(&old_hdr->head, &new_hdr->head);
            /* copy data */
   new_ptr = data_from_header(new_hdr);
            /* free old */
   old_hdr->magic = 0;
   old_ftr->magic = 0;
               }
      unsigned long
   debug_memory_begin(void)
   {
         }
      void
   debug_memory_end(unsigned long start_no)
   {
      size_t total_size = 0;
            if (start_no == last_no)
            entry = list.prev;
   for (; entry != &list; entry = entry->prev) {
      struct debug_memory_header *hdr;
   void *ptr;
            hdr = list_entry(entry, struct debug_memory_header, head);
   ptr = data_from_header(hdr);
            if (hdr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: bad or corrupted memory %p\n",
                           if ((start_no <= hdr->no && hdr->no < last_no) ||
      (last_no < start_no && (hdr->no < last_no || start_no <= hdr->no))) {
   debug_printf("%s:%u:%s: %lu bytes at %p not freed\n",
      #if DEBUG_MEMORY_STACK
         #endif
                        if (ftr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: buffer overflow %p\n",
                              if (total_size) {
      debug_printf("Total of %lu KB of system memory apparently leaked\n",
      }
   else {
            }
         /**
   * Put a tag (arbitrary integer) on a memory block.
   * Can be useful for debugging.
   */
   void
   debug_memory_tag(void *ptr, unsigned tag)
   {
               if (!ptr)
            hdr = header_from_data(ptr);
   if (hdr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s corrupted memory at %p\n", __func__, ptr);
                  }
         /**
   * Check the given block of memory for validity/corruption.
   */
   void
   debug_memory_check_block(void *ptr)
   {
      struct debug_memory_header *hdr;
            if (!ptr)
            hdr = header_from_data(ptr);
            if (hdr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: bad or corrupted memory %p\n",
                     if (ftr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: buffer overflow %p\n",
               }
            /**
   * We can periodically call this from elsewhere to do a basic sanity
   * check of the heap memory we've allocated.
   */
   void
   debug_memory_check(void)
   {
               entry = list.prev;
   for (; entry != &list; entry = entry->prev) {
      struct debug_memory_header *hdr;
   struct debug_memory_footer *ftr;
            hdr = list_entry(entry, struct debug_memory_header, head);
   ftr = footer_from_header(hdr);
            if (hdr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: bad or corrupted memory %p\n",
                     if (ftr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: buffer overflow %p\n",
               #if DEBUG_FREED_MEMORY
         /* If this block is marked as freed, check that it hasn't been touched */
   if (hdr->freed) {
      int i;
   for (i = 0; i < hdr->size; i++) {
      if (ptr[i] != DEBUG_FREED_BYTE) {
      debug_printf("Memory error: byte %d of block at %p of size %d is 0x%x\n",
            }
         #endif
         }
