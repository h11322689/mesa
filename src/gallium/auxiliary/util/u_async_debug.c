   /*
   * Copyright 2017 Advanced Micro Devices, Inc.
   * All Rights Reserved.
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
   *
   */
      #include "u_async_debug.h"
      #include <stdio.h>
   #include <stdarg.h>
      #include "util/u_debug.h"
   #include "util/u_string.h"
      static void
   u_async_debug_message(void *data, unsigned *id, enum util_debug_type type,
         {
      struct util_async_debug_callback *adbg = data;
   struct util_debug_message *msg;
   char *text;
            r = vasprintf(&text, fmt, args);
   if (r < 0)
            simple_mtx_lock(&adbg->lock);
   if (adbg->count >= adbg->max) {
               if (new_max < adbg->max ||
      new_max > SIZE_MAX / sizeof(*adbg->messages)) {
   free(text);
               struct util_debug_message *new_msg =
         if (!new_msg) {
      free(text);
               adbg->max = new_max;
               msg = &adbg->messages[adbg->count++];
   msg->id = id;
   msg->type = type;
         out:
         }
      void
   u_async_debug_init(struct util_async_debug_callback *adbg)
   {
               simple_mtx_init(&adbg->lock, mtx_plain);
   adbg->base.async = true;
   adbg->base.debug_message = u_async_debug_message;
      }
      void
   u_async_debug_cleanup(struct util_async_debug_callback *adbg)
   {
               for (unsigned i = 0; i < adbg->count; ++i)
            }
      void
   _u_async_debug_drain(struct util_async_debug_callback *adbg,
         {
      simple_mtx_lock(&adbg->lock);
   for (unsigned i = 0; i < adbg->count; ++i) {
                                    adbg->count = 0;
      }
   