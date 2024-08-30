   /*
   * Copyright 2021 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include "agx_test.h"
      #include <gtest/gtest.h>
      static void
   agx_optimize_and_dce(agx_context *ctx)
   {
      agx_optimizer(ctx);
      }
      #define CASE(instr, expected, size, returns)                                   \
      INSTRUCTION_CASE(                                                           \
      {                                                                        \
      UNUSED agx_index out = agx_temp(b->shader, AGX_SIZE_##size);          \
   instr;                                                                \
   if (returns)                                                          \
      },                                                                       \
   {                                                                        \
      UNUSED agx_index out = agx_temp(b->shader, AGX_SIZE_##size);          \
   expected;                                                             \
   if (returns)                                                          \
      },                                                                       \
      #define NEGCASE(instr, size) CASE(instr, instr, size, true)
      #define CASE16(instr, expected) CASE(instr, expected, 16, true)
   #define CASE32(instr, expected) CASE(instr, expected, 32, true)
      #define CASE_NO_RETURN(instr, expected)                                        \
            #define NEGCASE16(instr) NEGCASE(instr, 16)
   #define NEGCASE32(instr) NEGCASE(instr, 32)
      static inline agx_index
   agx_fmov(agx_builder *b, agx_index s0)
   {
      agx_index tmp = agx_temp(b->shader, s0.size);
   agx_fmov_to(b, tmp, s0);
      }
      class Optimizer : public testing::Test {
   protected:
      Optimizer()
   {
               wx = agx_register(0, AGX_SIZE_32);
   wy = agx_register(2, AGX_SIZE_32);
            hx = agx_register(0, AGX_SIZE_16);
   hy = agx_register(1, AGX_SIZE_16);
               ~Optimizer()
   {
                              };
      TEST_F(Optimizer, FloatCopyprop)
   {
      CASE32(agx_fadd_to(b, out, agx_abs(agx_fmov(b, wx)), wy),
            CASE32(agx_fadd_to(b, out, agx_neg(agx_fmov(b, wx)), wy),
      }
      TEST_F(Optimizer, FloatConversion)
   {
      CASE32(
      {
      agx_index cvt = agx_temp(b->shader, AGX_SIZE_32);
   agx_fmov_to(b, cvt, hx);
      },
         CASE16(
      {
      agx_index sum = agx_temp(b->shader, AGX_SIZE_32);
   agx_fadd_to(b, sum, wx, wy);
      },
   }
      TEST_F(Optimizer, FusedFABSNEG)
   {
      CASE32(agx_fadd_to(b, out, agx_fmov(b, agx_abs(wx)), wy),
            CASE32(agx_fmul_to(b, out, wx, agx_fmov(b, agx_neg(agx_abs(wx)))),
      }
      TEST_F(Optimizer, FusedFabsAbsorb)
   {
      CASE32(agx_fadd_to(b, out, agx_abs(agx_fmov(b, agx_abs(wx))), wy),
      }
      TEST_F(Optimizer, FusedFnegCancel)
   {
      CASE32(agx_fmul_to(b, out, wx, agx_neg(agx_fmov(b, agx_neg(wx)))),
            CASE32(agx_fmul_to(b, out, wx, agx_neg(agx_fmov(b, agx_neg(agx_abs(wx))))),
      }
      TEST_F(Optimizer, FmulFsatF2F16)
   {
      CASE16(
      {
      agx_index tmp = agx_temp(b->shader, AGX_SIZE_32);
   agx_fmov_to(b, tmp, agx_fmul(b, wx, wy))->saturate = true;
      },
   }
      TEST_F(Optimizer, Copyprop)
   {
      CASE32(agx_fmul_to(b, out, wx, agx_mov(b, wy)), agx_fmul_to(b, out, wx, wy));
   CASE32(agx_fmul_to(b, out, agx_mov(b, wx), agx_mov(b, wy)),
      }
      TEST_F(Optimizer, InlineHazards)
   {
      NEGCASE32({
      agx_instr *I = agx_collect_to(b, out, 4);
   I->src[0] = agx_mov_imm(b, AGX_SIZE_32, 0);
   I->src[1] = wy;
   I->src[2] = wz;
         }
      TEST_F(Optimizer, CopypropRespectsAbsNeg)
   {
      CASE32(agx_fadd_to(b, out, agx_abs(agx_mov(b, wx)), wy),
            CASE32(agx_fadd_to(b, out, agx_neg(agx_mov(b, wx)), wy),
            CASE32(agx_fadd_to(b, out, agx_neg(agx_abs(agx_mov(b, wx))), wy),
      }
      TEST_F(Optimizer, IntCopyprop)
   {
         }
      TEST_F(Optimizer, IntCopypropDoesntConvert)
   {
      NEGCASE32({
      agx_index cvt = agx_temp(b->shader, AGX_SIZE_32);
   agx_mov_to(b, cvt, hx);
         }
      TEST_F(Optimizer, SkipPreloads)
   {
      NEGCASE32({
      agx_index preload = agx_preload(b, agx_register(0, AGX_SIZE_32));
         }
      TEST_F(Optimizer, NoConversionsOn16BitALU)
   {
      NEGCASE16({
      agx_index cvt = agx_temp(b->shader, AGX_SIZE_16);
   agx_fmov_to(b, cvt, wx);
                  }
      TEST_F(Optimizer, IfCondition)
   {
      CASE_NO_RETURN(agx_if_icmp(b, agx_icmp(b, wx, wy, AGX_ICOND_UEQ, true),
                  CASE_NO_RETURN(agx_if_icmp(b, agx_fcmp(b, wx, wy, AGX_FCOND_EQ, true),
                  CASE_NO_RETURN(agx_if_icmp(b, agx_fcmp(b, hx, hy, AGX_FCOND_LT, false),
            }
      TEST_F(Optimizer, SelectCondition)
   {
      CASE32(agx_icmpsel_to(b, out, agx_icmp(b, wx, wy, AGX_ICOND_UEQ, false),
                  CASE32(agx_icmpsel_to(b, out, agx_icmp(b, wx, wy, AGX_ICOND_UEQ, true),
                  CASE32(agx_icmpsel_to(b, out, agx_fcmp(b, wx, wy, AGX_FCOND_EQ, false),
                  CASE32(agx_icmpsel_to(b, out, agx_fcmp(b, wx, wy, AGX_FCOND_LT, true),
            }
