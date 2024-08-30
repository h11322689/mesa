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
   * Generic handle table implementation.
   *  
   * @author José Fonseca <jfonseca@vmware.com>
   */
         #include "util/compiler.h"
   #include "util/u_debug.h"
      #include "util/u_memory.h"
   #include "util/u_handle_table.h"
         #define HANDLE_TABLE_INITIAL_SIZE 16  
         struct handle_table
   {
      /** Object array. Empty handles have a null object */
   void **objects;
      /** Number of objects the handle can currently hold */
   unsigned size;
   /** Number of consecutive objects allocated at the start of the table */
   unsigned filled;
      /** Optional object destructor */
      };
         struct handle_table *
   handle_table_create(void)
   {
      struct handle_table *ht;
      ht = MALLOC_STRUCT(handle_table);
   if (!ht)
            ht->objects = (void **)CALLOC(HANDLE_TABLE_INITIAL_SIZE, sizeof(void *));
   if(!ht->objects) {
      FREE(ht);
      }
      ht->size = HANDLE_TABLE_INITIAL_SIZE;
   ht->filled = 0;
      ht->destroy = NULL;
         }
         void
   handle_table_set_destroy(struct handle_table *ht,
         {
      assert(ht);
   if (!ht)
            }
         /**
   * Resize the table if necessary 
   */
   static inline int
   handle_table_resize(struct handle_table *ht,
         {
      unsigned new_size;
            if(ht->size > minimum_size)
            new_size = ht->size;
   while(!(new_size > minimum_size))
         assert(new_size);
      new_objects = (void **)REALLOC((void *)ht->objects,
      ht->size*sizeof(void *),
      if (!new_objects)
            memset(new_objects + ht->size, 0, (new_size - ht->size)*sizeof(void *));
      ht->size = new_size;
   ht->objects = new_objects;
         }
         static inline void
   handle_table_clear(struct handle_table *ht, 
         {
      void *object;
      /* The order here is important so that the object being destroyed is not
   * present in the table when seen by the destroy callback, because the 
   * destroy callback may directly or indirectly call the other functions in 
   * this module.
            object = ht->objects[index];
   if (object) {
      ht->objects[index] = NULL;
      if(ht->destroy)
         }
         unsigned
   handle_table_add(struct handle_table *ht, 
         {
      unsigned index;
   unsigned handle;
      assert(ht);
   assert(object);
   if(!object || !ht)
            /* linear search for an empty handle */
   while(ht->filled < ht->size) {
      break;
            }
      index = ht->filled;
   handle = index + 1;
      /* check integer overflow */
   if(!handle)
            /* grow the table if necessary */
   if(!handle_table_resize(ht, index))
            assert(!ht->objects[index]);
   ht->objects[index] = object;
   ++ht->filled;
         }
         unsigned
   handle_table_set(struct handle_table *ht, 
               {
      unsigned index;
      assert(ht);
   assert(handle);
   if(!handle || !ht)
            assert(object);
   if (!object)
                     /* grow the table if necessary */
   if(!handle_table_resize(ht, index))
                     ht->objects[index] = object;
         }
         void *
   handle_table_get(struct handle_table *ht, 
         {
      void *object;
      assert(ht);
   assert(handle);
   if(!handle || !ht || handle > ht->size)
            object = ht->objects[handle - 1];
         }
         void
   handle_table_remove(struct handle_table *ht, 
         {
      void *object;
   unsigned index;
      assert(ht);
   assert(handle);
   if(!handle || !ht || handle > ht->size)
            index = handle - 1;
   object = ht->objects[index];
   if (!object)
                     if(index < ht->filled)
      }
         unsigned
   handle_table_get_next_handle(struct handle_table *ht, 
         {
      unsigned index;
      for(index = handle; index < ht->size; ++index) {
      return index + 1;
                  }
         unsigned
   handle_table_get_first_handle(struct handle_table *ht)
   {
         }
         void
   handle_table_destroy(struct handle_table *ht)
   {
      unsigned index;
            if (!ht)
            if(ht->destroy)
      for(index = 0; index < ht->size; ++index)
         FREE(ht->objects);
      }
   