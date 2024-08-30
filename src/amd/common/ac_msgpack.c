   /*
   * Copyright 2021 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      /**
   * \file ac_msgpack.c
   *
   * This file provides functions to create msgpack formatted data.
   * for msgpack specification refer to
   * github.com/msgpack/msgpack/blob/master/spec.md
   */
      #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <stdbool.h>
   #include <string.h>
   #include "util/u_math.h"
   #include "ac_msgpack.h"
      #define MSGPACK_MEM_START_SIZE 0x1000
   #define MSGPACK_MEM_INC_SIZE 0x1000
      #define MSGPACK_FIXMAP_OP 0x80
   #define MSGPACK_MAP16_OP 0xde
   #define MSGPACK_MAP32_OP 0xdf
      #define MSGPACK_FIXARRAY_OP 0x90
   #define MSGPACK_ARRAY16_OP 0xdc
   #define MSGPACK_ARRAY32_OP 0xdd
      #define MSGPACK_FIXSTR_OP 0xa0
   #define MSGPACK_STR8_OP 0xd9
   #define MSGPACK_STR16_OP 0xda
   #define MSGPACK_STR32_OP 0xdb
      #define MSGPACK_UINT8_OP 0xcc
   #define MSGPACK_UINT16_OP 0xcd
   #define MSGPACK_UINT32_OP 0xce
   #define MSGPACK_UINT64_OP 0xcf
      #define MSGPACK_NIL_OP 0xc0
      #define MSGPACK_INT8_OP 0xd0
   #define MSGPACK_INT16_OP 0xd1
   #define MSGPACK_INT32_OP 0xd2
   #define MSGPACK_INT64_OP 0xd3
         void ac_msgpack_init(struct ac_msgpack *msgpack)
   {
      msgpack->mem = malloc(MSGPACK_MEM_START_SIZE);
   msgpack->mem_size = MSGPACK_MEM_START_SIZE;
      }
      void ac_msgpack_destroy(struct ac_msgpack *msgpack)
   {
         }
      int ac_msgpack_resize_if_required(struct ac_msgpack *msgpack,
         {
      if ((msgpack->offset + data_size) > msgpack->mem_size) {
               new_mem_size = msgpack->mem_size +
         msgpack->mem = realloc(msgpack->mem, new_mem_size);
   if (msgpack->mem == NULL)
                           }
      void ac_msgpack_add_fixmap_op(struct ac_msgpack *msgpack, uint32_t n)
   {
      if (n <= 0xf ) {
      if (!ac_msgpack_resize_if_required(msgpack, 1))
         msgpack->mem[msgpack->offset] = MSGPACK_FIXMAP_OP | n;
      } else if (n <= 0xffff) {
      if (!ac_msgpack_resize_if_required(msgpack, 3))
         msgpack->mem[msgpack->offset] = MSGPACK_MAP16_OP;
   *((uint16_t*)&msgpack->mem[msgpack->offset + 1]) = util_bswap16(n);
      } else {
      if (!ac_msgpack_resize_if_required(msgpack, 5))
         msgpack->mem[msgpack->offset] = MSGPACK_MAP32_OP;
   *((unsigned int*)&msgpack->mem[msgpack->offset + 1]) = util_bswap32(n);
         }
      void ac_msgpack_add_fixarray_op(struct ac_msgpack *msgpack, uint32_t n)
   {
      if (n <= 0xf ) {
      if (!ac_msgpack_resize_if_required(msgpack, 1))
         msgpack->mem[msgpack->offset] = MSGPACK_FIXARRAY_OP | n;
      } else if (n <= 0xffff) {
      if (!ac_msgpack_resize_if_required(msgpack, 3))
         msgpack->mem[msgpack->offset] = MSGPACK_ARRAY16_OP;
   *((uint16_t*)&msgpack->mem[msgpack->offset + 1]) = util_bswap16(n);
      } else {
      if (!ac_msgpack_resize_if_required(msgpack, 5))
         msgpack->mem[msgpack->offset] = MSGPACK_ARRAY32_OP;
   *((uint32_t*)&msgpack->mem[msgpack->offset + 1]) = util_bswap32(n);
         }
      void ac_msgpack_add_fixstr(struct ac_msgpack *msgpack, const char *str)
   {
                        if (n <= 0x1f) {
      if (!ac_msgpack_resize_if_required(msgpack, 1 + n))
         msgpack->mem[msgpack->offset] = MSGPACK_FIXSTR_OP | n;
      } else if (n <= 0xff) {
      if (!ac_msgpack_resize_if_required(msgpack, 2 + n))
         msgpack->mem[msgpack->offset] = MSGPACK_STR8_OP;
   msgpack->mem[msgpack->offset + 1] = n;
      } else if (n <= 0xffff) {
      if (!ac_msgpack_resize_if_required(msgpack, 3 + n))
         msgpack->mem[msgpack->offset] = MSGPACK_STR16_OP;
   *((uint16_t*)&msgpack->mem[msgpack->offset + 1]) = util_bswap16(n);
      } else {
      if (!ac_msgpack_resize_if_required(msgpack, 5 + n))
         msgpack->mem[msgpack->offset] = MSGPACK_STR32_OP;
   *((uint32_t*)&msgpack->mem[msgpack->offset + 1]) = util_bswap32(n);
               memcpy (&msgpack->mem[msgpack->offset], str, n);
      }
      void ac_msgpack_add_uint(struct ac_msgpack *msgpack, uint64_t val)
   {
      if (val <= 0x7f) {
      if (!ac_msgpack_resize_if_required(msgpack, 1))
         msgpack->mem[msgpack->offset] = val;
      } else if (val <= 0xff) {
      if (!ac_msgpack_resize_if_required(msgpack, 2))
         msgpack->mem[msgpack->offset] = MSGPACK_UINT8_OP;
   msgpack->mem[msgpack->offset + 1] = val;
      } else if (val <= 0xffff) {
      if (!ac_msgpack_resize_if_required(msgpack, 3))
         msgpack->mem[msgpack->offset] = MSGPACK_UINT16_OP;
   *((uint16_t*)&msgpack->mem[msgpack->offset + 1]) = util_bswap16(val);
      } else if (val <= 0xffffffff) {
      if (!ac_msgpack_resize_if_required(msgpack, 5))
         msgpack->mem[msgpack->offset] = MSGPACK_UINT32_OP;
   *((uint32_t*)&msgpack->mem[msgpack->offset + 1]) = util_bswap32(val);
      } else {
      if (!ac_msgpack_resize_if_required(msgpack, 9))
         msgpack->mem[msgpack->offset] = MSGPACK_UINT64_OP;
   *((uint64_t*)&msgpack->mem[msgpack->offset + 1]) = util_bswap64(val);
         }
      void ac_msgpack_add_int(struct ac_msgpack *msgpack, int64_t val)
   {
      if ((val >= -0x7f) && (val <= 0x7f)) {
      if ((val >= -31) && (val < 0)) {
      if (!ac_msgpack_resize_if_required(msgpack, 1))
         msgpack->mem[msgpack->offset] = val | MSGPACK_NIL_OP;
      } else if ((val >= 0) && (val <= 127)) {
      if (!ac_msgpack_resize_if_required(msgpack, 1))
         msgpack->mem[msgpack->offset] = val;
      } else {
      if (!ac_msgpack_resize_if_required(msgpack, 2))
         msgpack->mem[msgpack->offset] = MSGPACK_INT8_OP;
   msgpack->mem[msgpack->offset + 1] = val;
         } else if ((val >= -0x7fff) && (val <= 0x7fff)) {
      if (!ac_msgpack_resize_if_required(msgpack, 3))
         msgpack->mem[msgpack->offset] = MSGPACK_INT16_OP;
   *((int16_t*)&msgpack->mem[msgpack->offset + 1]) = util_bswap32(val);
      } else if ((val >= -0x7fffffff) && (val <= 0x7fffffff)) {
      if (!ac_msgpack_resize_if_required(msgpack, 5))
         msgpack->mem[msgpack->offset] = MSGPACK_INT32_OP;
   *((int32_t*)&msgpack->mem[msgpack->offset + 1]) = util_bswap32(val);
      } else {
      if (!ac_msgpack_resize_if_required(msgpack, 9))
         msgpack->mem[msgpack->offset] = MSGPACK_INT64_OP;
   *((int64_t*)&msgpack->mem[msgpack->offset + 1]) = util_bswap64(val);
         }
