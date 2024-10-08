   /*
   * Copyright © 2013 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
   #include <gtest/gtest.h>
   #include <signal.h>
   #include <setjmp.h>
      #include "glxclient.h"
   #include "glx_error.h"
      extern bool GetGLXScreenConfigs_called;
   extern struct glx_screen *psc;
      struct attribute_test_vector {
      const char *string;
      };
      #define E(x) { # x, x }
            static bool got_sigsegv;
   static jmp_buf jmp;
      static void
   sigsegv_handler(int sig)
   {
      (void) sig;
   got_sigsegv = true;
      }
      static bool query_renderer_string_called = false;
   static bool query_renderer_integer_called = false;
      static int
   fake_query_renderer_integer(struct glx_screen *psc, int attribute,
         {
      (void) psc;
   (void) attribute;
                        }
      static int
   fake_query_renderer_string(struct glx_screen *psc, int attribute,
         {
      (void) psc;
   (void) attribute;
                        }
      struct glx_screen_vtable fake_vtable = {
      NULL,
   NULL,
   fake_query_renderer_integer,
      };
      class query_renderer_string_test : public ::testing::Test {
   public:
      virtual void SetUp();
            struct glx_screen scr;
   struct sigaction sa;
   struct sigaction old_sa;
      };
      class query_renderer_integer_test : public query_renderer_string_test {
   };
      void query_renderer_string_test::SetUp()
   {
      memset(&scr, 0, sizeof(scr));
   scr.vtable = &fake_vtable;
                     sa.sa_handler = sigsegv_handler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
      }
      void query_renderer_string_test::TearDown()
   {
         }
      /**
   * glXQueryRendererStringMESA will return \c NULL if the query_render_string
   * vtable entry is \c NULL.  It will also not segfault.
   */
   TEST_F(query_renderer_string_test, null_query_render_string)
   {
      struct glx_screen_vtable vtable = {
      NULL,
   NULL,
   NULL,
                        if (setjmp(jmp) == 0) {
      const char *str =
            } else {
            }
      /**
   * glXQueryRendererStringMESA will not call the screen query_render_string
   * function with an invalid GLX enum value, and it will return NULL.
   */
   TEST_F(query_renderer_string_test, invalid_attribute)
   {
      static const attribute_test_vector invalid_attributes[] = {
      /* These values are just plain invalid for use with this extension.
   */
   E(0),
   E(GLX_VENDOR),
   E(GLX_VERSION),
   E(GLX_EXTENSIONS),
   E(GLX_RENDERER_VENDOR_ID_MESA + 0x10000),
            /* These enums are part of the extension, but they are not allowed for
   * the string query.
   */
   E(GLX_RENDERER_VERSION_MESA),
   E(GLX_RENDERER_ACCELERATED_MESA),
   E(GLX_RENDERER_VIDEO_MEMORY_MESA),
   E(GLX_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_MESA),
   E(GLX_RENDERER_PREFERRED_PROFILE_MESA),
   E(GLX_RENDERER_OPENGL_CORE_PROFILE_VERSION_MESA),
   E(GLX_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION_MESA),
   E(GLX_RENDERER_OPENGL_ES_PROFILE_VERSION_MESA),
               for (unsigned i = 0; i < ARRAY_SIZE(invalid_attributes); i++) {
      query_renderer_integer_called = false;
            const char *str =
         EXPECT_EQ((char *)0, str) << invalid_attributes[i].string;
   EXPECT_FALSE(query_renderer_integer_called)
         EXPECT_FALSE(query_renderer_string_called)
         }
      /**
   * glXQueryRendererStringMESA will not call GetGLXScreenConfigs if the display
   * pointer is \c NULL.  It will also not segfault.
   */
   TEST_F(query_renderer_string_test, null_display_pointer)
   {
      if (setjmp(jmp) == 0) {
               const char *str =
         EXPECT_EQ((char *)0, str);
      } else {
            }
      /**
   * glXQueryRendererStringMESA will return error if GetGLXScreenConfigs returns
   * NULL.  It will also not segfault.
   */
   TEST_F(query_renderer_string_test, null_screen_pointer)
   {
               if (setjmp(jmp) == 0) {
               const char *str =
         EXPECT_EQ((char *)0, str);
      } else {
            }
      /**
   * glXQueryRendererStringMESA will not call the screen query_render_string
   * function if the renderer is invalid, and it will return NULL.
   */
   TEST_F(query_renderer_string_test, invalid_renderer_index)
   {
      static const int invalid_renderer_indices[] = {
      -1,
   1,
               if (setjmp(jmp) == 0) {
      for (unsigned i = 0; i < ARRAY_SIZE(invalid_renderer_indices); i++) {
      const char *str =
      glXQueryRendererStringMESA(&dpy, 0,
            EXPECT_EQ((char *)0, str) << invalid_renderer_indices[i];
   EXPECT_FALSE(query_renderer_integer_called)
         EXPECT_FALSE(query_renderer_string_called)
         } else {
            }
      /**
   * glXQueryCurrentRendererStringMESA will return error if there is no context
   * current.  It will also not segfault.
   */
   TEST_F(query_renderer_string_test, no_current_context)
   {
      if (setjmp(jmp) == 0) {
      const char *str =
            } else {
            }
      /**
   * glXQueryCurrentRendererIntegerMESA will return \c NULL if the
   * query_render_string vtable entry is \c NULL.  It will also not segfault.
   */
   TEST_F(query_renderer_integer_test, null_query_render_string)
   {
      struct glx_screen_vtable vtable = {
      NULL,
   NULL,
   NULL,
                        if (setjmp(jmp) == 0) {
      unsigned value = 0xDEADBEEF;
   Bool success = glXQueryRendererIntegerMESA(&dpy, 0, 0,
               EXPECT_FALSE(success);
      } else {
            }
      /**
   * glXQueryCurrentRendererIntegerMESA will not call the screen
   * query_render_string function with an invalid GLX enum value, and it will
   * return NULL.
   */
   TEST_F(query_renderer_integer_test, invalid_attribute)
   {
      static const attribute_test_vector invalid_attributes[] = {
      /* These values are just plain invalid for use with this extension.
   */
   E(0),
   E(GLX_VENDOR),
   E(GLX_VERSION),
   E(GLX_EXTENSIONS),
   E(GLX_RENDERER_VENDOR_ID_MESA + 0x10000),
   E(GLX_RENDERER_DEVICE_ID_MESA + 0x10000),
   E(GLX_RENDERER_VERSION_MESA + 0x10000),
   E(GLX_RENDERER_ACCELERATED_MESA + 0x10000),
   E(GLX_RENDERER_VIDEO_MEMORY_MESA + 0x10000),
   E(GLX_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_MESA + 0x10000),
   E(GLX_RENDERER_PREFERRED_PROFILE_MESA + 0x10000),
   E(GLX_RENDERER_OPENGL_CORE_PROFILE_VERSION_MESA + 0x10000),
   E(GLX_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION_MESA + 0x10000),
   E(GLX_RENDERER_OPENGL_ES_PROFILE_VERSION_MESA + 0x10000),
               for (unsigned i = 0; i < ARRAY_SIZE(invalid_attributes); i++) {
      query_renderer_integer_called = false;
            unsigned value = 0xDEADBEEF;
   Bool success =
      glXQueryRendererIntegerMESA(&dpy, 0, 0,
            EXPECT_FALSE(success) << invalid_attributes[i].string;
   EXPECT_EQ(0xDEADBEEF, value) << invalid_attributes[i].string;
   EXPECT_FALSE(query_renderer_integer_called)
         EXPECT_FALSE(query_renderer_string_called)
         }
      /**
   * glXQueryCurrentRendererIntegerMESA will not call GetGLXScreenConfigs if the
   * display pointer is \c NULL.  It will also not segfault.
   */
   TEST_F(query_renderer_integer_test, null_display_pointer)
   {
      if (setjmp(jmp) == 0) {
               unsigned value = 0xDEADBEEF;
   Bool success =
      glXQueryRendererIntegerMESA(NULL, 0, 0, GLX_RENDERER_VENDOR_ID_MESA,
      EXPECT_FALSE(success);
   EXPECT_EQ(0xDEADBEEF, value);
      } else {
            }
      /**
   * glXQueryCurrentRendererIntegerMESA will return error if GetGLXScreenConfigs
   * returns NULL.  It will also not segfault.
   */
   TEST_F(query_renderer_integer_test, null_screen_pointer)
   {
               if (setjmp(jmp) == 0) {
               unsigned value = 0xDEADBEEF;
   Bool success =
      glXQueryRendererIntegerMESA(&dpy, 0, 0, GLX_RENDERER_VENDOR_ID_MESA,
      EXPECT_FALSE(success);
   EXPECT_EQ(0xDEADBEEF, value);
      } else {
            }
      /**
   * glXQueryRendererIntegerMESA will not call the screen query_render_integer
   * function if the renderer is invalid, and it will return NULL.
   */
   TEST_F(query_renderer_integer_test, invalid_renderer_index)
   {
      static const int invalid_renderer_indices[] = {
      -1,
   1,
               if (setjmp(jmp) == 0) {
      for (unsigned i = 0; i < ARRAY_SIZE(invalid_renderer_indices); i++) {
      unsigned value = 0xDEADBEEF;
   Bool success =
      glXQueryRendererIntegerMESA(&dpy, 0,
                  EXPECT_FALSE(success) << invalid_renderer_indices[i];
   EXPECT_EQ(0xDEADBEEF, value) << invalid_renderer_indices[i];
   EXPECT_FALSE(query_renderer_integer_called)
         EXPECT_FALSE(query_renderer_string_called)
         } else {
            }
      /**
   * glXQueryCurrentRendererIntegerMESA will return error if there is no context
   * current.  It will also not segfault.
   */
   TEST_F(query_renderer_integer_test, no_current_context)
   {
      if (setjmp(jmp) == 0) {
      unsigned value = 0xDEADBEEF;
   Bool success =
      glXQueryCurrentRendererIntegerMESA(GLX_RENDERER_VENDOR_ID_MESA,
      EXPECT_FALSE(success);
      } else {
            }
