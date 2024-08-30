   /* Copyright © 2007 Carl Worth
   * Copyright © 2009 Jeremy Huddleston, Julien Cristau, and Matthieu Herrb
   * Copyright © 2009-2010 Mikhail Gusarov
   * Copyright © 2012 Yaakov Selkowitz and Keith Packard
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "sha1/sha1.h"
   #include "mesa-sha1.h"
   #include "hex.h"
   #include <string.h>
   #include <inttypes.h>
      void
   _mesa_sha1_compute(const void *data, size_t size, unsigned char result[20])
   {
               _mesa_sha1_init(&ctx);
   _mesa_sha1_update(&ctx, data, size);
      }
      void
   _mesa_sha1_format(char *buf, const unsigned char *sha1)
   {
         }
      /* Convert a hashs string hexidecimal representation into its more compact
   * form.
   */
   void
   _mesa_sha1_hex_to_sha1(unsigned char *buf, const char *hex)
   {
         }
      static void
   sha1_to_uint32(const uint8_t sha1[SHA1_DIGEST_LENGTH],
         {
               for (unsigned i = 0; i < SHA1_DIGEST_LENGTH; i++)
      }
      void
   _mesa_sha1_print(FILE *f, const uint8_t sha1[SHA1_DIGEST_LENGTH])
   {
      uint32_t u32[SHA1_DIGEST_LENGTH];
            for (unsigned i = 0; i < SHA1_DIGEST_LENGTH32; i++) {
            }
      bool
   _mesa_printed_sha1_equal(const uint8_t sha1[SHA1_DIGEST_LENGTH],
         {
      uint32_t u32[SHA1_DIGEST_LENGTH32];
               }
