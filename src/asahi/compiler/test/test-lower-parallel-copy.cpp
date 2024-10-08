   /*
   * Copyright 2021 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include "agx_test.h"
      #include <gtest/gtest.h>
      #define CASE(copies, expected)                                                 \
      do {                                                                        \
      agx_builder *A = agx_test_builder(mem_ctx);                              \
   agx_builder *B = agx_test_builder(mem_ctx);                              \
         agx_emit_parallel_copies(A, copies, ARRAY_SIZE(copies));                 \
         {                                                                        \
      agx_builder *b = B;                                                   \
      }                                                                        \
                  static inline void
   extr_swap(agx_builder *b, agx_index x)
   {
      x.size = AGX_SIZE_32;
      }
      static inline void
   xor_swap(agx_builder *b, agx_index x, agx_index y)
   {
      agx_xor_to(b, x, x, y);
   agx_xor_to(b, y, x, y);
      }
      class LowerParallelCopy : public testing::Test {
   protected:
      LowerParallelCopy()
   {
                  ~LowerParallelCopy()
   {
                     };
      TEST_F(LowerParallelCopy, UnrelatedCopies)
   {
      struct agx_copy test_1[] = {
      {.dest = 0, .src = agx_register(2, AGX_SIZE_32)},
               CASE(test_1, {
      agx_mov_to(b, agx_register(0, AGX_SIZE_32), agx_register(2, AGX_SIZE_32));
               struct agx_copy test_2[] = {
      {.dest = 0, .src = agx_register(1, AGX_SIZE_16)},
               CASE(test_2, {
      agx_mov_to(b, agx_register(0, AGX_SIZE_16), agx_register(1, AGX_SIZE_16));
         }
      TEST_F(LowerParallelCopy, RelatedSource)
   {
      struct agx_copy test_1[] = {
      {.dest = 0, .src = agx_register(2, AGX_SIZE_32)},
               CASE(test_1, {
      agx_mov_to(b, agx_register(0, AGX_SIZE_32), agx_register(2, AGX_SIZE_32));
               struct agx_copy test_2[] = {
      {.dest = 0, .src = agx_register(1, AGX_SIZE_16)},
               CASE(test_2, {
      agx_mov_to(b, agx_register(0, AGX_SIZE_16), agx_register(1, AGX_SIZE_16));
         }
      TEST_F(LowerParallelCopy, DependentCopies)
   {
      struct agx_copy test_1[] = {
      {.dest = 0, .src = agx_register(2, AGX_SIZE_32)},
               CASE(test_1, {
      agx_mov_to(b, agx_register(4, AGX_SIZE_32), agx_register(0, AGX_SIZE_32));
               struct agx_copy test_2[] = {
      {.dest = 0, .src = agx_register(1, AGX_SIZE_16)},
               CASE(test_2, {
      agx_mov_to(b, agx_register(4, AGX_SIZE_16), agx_register(0, AGX_SIZE_16));
         }
      TEST_F(LowerParallelCopy, ManyDependentCopies)
   {
      struct agx_copy test_1[] = {
      {.dest = 0, .src = agx_register(2, AGX_SIZE_32)},
   {.dest = 4, .src = agx_register(0, AGX_SIZE_32)},
   {.dest = 8, .src = agx_register(6, AGX_SIZE_32)},
               CASE(test_1, {
      agx_mov_to(b, agx_register(8, AGX_SIZE_32), agx_register(6, AGX_SIZE_32));
   agx_mov_to(b, agx_register(6, AGX_SIZE_32), agx_register(4, AGX_SIZE_32));
   agx_mov_to(b, agx_register(4, AGX_SIZE_32), agx_register(0, AGX_SIZE_32));
               struct agx_copy test_2[] = {
      {.dest = 0, .src = agx_register(1, AGX_SIZE_16)},
   {.dest = 2, .src = agx_register(0, AGX_SIZE_16)},
   {.dest = 4, .src = agx_register(3, AGX_SIZE_16)},
               CASE(test_2, {
      agx_mov_to(b, agx_register(4, AGX_SIZE_16), agx_register(3, AGX_SIZE_16));
   agx_mov_to(b, agx_register(3, AGX_SIZE_16), agx_register(2, AGX_SIZE_16));
   agx_mov_to(b, agx_register(2, AGX_SIZE_16), agx_register(0, AGX_SIZE_16));
         }
      TEST_F(LowerParallelCopy, Swap)
   {
      struct agx_copy test_1[] = {
      {.dest = 0, .src = agx_register(2, AGX_SIZE_32)},
               CASE(test_1, {
                  struct agx_copy test_2[] = {
      {.dest = 0, .src = agx_register(1, AGX_SIZE_16)},
                  }
      TEST_F(LowerParallelCopy, Cycle3)
   {
      struct agx_copy test[] = {
      {.dest = 0, .src = agx_register(1, AGX_SIZE_16)},
   {.dest = 1, .src = agx_register(2, AGX_SIZE_16)},
               CASE(test, {
      extr_swap(b, agx_register(0, AGX_SIZE_16));
         }
      /* Test case from Hack et al */
   TEST_F(LowerParallelCopy, TwoSwaps)
   {
      struct agx_copy test[] = {
      {.dest = 4, .src = agx_register(2, AGX_SIZE_32)},
   {.dest = 6, .src = agx_register(4, AGX_SIZE_32)},
   {.dest = 2, .src = agx_register(6, AGX_SIZE_32)},
               CASE(test, {
      xor_swap(b, agx_register(4, AGX_SIZE_32), agx_register(2, AGX_SIZE_32));
         }
      TEST_F(LowerParallelCopy, VectorizeAlignedHalfRegs)
   {
      struct agx_copy test[] = {
      {.dest = 0, .src = agx_register(10, AGX_SIZE_16)},
   {.dest = 1, .src = agx_register(11, AGX_SIZE_16)},
   {.dest = 2, .src = agx_uniform(8, AGX_SIZE_16)},
               CASE(test, {
      agx_mov_to(b, agx_register(0, AGX_SIZE_32),
               }
      #if 0
   TEST_F(LowerParallelCopy, LooksLikeASwap) {
      struct agx_copy test[] = {
      { .dest = 0, .src = agx_register(2, AGX_SIZE_32) },
   { .dest = 2, .src = agx_register(0, AGX_SIZE_32) },
               CASE(test, {
         agx_mov_to(b, agx_register(4, AGX_SIZE_32), agx_register(2, AGX_SIZE_32));
   agx_mov_to(b, agx_register(2, AGX_SIZE_32), agx_register(0, AGX_SIZE_32));
      }
   #endif
