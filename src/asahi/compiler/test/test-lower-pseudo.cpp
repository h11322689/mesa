   /*
   * Copyright 2021 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include "agx_test.h"
      #include <gtest/gtest.h>
      #define CASE(instr, expected)                                                  \
         #define NEGCASE(instr) CASE(instr, instr)
      class LowerPseudo : public testing::Test {
   protected:
      LowerPseudo()
   {
               wx = agx_register(0, AGX_SIZE_32);
   wy = agx_register(2, AGX_SIZE_32);
               ~LowerPseudo()
   {
                  void *mem_ctx;
      };
      TEST_F(LowerPseudo, Move)
   {
         }
      TEST_F(LowerPseudo, Not)
   {
         }
      TEST_F(LowerPseudo, BinaryBitwise)
   {
      CASE(agx_and_to(b, wx, wy, wz), agx_bitop_to(b, wx, wy, wz, 0x8));
   CASE(agx_xor_to(b, wx, wy, wz), agx_bitop_to(b, wx, wy, wz, 0x6));
      }
