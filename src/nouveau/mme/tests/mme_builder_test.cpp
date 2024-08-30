   /*
   * Copyright © 2022 Collabora Ltd.
   * SPDX-License-Identifier: MIT
   */
   #include "mme_runner.h"
   #include "mme_tu104_sim.h"
      #include <vector>
      #include "nvk_clc597.h"
      class mme_builder_test : public ::testing::Test {
   public:
      mme_builder_test();
            std::vector<mme_runner *> sims;
         private:
      mme_fermi_sim_runner fermi_sim;
      };
      #define DATA_ADDR 0xc0ffee00
      mme_builder_test::mme_builder_test() :
      fermi_sim(DATA_ADDR),
      {
      memset(expected, 0, sizeof(expected));
   sims.push_back(&fermi_sim);
      }
      mme_builder_test::~mme_builder_test()
   { }
      #define ASSERT_SIM_DATA(sim) do {               \
      for (uint32_t i = 0; i < DATA_DWORDS; i++)   \
      } while (0)
      TEST_F(mme_builder_test, sanity)
   {
                        for (auto sim : sims) {
      mme_builder b;
                              std::vector<uint32_t> params;
   sim->run_macro(macro, params);
         }
      static uint32_t
   merge(uint32_t x, uint32_t y,
         {
      x &= ~(BITFIELD_MASK(bits) << dst_pos);
   y &= (BITFIELD_MASK(bits) << src_pos);
      }
      static const uint32_t add_cases[] = {
      0x00000001,
   0xffffffff,
   0x0000ffff,
   0x00008000,
   0x0001ffff,
   0xffff8000,
   0x00010000,
   0x00020000,
   0xfffc0000,
      };
      TEST_F(mme_builder_test, add)
   {
      for (auto sim : sims) {
      mme_builder b;
            mme_value x = mme_load(&b);
                              for (uint32_t i = 0; i < ARRAY_SIZE(add_cases); i++) {
      for (uint32_t j = 0; j < ARRAY_SIZE(add_cases); j++) {
      std::vector<uint32_t> params;
                  sim->run_macro(macro, params);
               }
      TEST_F(mme_builder_test, add_imm)
   {
      for (auto sim : sims) {
      mme_builder b;
                     for (uint32_t j = 0; j < ARRAY_SIZE(add_cases); j++) {
      mme_value y = mme_imm(add_cases[j]);
                        for (uint32_t i = 0; i < ARRAY_SIZE(add_cases); i++) {
                              for (uint32_t j = 0; j < ARRAY_SIZE(add_cases); j++)
            }
      TEST_F(mme_builder_test, sub)
   {
      for (auto sim : sims) {
      mme_builder b;
            mme_value x = mme_load(&b);
                              for (uint32_t i = 0; i < ARRAY_SIZE(add_cases); i++) {
      for (uint32_t j = 0; j < ARRAY_SIZE(add_cases); j++) {
      std::vector<uint32_t> params;
                  sim->run_macro(macro, params);
               }
      TEST_F(mme_builder_test, sub_imm)
   {
      for (auto sim : sims) {
      mme_builder b;
                     for (uint32_t j = 0; j < ARRAY_SIZE(add_cases); j++) {
      mme_value y = mme_imm(add_cases[j]);
                        for (uint32_t i = 0; i < ARRAY_SIZE(add_cases); i++) {
                              for (uint32_t j = 0; j < ARRAY_SIZE(add_cases); j++)
            }
      TEST_F(mme_builder_test, sll_srl)
   {
               for (auto sim : sims) {
      mme_builder b;
            mme_value xv = mme_load(&b);
            sim->mme_store_data(&b, 0, mme_sll(&b, xv, yv));
                     /* Fermi can't shift by 0 */
   for (uint32_t i = 1; i < 31; i++) {
      std::vector<uint32_t> params;
                  sim->run_macro(macro, params);
   ASSERT_EQ(sim->data[0], x << i);
            }
      TEST_F(mme_builder_test, merge)
   {
      static const struct {
      uint16_t dst_pos;
   uint16_t bits;
      } cases[] = {
      { 12, 12, 20 },
   { 12, 8,  20 },
   { 8,  12, 20 },
   { 12, 16, 8 },
               static const uint32_t x = 0x0c406fe0;
            for (uint32_t i = 0; i < ARRAY_SIZE(cases); i++) {
      expected[i] = merge(x, y, cases[i].dst_pos,
               for (auto sim : sims) {
      mme_builder b;
            mme_value xv = mme_load(&b);
            for (uint32_t i = 0; i < ARRAY_SIZE(cases); i++) {
      mme_value mv = mme_merge(&b, xv, yv, cases[i].dst_pos,
                              std::vector<uint32_t> params;
   params.push_back(x);
            sim->run_macro(macro, params);
         }
      TEST_F(mme_builder_test, while_ine)
   {
               for (auto sim : sims) {
      mme_builder b;
            mme_value inc = mme_load(&b);
            mme_value x = mme_mov(&b, mme_zero());
   mme_value y = mme_mov(&b, mme_zero());
            mme_value i = mme_mov(&b, mme_zero());
   mme_while(&b, ine, i, bound) {
      mme_add_to(&b, x, x, mme_imm(1));
   mme_add_to(&b, y, y, mme_imm(2));
   mme_add_to(&b, z, z, mme_imm(3));
               sim->mme_store_data(&b, 0, x);
   sim->mme_store_data(&b, 1, y);
   sim->mme_store_data(&b, 2, z);
                     for (unsigned i = 0; i < ARRAY_SIZE(cases); i++) {
      const uint32_t inc = cases[i];
                  std::vector<uint32_t> params;
                  expected[0] = count * 1;
   expected[1] = count * 2;
                  sim->run_macro(macro, params);
            }
