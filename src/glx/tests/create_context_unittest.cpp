   /*
   * Copyright © 2011 Intel Corporation
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
   #include <string.h>
      #include "glxclient.h"
   #include "glx_error.h"
      #include <xcb/glx.h>
   #include "mock_xdisplay.h"
   #include "fake_glx_screen.h"
      static bool CreateContextAttribsARB_was_sent;
   static xcb_glx_create_context_attribs_arb_request_t req;
   static uint32_t sent_attribs[1024];
   static uint32_t next_id;
         struct glx_screen *psc;
      extern "C" Bool
   glx_context_init(struct glx_context *gc,
         {
      gc->majorOpcode = 123;
   gc->psc = psc;
   gc->config = config;
   gc->isDirect = GL_TRUE;
               }
      bool GetGLXScreenConfigs_called = false;
      extern "C" struct glx_screen *
   GetGLXScreenConfigs(Display * dpy, int scrn)
   {
      (void) dpy;
            GetGLXScreenConfigs_called = true;
      }
      extern "C" uint32_t
   xcb_generate_id(xcb_connection_t *c)
   {
                  }
      extern "C" xcb_void_cookie_t
   xcb_glx_create_context_attribs_arb_checked(xcb_connection_t *c,
         xcb_glx_context_t context,
   uint32_t fbconfig,
   uint32_t screen,
   uint32_t share_list,
   uint8_t is_direct,
   uint32_t num_attribs,
   {
               CreateContextAttribsARB_was_sent = true;
   req.context = context;
   req.fbconfig = fbconfig;
   req.screen = screen;
   req.share_list = share_list;
   req.is_direct = is_direct;
            if (num_attribs != 0 && attribs != NULL)
            xcb_void_cookie_t cookie;
               }
      extern "C" xcb_void_cookie_t
   xcb_glx_destroy_context(xcb_connection_t *c, xcb_glx_context_t context)
   {
      xcb_void_cookie_t cookie;
               }
      extern "C" xcb_generic_error_t *
   xcb_request_check(xcb_connection_t *c, xcb_void_cookie_t cookie)
   {
         }
      extern "C" void
   __glXSendErrorForXcb(Display * dpy, const xcb_generic_error_t *err)
   {
   }
      extern "C" void
   __glXSendError(Display * dpy, int_fast8_t errorCode, uint_fast32_t resourceID,
         {
   }
      class glXCreateContextAttribARB_test : public ::testing::Test {
   public:
      virtual void SetUp();
            /**
   * Replace the existing screen with a direct-rendering screen
   */
            mock_XDisplay *dpy;
   GLXContext ctx;
      };
      void
   glXCreateContextAttribARB_test::SetUp()
   {
      CreateContextAttribsARB_was_sent = false;
   memset(&req, 0, sizeof(req));
   next_id = 99;
   fake_glx_context::contexts_allocated = 0;
                     memset(&this->fbc, 0, sizeof(this->fbc));
               }
      void
   glXCreateContextAttribARB_test::TearDown()
   {
      if (ctx)
                        }
      void
   glXCreateContextAttribARB_test::use_direct_rendering_screen()
   {
      struct glx_screen *direct_psc =
         psc->scr,
            delete (fake_glx_screen *)psc;
      }
      /**
   * \name Verify detection of client-side errors
   */
   /*@{*/
   TEST_F(glXCreateContextAttribARB_test, NULL_display_returns_None)
   {
      GLXContext ctx =
                  EXPECT_EQ(None, ctx);
      }
      TEST_F(glXCreateContextAttribARB_test, NULL_screen_returns_None)
   {
      delete (fake_glx_screen *)psc;
            GLXContext ctx =
                  EXPECT_EQ(None, ctx);
      }
   /*@}*/
      /**
   * \name Verify that correct protocol bits are sent to the server.
   */
   /*@{*/
   TEST_F(glXCreateContextAttribARB_test, does_send_protocol)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
               }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_context)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
            }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_fbconfig)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
               }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_share_list)
   {
      GLXContext share =
                           ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, share,
            struct glx_context *glx_ctx = (struct glx_context *) share;
               }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_is_direct_for_indirect_screen_and_direct_set_to_true)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
               }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_is_direct_for_indirect_screen_and_direct_set_to_false)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
               }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_is_direct_for_direct_screen_and_direct_set_to_true)
   {
               ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
               }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_is_direct_for_direct_screen_and_direct_set_to_false)
   {
               ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
               }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_screen)
   {
      this->fbc.screen = 7;
            ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
               }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_num_attribs)
   {
      /* Use zeros in the second half of each attribute pair to try and trick the
   * implementation into termiating the list early.
   *
   * Use non-zero in the second half of the last attribute pair to try and
   * trick the implementation into not terminating the list early enough.
   */
   static const int attribs[] = {
      1, 0,
   2, 0,
   3, 0,
   4, 0,
   0, 6,
               ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
               }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_num_attribs_empty_list)
   {
      static const int attribs[] = {
                  ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
               }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_num_attribs_NULL_list_pointer)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
               }
      TEST_F(glXCreateContextAttribARB_test, sent_correct_attrib_list)
   {
      int attribs[] = {
      GLX_RENDER_TYPE, GLX_RGBA_TYPE,
   GLX_CONTEXT_MAJOR_VERSION_ARB, 1,
   GLX_CONTEXT_MINOR_VERSION_ARB, 2,
               ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
            for (unsigned i = 0; i < 6; i++) {
            }
   /*@}*/
      /**
   * \name Verify details of the returned GLXContext
   */
   /*@{*/
   TEST_F(glXCreateContextAttribARB_test, correct_context)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
            /* Since the server did not return an error, the GLXContext should not be
   * NULL.
   */
            /* It shouldn't be the XID of the context either.
   */
      }
      TEST_F(glXCreateContextAttribARB_test, correct_context_xid)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
            /* Since the server did not return an error, the GLXContext should not be
   * NULL.
   */
            struct glx_context *glx_ctx = (struct glx_context *) ctx;
      }
      TEST_F(glXCreateContextAttribARB_test, correct_context_share_xid)
   {
      GLXContext first =
                           GLXContext second =
                           struct glx_context *share = (struct glx_context *) first;
   struct glx_context *ctx = (struct glx_context *) second;
            delete (fake_glx_context *)first;
      }
      TEST_F(glXCreateContextAttribARB_test, correct_context_isDirect_for_indirect_screen_and_direct_set_to_true)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
                                 }
      TEST_F(glXCreateContextAttribARB_test, correct_context_isDirect_for_indirect_screen_and_direct_set_to_false)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
                                 }
      TEST_F(glXCreateContextAttribARB_test, correct_context_isDirect_for_direct_screen_and_direct_set_to_true)
   {
               ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
                                 }
      TEST_F(glXCreateContextAttribARB_test, correct_context_isDirect_for_direct_screen_and_direct_set_to_false)
   {
               ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
                                 }
      TEST_F(glXCreateContextAttribARB_test, correct_indirect_context_client_state_private)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
                              ASSERT_FALSE(gc->isDirect);
   EXPECT_EQ((struct __GLXattributeRec *) 0xcafebabe,
      }
      TEST_F(glXCreateContextAttribARB_test, correct_indirect_context_config)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
                                 }
      TEST_F(glXCreateContextAttribARB_test, correct_context_screen_number)
   {
      this->fbc.screen = 7;
            ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
                                 }
      TEST_F(glXCreateContextAttribARB_test, correct_context_screen_pointer)
   {
      ctx = glXCreateContextAttribsARB(this->dpy, (GLXFBConfig) &this->fbc, 0,
                                 }
   /*@}*/
