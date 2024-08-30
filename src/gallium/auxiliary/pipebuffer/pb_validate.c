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
   * Buffer validation.
   * 
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "util/compiler.h"
   #include "pipe/p_defines.h"
   #include "util/u_memory.h"
   #include "util/u_debug.h"
   #include "util/u_hash_table.h"
      #include "pb_buffer.h"
   #include "pb_validate.h"
         #define PB_VALIDATE_INITIAL_SIZE 1 /* 512 */ 
         struct pb_validate_entry
   {
      struct pb_buffer *buf;
      };
         struct pb_validate
   {
      struct pb_validate_entry *entries;
   unsigned used;
      };
         enum pipe_error
   pb_validate_add_buffer(struct pb_validate *vl,
                           {
      assert(buf);
   *already_present = false;
   if (!buf)
            assert(flags & PB_USAGE_GPU_READ_WRITE);
   assert(!(flags & ~PB_USAGE_GPU_READ_WRITE));
            if (ht) {
               if (entry_idx) {
               assert(entry->buf == buf);
                                 /* Grow the table */
   if(vl->used == vl->size) {
      unsigned new_size;
   struct pb_validate_entry *new_entries;
      new_size = vl->size * 2;
   return PIPE_ERROR_OUT_OF_MEMORY;
            new_entries = (struct pb_validate_entry *)REALLOC(vl->entries,
               if (!new_entries)
            memset(new_entries + vl->size, 0, (new_size - vl->size)*sizeof(struct pb_validate_entry));
      vl->size = new_size;
      }
      assert(!vl->entries[vl->used].buf);
   pb_reference(&vl->entries[vl->used].buf, buf);
   vl->entries[vl->used].flags = flags;
            if (ht)
               }
         enum pipe_error
   pb_validate_foreach(struct pb_validate *vl,
               {
      unsigned i;
   for(i = 0; i < vl->used; ++i) {
      enum pipe_error ret;
   ret = callback(vl->entries[i].buf, data);
   if(ret != PIPE_OK)
      }
      }
         enum pipe_error
   pb_validate_validate(struct pb_validate *vl) 
   {
      unsigned i;
      for(i = 0; i < vl->used; ++i) {
      enum pipe_error ret;
   ret = pb_validate(vl->entries[i].buf, vl, vl->entries[i].flags);
   if(ret != PIPE_OK) {
      while(i--)
                           }
         void
   pb_validate_fence(struct pb_validate *vl,
         {
      unsigned i;
   for(i = 0; i < vl->used; ++i) {
      pb_fence(vl->entries[i].buf, fence);
      }
      }
         void
   pb_validate_destroy(struct pb_validate *vl)
   {
      unsigned i;
   for(i = 0; i < vl->used; ++i)
         FREE(vl->entries);
      }
         struct pb_validate *
   pb_validate_create()
   {
      struct pb_validate *vl;
      vl = CALLOC_STRUCT(pb_validate);
   if (!vl)
            vl->size = PB_VALIDATE_INITIAL_SIZE;
   vl->entries = (struct pb_validate_entry *)CALLOC(vl->size, sizeof(struct pb_validate_entry));
   if(!vl->entries) {
      FREE(vl);
                  }
   