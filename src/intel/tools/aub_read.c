   /*
   * Copyright © 2016-2018 Intel Corporation
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
      #include <inttypes.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <stdarg.h>
      #include "common/intel_gem.h"
   #include "util/macros.h"
      #include "aub_read.h"
   #include "intel_context.h"
   #include "intel_aub.h"
      #define TYPE(dw)       (((dw) >> 29) & 7)
   #define OPCODE(dw)     (((dw) >> 23) & 0x3f)
   #define SUBOPCODE(dw)  (((dw) >> 16) & 0x7f)
      #define MAKE_HEADER(type, opcode, subopcode) \
            #define TYPE_AUB            0x7
      /* Classic AUB opcodes */
   #define OPCODE_AUB          0x01
   #define SUBOPCODE_HEADER    0x05
   #define SUBOPCODE_BLOCK     0x41
   #define SUBOPCODE_BMP       0x1e
      /* Newer version AUB opcode */
   #define OPCODE_NEW_AUB              0x2e
   #define SUBOPCODE_VERSION           0x00
   #define SUBOPCODE_REG_CMP           0x01
   #define SUBOPCODE_REG_POLL          0x02
   #define SUBOPCODE_REG_WRITE         0x03
   #define SUBOPCODE_MEM_CMP           0x04
   #define SUBOPCODE_MEM_POLL          0x05
   #define SUBOPCODE_MEM_WRITE         0x06
   #define SUBOPCODE_FRAME_BEGIN       0x07
   #define SUBOPCODE_COMMENT           0x08
   #define SUBOPCODE_TRACE_DELAY       0x09
   #define SUBOPCODE_MEM_DUMP          0x0a
   #define SUBOPCODE_MEM_WRITE_DISCONT 0x0b
   #define SUBOPCODE_TEST_PHASE_MARKER 0x0c
   #define SUBOPCODE_MEM_CONT_REGION   0x0d
   #define SUBOPCODE_VERSION_EXT       0x0e
   #define SUBOPCODE_PREDICATE         0x0f
   #define SUBOPCODE_DUMP_COMPRESS     0x10
      static PRINTFLIKE(3, 4) void
   parse_error(struct aub_read *read, const uint32_t *p, const char *fmt, ...)
   {
      if (!read->error)
            va_list ap;
            char msg[80];
   vsnprintf(msg, sizeof(msg), fmt, ap);
               }
      static bool
   handle_trace_header(struct aub_read *read, const uint32_t *p)
   {
      /* The intel_aubdump tool from IGT is kind enough to put a PCI-ID= tag in
   * the AUB header comment.  If the user hasn't specified a hardware
   * generation, try to use the one from the AUB file.
   */
   const uint32_t *end = p + (p[0] & 0xffff) + 2;
            if (end > &p[12] && p[12] > 0) {
      if (sscanf((char *)&p[13], "PCI-ID=%i", &aub_pci_id) > 0) {
      if (!intel_get_device_info_from_pci_id(aub_pci_id, &read->devinfo)) {
      parse_error(read, p,
                           char app_name[33];
   strncpy(app_name, (const char *)&p[2], 32);
            if (read->info)
               }
      static bool
   handle_memtrace_version(struct aub_read *read, const uint32_t *p)
   {
      int header_length = p[0] & 0xffff;
   char app_name[64];
   int app_name_len = MIN2(4 * (header_length + 1 - 5), ARRAY_SIZE(app_name) - 1);
   int pci_id_len = 0;
            strncpy(app_name, (const char *)&p[5], app_name_len);
            if (sscanf(app_name, "PCI-ID=%i %n", &aub_pci_id, &pci_id_len) > 0) {
      if (!intel_get_device_info_from_pci_id(aub_pci_id, &read->devinfo)) {
      parse_error(read, p, "can't find device information: pci_id=0x%x\n", aub_pci_id);
               if (read->info)
                  }
      static bool
   handle_trace_block(struct aub_read *read, const uint32_t *p)
   {
      int operation = p[1] & AUB_TRACE_OPERATION_MASK;
   int type = p[1] & AUB_TRACE_TYPE_MASK;
   int address_space = p[1] & AUB_TRACE_ADDRESS_SPACE_MASK;
   int header_length = p[0] & 0xffff;
   enum intel_engine_class engine = INTEL_ENGINE_CLASS_RENDER;
   const void *data = p + header_length + 2;
   uint64_t address = intel_48b_address((read->devinfo.ver >= 8 ? ((uint64_t) p[5] << 32) : 0) |
                  switch (operation) {
   case AUB_TRACE_OP_DATA_WRITE:
      if (address_space == AUB_TRACE_MEMTYPE_GTT) {
      if (read->local_write)
         case AUB_TRACE_OP_COMMAND_WRITE:
      switch (type) {
   case AUB_TRACE_TYPE_RING_PRB0:
      engine = INTEL_ENGINE_CLASS_RENDER;
      case AUB_TRACE_TYPE_RING_PRB1:
      engine = INTEL_ENGINE_CLASS_VIDEO;
      case AUB_TRACE_TYPE_RING_PRB2:
      engine = INTEL_ENGINE_CLASS_COPY;
      default:
      parse_error(read, p, "command write to unknown ring %d\n", type);
               if (read->ring_write)
         break;
                  }
      static void
   handle_memtrace_reg_write(struct aub_read *read, const uint32_t *p)
   {
      uint32_t offset = p[1];
            if (read->reg_write)
            enum intel_engine_class engine;
            switch (offset) {
   case RCSUNIT(EXECLIST_SUBMITPORT): /* render elsp */
      read->render_elsp[read->render_elsp_index++] = value;
   if (read->render_elsp_index < 4)
            read->render_elsp_index = 0;
   engine = INTEL_ENGINE_CLASS_RENDER;
   context_descriptor = (uint64_t)read->render_elsp[2] << 32 |
            case VCSUNIT0(EXECLIST_SUBMITPORT): /* video elsp */
      read->video_elsp[read->video_elsp_index++] = value;
   if (read->video_elsp_index < 4)
            read->video_elsp_index = 0;
   engine = INTEL_ENGINE_CLASS_VIDEO;
   context_descriptor = (uint64_t)read->video_elsp[2] << 32 |
            case BCSUNIT0(EXECLIST_SUBMITPORT): /* blitter elsp */
      read->blitter_elsp[read->blitter_elsp_index++] = value;
   if (read->blitter_elsp_index < 4)
            read->blitter_elsp_index = 0;
   engine = INTEL_ENGINE_CLASS_COPY;
   context_descriptor = (uint64_t)read->blitter_elsp[2] << 32 |
            case RCSUNIT(EXECLIST_SQ_CONTENTS): /* render elsq0 lo */
      read->render_elsp[3] = value;
      case RCSUNIT(EXECLIST_SQ_CONTENTS) + 4: /* render elsq0 hi */
      read->render_elsp[2] = value;
      case VCSUNIT0(EXECLIST_SQ_CONTENTS): /* video elsq0 lo */
      read->video_elsp[3] = value;
      case VCSUNIT0(EXECLIST_SQ_CONTENTS) + 4: /* video elsq0 hi */
      read->video_elsp[2] = value;
      case BCSUNIT0(EXECLIST_SQ_CONTENTS): /* blitter elsq0 lo */
      read->blitter_elsp[3] = value;
      case BCSUNIT0(EXECLIST_SQ_CONTENTS) + 4: /* blitter elsq0 hi */
      read->blitter_elsp[2] = value;
      case RCSUNIT(EXECLIST_CONTROL): /* render elsc */
      engine = INTEL_ENGINE_CLASS_RENDER;
   context_descriptor = (uint64_t)read->render_elsp[2] << 32 |
            case VCSUNIT0(EXECLIST_CONTROL): /* video_elsc */
      engine = INTEL_ENGINE_CLASS_VIDEO;
   context_descriptor = (uint64_t)read->video_elsp[2] << 32 |
            case BCSUNIT0(EXECLIST_CONTROL): /* blitter elsc */
      engine = INTEL_ENGINE_CLASS_COPY;
   context_descriptor = (uint64_t)read->blitter_elsp[2] << 32 |
            default:
                  if (read->execlist_write)
      }
      static void
   do_write(struct aub_read *read, uint32_t address_space, uint64_t addr, const void *data, uint32_t size)
   {
      if (0)
            switch (address_space) {
   case 0: /* GGTT */
      if (read->ggtt_write)
            case 1: /* Local */
      if (read->local_write)
            case 2: /* Physical */
      if (read->phys_write)
            case 4: /* GGTT Entry */
      if (read->ggtt_entry_write)
               }
      static void
   handle_memtrace_mem_write(struct aub_read *read, const uint32_t *p)
   {
      const void *data = p + 5;
   uint64_t addr = intel_48b_address(*(uint64_t*)&p[1]);
   uint32_t size = p[4];
               }
      static void
   handle_memtrace_mem_write_discont(struct aub_read *read, const uint32_t *p)
   {
      uint32_t address_space = p[1] >> 28;
   const struct {
      uint64_t address;
      } __attribute__((packed)) *cur = (const void *)(p + 2);
            for (unsigned i = 0; i < 63; ++i, ++cur) {
      uint64_t addr = intel_48b_address(cur->address);
            if (size == 0)
                  }
      int
   aub_read_command(struct aub_read *read, const void *data, uint32_t data_len)
   {
      const uint32_t *p = data, *next;
   ASSERTED const uint32_t *end = data + data_len;
                     h = *p;
            switch (OPCODE(h)) {
   case OPCODE_AUB:
      bias = 2;
      case OPCODE_NEW_AUB:
      bias = 1;
      default:
      parse_error(read, data, "unknown opcode %d\n", OPCODE(h));
               next = p + header_length + bias;
   if ((h & 0xffff0000) == MAKE_HEADER(TYPE_AUB, OPCODE_AUB, SUBOPCODE_BLOCK)) {
      assert(end - p >= 4);
               if (next > end) {
      parse_error(read, data,
         "input ends unexpectedly (command length: %zu, remaining bytes: %zu)\n",
   (uintptr_t)next - (uintptr_t)data,
               if (0) {
      fprintf(stderr, "0x%x, 0x%x, 0x%x, len: %d\n",
         for (const uint32_t *cur = p; cur < next; ++cur)
                     switch (h & 0xffff0000) {
   case MAKE_HEADER(TYPE_AUB, OPCODE_AUB, SUBOPCODE_HEADER):
      if (!handle_trace_header(read, p))
            case MAKE_HEADER(TYPE_AUB, OPCODE_AUB, SUBOPCODE_BLOCK):
      if (!handle_trace_block(read, p))
            case MAKE_HEADER(TYPE_AUB, OPCODE_AUB, SUBOPCODE_BMP):
         case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_VERSION_EXT):
      if (!handle_memtrace_version(read, p))
            case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_REG_WRITE):
      handle_memtrace_reg_write(read, p);
      case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_MEM_WRITE):
      handle_memtrace_mem_write(read, p);
      case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_MEM_POLL):
      /* fprintf(outfile, "memory poll block (dwords %d):\n", h & 0xffff); */
      case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_REG_POLL):
         case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_COMMENT):
      if (read->comment)
            case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_MEM_WRITE_DISCONT):
      handle_memtrace_mem_write_discont(read, p);
      case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_VERSION):
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_REG_CMP):
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_MEM_CMP):
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_FRAME_BEGIN):
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_TRACE_DELAY):
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_MEM_DUMP):
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_TEST_PHASE_MARKER):
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_MEM_CONT_REGION):
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_PREDICATE):
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_DUMP_COMPRESS):
   default:
      parse_error(read, p,
                              }
