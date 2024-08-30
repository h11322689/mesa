   /*
   * Copyright © 2017 Thomas Helland
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
   *
   */
      #include <stdlib.h>
   #include <stdio.h>
   #include <string.h>
   #include <assert.h>
   #include <gtest/gtest.h>
   #include "util/string_buffer.h"
      /**
   * \file string_buffer_test.cpp
   *
   * Test the string buffer implementation
   */
      #define INITIAL_BUF_SIZE 6
   class string_buffer : public ::testing::Test {
   public:
         struct _mesa_string_buffer *buf;
   const char *str1;
   const char *str2;
   const char *str3;
   char str4[80];
            virtual void SetUp();
      };
      void
   string_buffer::SetUp()
   {
      str1 = "test1";
   str2 = "test2";
   str3 = "test1test2";
      }
      void
   string_buffer::TearDown()
   {
      /* Finally, clean up after us */
      }
      static uint32_t
   space_left_in_buffer(_mesa_string_buffer *buf)
   {
         }
      TEST_F(string_buffer, string_buffer_tests)
   {
      /* The string terminator needs one byte, so there should one "missing" */
            /* Start by appending str1 */
   EXPECT_TRUE(_mesa_string_buffer_append(buf, str1));
   EXPECT_TRUE(space_left_in_buffer(buf) ==
                  /* Add more, so that the string is resized */
            /* The string should now be equal to str3 */
            /* Check that the length of the string is reset when clearing */
   _mesa_string_buffer_clear(buf);
   EXPECT_TRUE(buf->length == 0);
            /* Test a string with some formatting */
   snprintf(str4, sizeof(str4), "Testing formatting %d, %f", 100, 1.0);
   EXPECT_TRUE(_mesa_string_buffer_printf(buf, "Testing formatting %d, %f", 100, 1.0));
            /* Compile a string with some other formatting */
            /* Concatenate str5 to str4 */
            /* Now use the formatted append function again */
            /* The string buffer should now be equal to str4 */
                     /* Test appending by one char at a time */
   EXPECT_TRUE(_mesa_string_buffer_append_char(buf, 'a'));
   EXPECT_TRUE(buf->length == 1);
   EXPECT_TRUE(strcmp(buf->buf, "a") == 0);
   EXPECT_TRUE(_mesa_string_buffer_append_char(buf, 'a'));
      }
