   /**************************************************************************
   * 
   * Copyright 2007 VMware, Inc.
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
      /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   */
      #include "util/detect.h"
   #include "pipe/p_state.h"
   #include "translate.h"
      struct translate *translate_create( const struct translate_key *key )
   {
            #if DETECT_ARCH_X86 || DETECT_ARCH_X86_64
      translate = translate_sse2_create( key );
   if (translate)
      #else
         #endif
            }
      bool translate_is_output_format_supported(enum pipe_format format)
   {
         }
