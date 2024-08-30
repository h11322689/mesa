   /*
   * Copyright © 2014 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <string.h>
      #include "blob.h"
   #include "u_math.h"
      #ifdef HAVE_VALGRIND
   #include <valgrind.h>
   #include <memcheck.h>
   #define VG(x) x
   #else
   #define VG(x)
   #endif
      #define BLOB_INITIAL_SIZE 4096
      /* Ensure that \blob will be able to fit an additional object of size
   * \additional.  The growing (if any) will occur by doubling the existing
   * allocation.
   */
   static bool
   grow_to_fit(struct blob *blob, size_t additional)
   {
      size_t to_allocate;
            if (blob->out_of_memory)
            if (blob->size + additional <= blob->allocated)
            if (blob->fixed_allocation) {
      blob->out_of_memory = true;
               if (blob->allocated == 0)
         else
                     new_data = realloc(blob->data, to_allocate);
   if (new_data == NULL) {
      blob->out_of_memory = true;
               blob->data = new_data;
               }
      /* Align the blob->size so that reading or writing a value at (blob->data +
   * blob->size) will result in an access aligned to a granularity of \alignment
   * bytes.
   *
   * \return True unless allocation fails
   */
   bool
   blob_align(struct blob *blob, size_t alignment)
   {
               if (blob->size < new_size) {
      if (!grow_to_fit(blob, new_size - blob->size))
            if (blob->data)
                        }
      void
   blob_reader_align(struct blob_reader *blob, size_t alignment)
   {
         }
      void
   blob_init(struct blob *blob)
   {
      blob->data = NULL;
   blob->allocated = 0;
   blob->size = 0;
   blob->fixed_allocation = false;
      }
      void
   blob_init_fixed(struct blob *blob, void *data, size_t size)
   {
      blob->data = data;
   blob->allocated = size;
   blob->size = 0;
   blob->fixed_allocation = true;
      }
      void
   blob_finish_get_buffer(struct blob *blob, void **buffer, size_t *size)
   {
      *buffer = blob->data;
   *size = blob->size;
            /* Trim the buffer. */
      }
      bool
   blob_overwrite_bytes(struct blob *blob,
                     {
      /* Detect an attempt to overwrite data out of bounds. */
   if (offset + to_write < offset || blob->size < offset + to_write)
                     if (blob->data)
               }
      bool
   blob_write_bytes(struct blob *blob, const void *bytes, size_t to_write)
   {
      if (! grow_to_fit(blob, to_write))
            if (blob->data && to_write > 0) {
      VG(VALGRIND_CHECK_MEM_IS_DEFINED(bytes, to_write));
      }
               }
      intptr_t
   blob_reserve_bytes(struct blob *blob, size_t to_write)
   {
               if (! grow_to_fit (blob, to_write))
            ret = blob->size;
               }
      intptr_t
   blob_reserve_uint32(struct blob *blob)
   {
      blob_align(blob, sizeof(uint32_t));
      }
      intptr_t
   blob_reserve_intptr(struct blob *blob)
   {
      blob_align(blob, sizeof(intptr_t));
      }
      #define BLOB_WRITE_TYPE(name, type)                      \
   bool                                                     \
   name(struct blob *blob, type value)                      \
   {                                                        \
      blob_align(blob, sizeof(value));                      \
      }
      BLOB_WRITE_TYPE(blob_write_uint8, uint8_t)
   BLOB_WRITE_TYPE(blob_write_uint16, uint16_t)
   BLOB_WRITE_TYPE(blob_write_uint32, uint32_t)
   BLOB_WRITE_TYPE(blob_write_uint64, uint64_t)
   BLOB_WRITE_TYPE(blob_write_intptr, intptr_t)
      #define ASSERT_ALIGNED(_offset, _align) \
            bool
   blob_overwrite_uint8 (struct blob *blob,
               {
      ASSERT_ALIGNED(offset, sizeof(value));
      }
      bool
   blob_overwrite_uint32 (struct blob *blob,
               {
      ASSERT_ALIGNED(offset, sizeof(value));
      }
      bool
   blob_overwrite_intptr (struct blob *blob,
               {
      ASSERT_ALIGNED(offset, sizeof(value));
      }
      bool
   blob_write_string(struct blob *blob, const char *str)
   {
         }
      void
   blob_reader_init(struct blob_reader *blob, const void *data, size_t size)
   {
      blob->data = data;
   blob->end = blob->data + size;
   blob->current = data;
      }
      /* Check that an object of size \size can be read from this blob.
   *
   * If not, set blob->overrun to indicate that we attempted to read too far.
   */
   static bool
   ensure_can_read(struct blob_reader *blob, size_t size)
   {
      if (blob->overrun)
            if (blob->current <= blob->end && blob->end - blob->current >= size)
                        }
      const void *
   blob_read_bytes(struct blob_reader *blob, size_t size)
   {
               if (! ensure_can_read (blob, size))
                                 }
      void
   blob_copy_bytes(struct blob_reader *blob, void *dest, size_t size)
   {
               bytes = blob_read_bytes(blob, size);
   if (bytes == NULL || size == 0)
               }
      void
   blob_skip_bytes(struct blob_reader *blob, size_t size)
   {
      if (ensure_can_read (blob, size))
      }
      #define BLOB_READ_TYPE(name, type)         \
   type                                       \
   name(struct blob_reader *blob)             \
   {                                          \
      type ret = 0;                           \
   int size = sizeof(ret);                 \
   blob_reader_align(blob, size);          \
   blob_copy_bytes(blob, &ret, size);      \
      }
      BLOB_READ_TYPE(blob_read_uint8, uint8_t)
   BLOB_READ_TYPE(blob_read_uint16, uint16_t)
   BLOB_READ_TYPE(blob_read_uint32, uint32_t)
   BLOB_READ_TYPE(blob_read_uint64, uint64_t)
   BLOB_READ_TYPE(blob_read_intptr, intptr_t)
      char *
   blob_read_string(struct blob_reader *blob)
   {
      int size;
   char *ret;
            /* If we're already at the end, then this is an overrun. */
   if (blob->current >= blob->end) {
      blob->overrun = true;
               /* Similarly, if there is no zero byte in the data remaining in this blob,
   * we also consider that an overrun.
   */
            if (nul == NULL) {
      blob->overrun = true;
                                                      }
