   /*
   * Copyright (C) 2019 Connor Abbott <cwabbott0@gmail.com>
   * Copyright (C) 2019 Lyude Paul <thatslyude@gmail.com>
   * Copyright (C) 2019 Ryan Houdek <Sonicadvance1@gmail.com>
   * Copyright (C) 2019-2020 Collabora, Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      /* Prints shared with the disassembler */
      #include "bi_print_common.h"
      const char *
   bi_message_type_name(enum bifrost_message_type T)
   {
      switch (T) {
   case BIFROST_MESSAGE_NONE:
         case BIFROST_MESSAGE_VARYING:
         case BIFROST_MESSAGE_ATTRIBUTE:
         case BIFROST_MESSAGE_TEX:
         case BIFROST_MESSAGE_VARTEX:
         case BIFROST_MESSAGE_LOAD:
         case BIFROST_MESSAGE_STORE:
         case BIFROST_MESSAGE_ATOMIC:
         case BIFROST_MESSAGE_BARRIER:
         case BIFROST_MESSAGE_BLEND:
         case BIFROST_MESSAGE_TILE:
         case BIFROST_MESSAGE_Z_STENCIL:
         case BIFROST_MESSAGE_ATEST:
         case BIFROST_MESSAGE_JOB:
         case BIFROST_MESSAGE_64BIT:
         default:
            }
      const char *
   bi_flow_control_name(enum bifrost_flow mode)
   {
      switch (mode) {
   case BIFROST_FLOW_END:
         case BIFROST_FLOW_NBTB_PC:
         case BIFROST_FLOW_NBTB_UNCONDITIONAL:
         case BIFROST_FLOW_NBTB:
         case BIFROST_FLOW_BTB_UNCONDITIONAL:
         case BIFROST_FLOW_BTB_NONE:
         case BIFROST_FLOW_WE_UNCONDITIONAL:
         case BIFROST_FLOW_WE:
         default:
            }
