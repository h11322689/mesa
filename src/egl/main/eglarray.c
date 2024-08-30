   /**************************************************************************
   *
   * Copyright 2010 LunarG, Inc.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <assert.h>
   #include <stdlib.h>
   #include <string.h>
      #include "eglarray.h"
   #include "egllog.h"
      /**
   * Grow the size of the array.
   */
   static EGLBoolean
   _eglGrowArray(_EGLArray *array)
   {
      EGLint new_size;
            new_size = array->MaxSize;
   while (new_size <= array->Size)
            elems = realloc(array->Elements, new_size * sizeof(array->Elements[0]));
   if (!elems) {
      _eglLog(_EGL_DEBUG, "failed to grow %s array to %d", array->Name,
                     array->Elements = elems;
               }
      /**
   * Create an array.
   */
   _EGLArray *
   _eglCreateArray(const char *name, EGLint init_size)
   {
               array = calloc(1, sizeof(*array));
   if (array) {
      array->Name = name;
   array->MaxSize = (init_size > 0) ? init_size : 1;
   if (!_eglGrowArray(array)) {
      free(array);
                     }
      /**
   * Destroy an array, optionally free the data.
   */
   void
   _eglDestroyArray(_EGLArray *array, void (*free_cb)(void *))
   {
      if (free_cb) {
      EGLint i;
   for (i = 0; i < array->Size; i++)
      }
   free(array->Elements);
      }
      /**
   * Append a element to an array.
   */
   void
   _eglAppendArray(_EGLArray *array, void *elem)
   {
      if (array->Size >= array->MaxSize && !_eglGrowArray(array))
               }
      /**
   * Erase an element from an array.
   */
   void
   _eglEraseArray(_EGLArray *array, EGLint i, void (*free_cb)(void *))
   {
      if (free_cb)
         if (i < array->Size - 1) {
      memmove(&array->Elements[i], &array->Elements[i + 1],
      }
      }
      /**
   * Find in an array for the given element.
   */
   void *
   _eglFindArray(_EGLArray *array, void *elem)
   {
               if (!array)
            for (i = 0; i < array->Size; i++)
      if (array->Elements[i] == elem)
         }
      /**
   * Filter an array and return the number of filtered elements.
   */
   EGLint
   _eglFilterArray(_EGLArray *array, void **data, EGLint size,
         {
               if (!array)
            assert(filter);
   for (i = 0; i < array->Size; i++) {
      if (filter(array->Elements[i], filter_data)) {
      if (data && count < size)
            }
   if (data && count >= size)
                  }
      /**
   * Flatten an array by converting array elements into another form and store
   * them in a buffer.
   */
   EGLint
   _eglFlattenArray(_EGLArray *array, void *buffer, EGLint elem_size, EGLint size,
         {
               if (!array)
            count = array->Size;
   if (buffer) {
      /* clamp size to 0 */
   if (size < 0)
         /* do not exceed buffer size */
   if (count > size)
         for (i = 0; i < count; i++)
                  }
