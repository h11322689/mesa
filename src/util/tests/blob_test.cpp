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
      #include <inttypes.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <stdbool.h>
   #include <string.h>
   #ifdef _MSC_VER
   #include <BaseTsd.h>
   typedef SSIZE_T ssize_t;
   #endif
      #include "util/ralloc.h"
   #include "blob.h"
      #include <gtest/gtest.h>
   #include "mesa-gtest-extras.h"
      #define bytes_test_str     "bytes_test"
   #define reserve_test_str   "reserve_test"
      // This placeholder must be the same length as the next overwrite_test_str.
   #define placeholder_str    "XXXXXXXXXXXXXX"
   #define overwrite_test_str "overwrite_test"
   #define uint32_test        0x12345678
   #define uint32_placeholder 0xDEADBEEF
   #define uint32_overwrite   0xA1B2C3D4
   #define uint64_test        0x1234567890ABCDEF
   #define string_test_str    "string_test"
         // Test at least one call of each blob_write_foo and blob_read_foo function,
   // verifying that we read out everything we wrote, that every bytes is
   // consumed, and that the overrun bit is not set.
   TEST(BlobTest, WriteAndReadFunctions)
   {
      struct blob blob;
   struct blob_reader reader;
   ssize_t reserved;
   size_t str_offset, uint_offset;
                                       reserved = blob_reserve_bytes(&blob, sizeof(reserve_test_str));
            // Write a placeholder, (to be replaced later via overwrite_bytes).
   str_offset = blob.size;
                     // Write a placeholder, (to be replaced later via overwrite_uint32).
   uint_offset = blob.size;
                                       // Finally, overwrite our placeholders.
   blob_overwrite_bytes(&blob, str_offset, overwrite_test_str,
                                    EXPECT_STREQ(bytes_test_str,
                           EXPECT_STREQ(reserve_test_str, (char *) reserve_buf)
            EXPECT_STREQ(overwrite_test_str,
                  EXPECT_EQ(uint32_test, blob_read_uint32(&reader)) << "blob_write/read_uint32";
   EXPECT_EQ(uint32_overwrite, blob_read_uint32(&reader)) << "blob_overwrite_uint32";
   EXPECT_EQ(uint64_test, blob_read_uint64(&reader)) << "blob_write/read_uint64";
                     EXPECT_EQ(reader.end - reader.data, reader.current - reader.data) << "read_consumes_all_bytes";
               }
      // Test that data values are written and read with proper alignment.
   TEST(BlobTest, Alignment)
   {
      struct blob blob;
   struct blob_reader reader;
   uint8_t bytes[] = "ABCDEFGHIJKLMNOP";
                     // First, write an intptr value to the blob and capture that size. This is
   // the expected offset between any pair of intptr values (if written with
   // alignment).
            delta = blob.size;
            // Then loop doing the following:
   //
   //   1. Write an unaligned number of bytes
   //   2. Verify that write results in an unaligned size
   //   3. Write an intptr_t value
   //   2. Verify that that write results in an aligned size
   //
   for (num_bytes = 1; num_bytes < sizeof(intptr_t); num_bytes++) {
                                                      // Finally, test that reading also does proper alignment. Since we know
   // that values were written with all the right alignment, all we have to do
   // here is verify that correct values are read.
            EXPECT_EQ((intptr_t) &blob, blob_read_intptr(&reader))
            for (num_bytes = 1; num_bytes < sizeof(intptr_t); num_bytes++) {
      EXPECT_U8_ARRAY_EQUAL(bytes, (const uint8_t *) blob_read_bytes(&reader, num_bytes),
                        }
      // Test that we detect overrun.
   TEST(BlobTest, DetectOverrun)
   {
      struct blob blob;
   struct blob_reader reader;
                                       EXPECT_EQ(value, blob_read_uint32(&reader)) << "read before overrun";
   EXPECT_FALSE(reader.overrun);
   EXPECT_EQ(0, blob_read_uint32(&reader)) << "read at overrun";
               }
      // Test that we can read and write some large objects, (exercising the code in
   // the blob_write functions to realloc blob->data.
   TEST(BlobTest, BigObjects)
   {
      void *ctx = ralloc_context(NULL);
   struct blob blob;
   struct blob_reader reader;
   int size = 1000;
   int count = 1000;
                     // Initialize our buffer.
   buf = (char *) ralloc_size(ctx, size);
   for (int i = 0; i < size; i++) {
                  // Write it many times.
   for (int i = 0; i < count; i++) {
                           // Read and verify it many times.
   for (int i = 0; i < count; i++) {
      EXPECT_U8_ARRAY_EQUAL((uint8_t *) buf,
                     EXPECT_EQ(reader.end - reader.data, reader.current - reader.data)
                     blob_finish(&blob);
      }
