   /*
   * Copyright © 2022 Mary Guillemard
   * SPDX-License-Identifier: MIT
   */
   #include "mme_runner.h"
      #include "mme_fermi_sim.h"
   /* for VOLTA_A */
   #include "nvk_clc397.h"
      class mme_fermi_sim_test : public ::testing::Test, public mme_hw_runner {
   public:
      mme_fermi_sim_test();
            void SetUp();
   void test_macro(const mme_builder *b,
            };
      mme_fermi_sim_test::mme_fermi_sim_test() :
      ::testing::Test(),
      { }
      mme_fermi_sim_test::~mme_fermi_sim_test()
   { }
      void
   mme_fermi_sim_test::SetUp()
   {
         }
      void
   mme_fermi_sim_test::test_macro(const mme_builder *b,
               {
               std::vector<mme_fermi_inst> insts(macro.size());
            /* First, make a copy of the data and simulate the macro */
   std::vector<uint32_t> sim_data(data, data + (DATA_BO_SIZE / 4));
   mme_fermi_sim_mem sim_mem = {
      .addr = data_addr,
   .data = &sim_data[0],
      };
   mme_fermi_sim(insts.size(), &insts[0],
                           /* Check the results */
   for (uint32_t i = 0; i < data_dwords; i++)
      }
      static mme_fermi_reg
   mme_fermi_value_as_reg(mme_value val)
   {
      assert(val.type == MME_VALUE_TYPE_REG);
      }
      TEST_F(mme_fermi_sim_test, sanity)
   {
               mme_builder b;
                              std::vector<uint32_t> params;
      }
      TEST_F(mme_fermi_sim_test, add)
   {
      mme_builder b;
            mme_value x = mme_load(&b);
   mme_value y = mme_load(&b);
   mme_value sum = mme_add(&b, x, y);
                     std::vector<uint32_t> params;
   params.push_back(25);
               }
      TEST_F(mme_fermi_sim_test, add_imm)
   {
      mme_builder b;
                     mme_value v0 = mme_add(&b, x, mme_imm(0x00000001));
            mme_value v1 = mme_add(&b, x, mme_imm(0xffffffff));
            mme_value v2 = mme_add(&b, x, mme_imm(0xffff8000));
            mme_value v3 = mme_add(&b, mme_imm(0x00000001), x);
            mme_value v4 = mme_add(&b, mme_imm(0xffffffff), x);
            mme_value v5 = mme_add(&b, mme_imm(0xffff8000), x);
            mme_value v6 = mme_add(&b, mme_zero(), mme_imm(0x00000001));
            mme_value v7 = mme_add(&b, mme_zero(), mme_imm(0xffffffff));
            mme_value v8 = mme_add(&b, mme_zero(), mme_imm(0xffff8000));
                     uint32_t vals[] = {
      0x0000ffff,
   0x00008000,
   0x0001ffff,
               for (uint32_t i = 0; i < ARRAY_SIZE(vals); i++) {
               std::vector<uint32_t> params;
                  }
      TEST_F(mme_fermi_sim_test, add_imm_no_carry)
   {
      mme_builder b;
            mme_value x_lo = mme_load(&b);
            mme_value v1_lo = mme_alloc_reg(&b);
   mme_value v1_hi = mme_alloc_reg(&b);
   mme_fermi_asm(&b, i) {
      i.op = MME_FERMI_OP_ADD_IMM;
   i.assign_op = MME_FERMI_ASSIGN_OP_MOVE;
   i.dst = mme_fermi_value_as_reg(v1_lo);
   i.src[0] = mme_fermi_value_as_reg(x_lo);
               mme_fermi_asm(&b, i) {
      i.op = MME_FERMI_OP_ADD_IMM;
   i.assign_op = MME_FERMI_ASSIGN_OP_MOVE;
   i.dst = mme_fermi_value_as_reg(v1_hi);
   i.src[0] = mme_fermi_value_as_reg(x_hi);
      }
   mme_store_imm_addr(&b, data_addr + 0,  v1_lo, true);
            mme_value v2_lo = mme_alloc_reg(&b);
   mme_value v2_hi = mme_alloc_reg(&b);
   mme_fermi_asm(&b, i) {
      i.op = MME_FERMI_OP_ADD_IMM;
   i.assign_op = MME_FERMI_ASSIGN_OP_MOVE;
   i.dst = mme_fermi_value_as_reg(v2_lo);
   i.src[0] = mme_fermi_value_as_reg(x_lo);
               mme_fermi_asm(&b, i) {
      i.op = MME_FERMI_OP_ADD_IMM;
   i.assign_op = MME_FERMI_ASSIGN_OP_MOVE;
   i.dst = mme_fermi_value_as_reg(v2_hi);
   i.src[0] = mme_fermi_value_as_reg(x_hi);
      }
   mme_store_imm_addr(&b, data_addr + 8,  v2_lo, true);
            mme_value v3_lo = mme_alloc_reg(&b);
   mme_value v3_hi = mme_alloc_reg(&b);
   mme_fermi_asm(&b, i) {
      i.op = MME_FERMI_OP_ADD_IMM;
   i.assign_op = MME_FERMI_ASSIGN_OP_MOVE;
   i.dst = mme_fermi_value_as_reg(v2_lo);
   i.src[0] = mme_fermi_value_as_reg(x_lo);
               mme_fermi_asm(&b, i) {
      i.op = MME_FERMI_OP_ADD_IMM;
   i.assign_op = MME_FERMI_ASSIGN_OP_MOVE;
   i.dst = mme_fermi_value_as_reg(v2_hi);
   i.src[0] = mme_fermi_value_as_reg(x_hi);
      }
   mme_store_imm_addr(&b, data_addr + 16, v3_lo, true);
            mme_value v4_lo = mme_alloc_reg(&b);
   mme_value v4_hi = mme_alloc_reg(&b);
   mme_fermi_asm(&b, i) {
      i.op = MME_FERMI_OP_ADD_IMM;
   i.assign_op = MME_FERMI_ASSIGN_OP_MOVE;
   i.dst = mme_fermi_value_as_reg(v2_lo);
   i.src[0] = mme_fermi_value_as_reg(x_lo);
               mme_fermi_asm(&b, i) {
      i.op = MME_FERMI_OP_ADD_IMM;
   i.assign_op = MME_FERMI_ASSIGN_OP_MOVE;
   i.dst = mme_fermi_value_as_reg(v2_hi);
   i.src[0] = mme_fermi_value_as_reg(x_hi);
      }
   mme_store_imm_addr(&b, data_addr + 24, v4_lo, true);
                     uint64_t vals[] = {
      0x0000ffffffffffffull,
   0x0000ffffffff8000ull,
   0x0000ffff00000000ull,
   0x0000800000000000ull,
   0x00008000ffffffffull,
   0x0001ffff00000000ull,
   0xffffffff00000000ull,
               for (uint32_t i = 0; i < ARRAY_SIZE(vals); i++) {
               std::vector<uint32_t> params;
   params.push_back(low32(vals[i]));
                  }
      TEST_F(mme_fermi_sim_test, addc)
   {
      mme_builder b;
            struct mme_value64 x = { mme_load(&b), mme_load(&b) };
                     mme_store_imm_addr(&b, data_addr + 0, sum.lo, true);
                     std::vector<uint32_t> params;
   params.push_back(0x80008650);
   params.push_back(0x596);
   params.push_back(0x8000a8f6);
               }
      TEST_F(mme_fermi_sim_test, sub)
   {
      mme_builder b;
            mme_value x = mme_load(&b);
   mme_value y = mme_load(&b);
   mme_value diff = mme_sub(&b, x, y);
                     std::vector<uint32_t> params;
   params.push_back(25);
               }
      TEST_F(mme_fermi_sim_test, subb)
   {
      mme_builder b;
            struct mme_value64 x = { mme_load(&b), mme_load(&b) };
                     mme_store_imm_addr(&b, data_addr + 0, sum.lo, true);
                     std::vector<uint32_t> params;
   params.push_back(0x80008650);
   params.push_back(0x596);
   params.push_back(0x8000a8f6);
               }
      #define SHIFT_TEST(op)                                                     \
   TEST_F(mme_fermi_sim_test, op)                                             \
   {                                                                          \
      mme_builder b;                                                          \
   mme_builder_init(&b, devinfo);                                       \
         mme_value val = mme_load(&b);                                           \
   mme_value shift1 = mme_load(&b);                                        \
   mme_value shift2 = mme_load(&b);                                        \
   mme_store_imm_addr(&b, data_addr + 0, mme_##op(&b, val, shift1), true); \
   mme_store_imm_addr(&b, data_addr + 4, mme_##op(&b, val, shift2), true); \
         auto macro = mme_builder_finish_vec(&b);                                \
         std::vector<uint32_t> params;                                           \
   params.push_back(0x0c406fe0);                                           \
   params.push_back(5);                                                    \
   params.push_back(51);                                                   \
            }
      SHIFT_TEST(sll)
   SHIFT_TEST(srl)
      #undef SHIFT_TEST
      TEST_F(mme_fermi_sim_test, bfe)
   {
               mme_builder b;
            mme_value val = mme_load(&b);
            mme_store_imm_addr(&b, data_addr + 0, mme_bfe(&b, val, pos, 1), true);
   mme_store_imm_addr(&b, data_addr + 4, mme_bfe(&b, val, pos, 2), true);
                     for (unsigned i = 0; i < 31; i++) {
      std::vector<uint32_t> params;
   params.push_back(canary);
                     ASSERT_EQ(data[0], (canary >> i) & 0x1);
   ASSERT_EQ(data[1], (canary >> i) & 0x3);
         }
      #define BITOP_TEST(op)                                   \
   TEST_F(mme_fermi_sim_test, op)                           \
   {                                                        \
      mme_builder b;                                        \
   mme_builder_init(&b, devinfo);                     \
         mme_value x = mme_load(&b);                           \
   mme_value y = mme_load(&b);                           \
   mme_value v1 = mme_##op(&b, x, y);                    \
   mme_value v2 = mme_##op(&b, x, mme_imm(0xffff8000));  \
   mme_value v3 = mme_##op(&b, x, mme_imm(0xffffffff));  \
   mme_store_imm_addr(&b, data_addr + 0, v1, true);      \
   mme_store_imm_addr(&b, data_addr + 4, v2, true);      \
   mme_store_imm_addr(&b, data_addr + 8, v3, true);      \
         auto macro = mme_builder_finish_vec(&b);              \
         std::vector<uint32_t> params;                         \
   params.push_back(0x0c406fe0);                         \
   params.push_back(0x00fff0c0);                         \
            }
      BITOP_TEST(and)
   //BITOP_TEST(and_not)
   BITOP_TEST(nand)
   BITOP_TEST(or)
   BITOP_TEST(xor)
      #undef BITOP_TEST
      static bool c_ine(int32_t x, int32_t y) { return x != y; };
   static bool c_ieq(int32_t x, int32_t y) { return x == y; };
      #define IF_TEST(op)                                                  \
   TEST_F(mme_fermi_sim_test, if_##op)                                  \
   {                                                                    \
      mme_builder b;                                                    \
   mme_builder_init(&b, devinfo);                                 \
         mme_value x = mme_load(&b);                                       \
   mme_value y = mme_load(&b);                                       \
   mme_value i = mme_mov(&b, mme_zero());                            \
         mme_start_if_##op(&b, x, y);                                      \
   {                                                                 \
      mme_add_to(&b, i, i, mme_imm(1));                              \
      }                                                                 \
   mme_end_if(&b);                                                   \
   mme_add_to(&b, i, i, mme_imm(1));                                 \
   mme_add_to(&b, i, i, mme_imm(1));                                 \
   mme_add_to(&b, i, i, mme_imm(1));                                 \
         mme_store_imm_addr(&b, data_addr + 0, i, true);                   \
         auto macro = mme_builder_finish_vec(&b);                          \
         uint32_t vals[] = {23, 56, (uint32_t)-5, (uint32_t)-10, 56, 14};  \
         for (uint32_t i = 0; i < ARRAY_SIZE(vals) - 1; i++) {             \
      reset_push();                                                  \
         std::vector<uint32_t> params;                                  \
   params.push_back(vals[i + 0]);                                 \
   params.push_back(vals[i + 1]);                                 \
         test_macro(&b, macro, params);                                 \
               }
      IF_TEST(ieq)
   IF_TEST(ine)
      #undef IF_TEST
      static inline void
   mme_fermi_inc_whole_inst(mme_builder *b, mme_value val)
   {
      mme_fermi_asm(b, i) {
      i.op = MME_FERMI_OP_ADD_IMM;
   i.assign_op = MME_FERMI_ASSIGN_OP_MOVE;
   i.dst = mme_fermi_value_as_reg(val);
   i.src[0] = mme_fermi_value_as_reg(val);
         }
      #define WHILE_TEST(op, start, step, bound)               \
   TEST_F(mme_fermi_sim_test, while_##op)                   \
   {                                                        \
      mme_builder b;                                        \
   mme_builder_init(&b, devinfo);                     \
         mme_value x = mme_mov(&b, mme_zero());                \
   mme_value y = mme_mov(&b, mme_zero());                \
   mme_value z = mme_mov(&b, mme_imm(start));            \
   mme_value w = mme_mov(&b, mme_zero());                \
   mme_value v = mme_mov(&b, mme_zero());                \
         for (uint32_t j = 0; j < 5; j++)                      \
         mme_store_imm_addr(&b, data_addr + 0, x, true);       \
         mme_while(&b, op, z, mme_imm(bound)) {                \
      for (uint32_t j = 0; j < 5; j++)                   \
      mme_fermi_inc_whole_inst(&b, y);                \
      mme_add_to(&b, z, z, mme_imm(step));               \
         for (uint32_t j = 0; j < 5; j++)                   \
      }                                                     \
   mme_store_imm_addr(&b, data_addr + 4, y, true);       \
   mme_store_imm_addr(&b, data_addr + 8, z, true);       \
   mme_store_imm_addr(&b, data_addr + 12, w, true);      \
         for (uint32_t j = 0; j < 5; j++)                      \
      mme_fermi_inc_whole_inst(&b, v);                   \
      mme_store_imm_addr(&b, data_addr + 16, v, true);      \
         auto macro = mme_builder_finish_vec(&b);              \
         uint32_t end = (uint32_t)(start), count = 0;          \
   while (c_##op(end, (bound))) {                        \
      end += (uint32_t)(step);                           \
      }                                                     \
         std::vector<uint32_t> params;                         \
   test_macro(&b, macro, params);                        \
   ASSERT_EQ(data[0], 5);                                \
   ASSERT_EQ(data[1], 5 * count);                        \
   ASSERT_EQ(data[2], end);                              \
   ASSERT_EQ(data[3], 5 * count);                        \
      }
      WHILE_TEST(ieq, 0, 5, 0)
   WHILE_TEST(ine, 0, 1, 7)
      #undef WHILE_TWST
         TEST_F(mme_fermi_sim_test, loop)
   {
      mme_builder b;
                     mme_value x = mme_mov(&b, mme_zero());
            mme_loop(&b, count) {
      mme_fermi_asm(&b, i) { } /* noop */
      }
   mme_add_to(&b, y, y, mme_imm(1));
   mme_fermi_asm(&b, i) { } /* noop */
   mme_fermi_asm(&b, i) { } /* noop */
            mme_store_imm_addr(&b, data_addr + 0,  count, true);
   mme_store_imm_addr(&b, data_addr + 4,  x, true);
                              for (uint32_t i = 0; i < ARRAY_SIZE(counts); i++) {
               std::vector<uint32_t> params;
            test_macro(&b, macro, params);
   ASSERT_EQ(data[0], counts[i]);
   ASSERT_EQ(data[1], counts[i] * counts[i]);
         }
      TEST_F(mme_fermi_sim_test, merge)
   {
      mme_builder b;
            mme_value x = mme_load(&b);
            mme_value m1 = mme_merge(&b, x, y, 12, 12, 20);
            mme_value m2 = mme_merge(&b, x, y, 12, 8,  20);
            mme_value m3 = mme_merge(&b, x, y, 8,  12, 20);
            mme_value m4 = mme_merge(&b, x, y, 12, 16, 8);
            mme_value m5 = mme_merge(&b, x, y, 24, 12, 8);
                     std::vector<uint32_t> params;
   params.push_back(0x0c406fe0);
               }
      TEST_F(mme_fermi_sim_test, branch_delay_slot)
   {
      mme_builder b;
            mme_value x = mme_load(&b);
            mme_fermi_asm(&b, i) {
      i.op = MME_FERMI_OP_BRANCH;
   i.src[0] = MME_FERMI_REG_ZERO;
   i.imm = 2;
   i.branch.no_delay = false;
                                          std::vector<uint32_t> params;
   params.push_back(3);
            test_macro(&b, macro, params);
      }
      TEST_F(mme_fermi_sim_test, state)
   {
      mme_builder b;
            mme_value x = mme_load(&b);
            mme_mthd(&b, NV9097_SET_MME_SHADOW_SCRATCH(5));
            mme_mthd(&b, NV9097_SET_MME_SHADOW_SCRATCH(8));
            mme_value y2 = mme_state(&b, NV9097_SET_MME_SHADOW_SCRATCH(8));
            mme_store_imm_addr(&b, data_addr + 0, y2, true);
                     std::vector<uint32_t> params;
   params.push_back(-10);
               }
      TEST_F(mme_fermi_sim_test, scratch_limit)
   {
               mme_builder b;
            mme_value start = mme_load(&b);
            mme_value i = mme_mov(&b, start);
   mme_loop(&b, count) {
      mme_mthd_arr(&b, NV9097_SET_MME_SHADOW_SCRATCH(0), i);
   mme_emit(&b, i);
      }
            mme_value j = mme_mov(&b, start);
   mme_free_reg(&b, start);
            mme_loop(&b, count) {
      mme_value x = mme_state_arr(&b, NV9097_SET_MME_SHADOW_SCRATCH(0), j);
   mme_store(&b, addr, x, true);
   mme_add_to(&b, j, j, mme_imm(1));
      }
   mme_free_reg(&b, j);
                     for (uint32_t i = 0; i < MME_FERMI_SCRATCH_COUNT; i += chunk_size) {
                        P_1INC(p, NV9097, CALL_MME_MACRO(0));
   P_INLINE_DATA(p, i);
                     for (uint32_t j = 0; j < chunk_size; j++)
         }
      TEST_F(mme_fermi_sim_test, load_imm_to_reg)
   {
      mme_builder b;
            uint32_t vals[] = {
      0x0001ffff,
   0x1ffff000,
   0x0007ffff,
   0x00080000,
   0x7fffffff,
   0x80000000,
               for (uint32_t i = 0; i < ARRAY_SIZE(vals); i++)
                                       for (uint32_t i = 0; i < ARRAY_SIZE(vals); i++)
      }
