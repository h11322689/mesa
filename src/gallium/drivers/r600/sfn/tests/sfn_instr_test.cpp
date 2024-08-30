      #include "../sfn_instr_alugroup.h"
   #include "../sfn_instr_export.h"
   #include "../sfn_instr_fetch.h"
   #include "../sfn_instr_lds.h"
   #include "../sfn_instr_tex.h"
      #include "gtest/gtest.h"
      using namespace r600;
      using std::vector;
      class InstrTest : public ::testing::Test {
                     protected:
         };
      TEST_F(InstrTest, test_alu_barrier)
   {
               EXPECT_FALSE(alu.has_alu_flag(alu_write));
                        }
      TEST_F(InstrTest, test_alu_uni_op_mov)
   {
      AluInstr alu(op1_mov,
                                 EXPECT_FALSE(alu.has_alu_flag(alu_last_instr));
   EXPECT_FALSE(alu.end_group());
   EXPECT_FALSE(alu.has_alu_flag(alu_op3));
   EXPECT_FALSE(alu.has_source_mod(0, AluInstr::mod_abs));
                     EXPECT_EQ(alu.dest_chan(), 2);
            ASSERT_TRUE(dest);
   EXPECT_EQ(dest->sel(), 128);
   EXPECT_EQ(dest->chan(), 2);
            auto src0 = alu.psrc(0);
            EXPECT_EQ(src0->sel(), 129);
   EXPECT_EQ(src0->chan(), 0);
                     EXPECT_FALSE(alu.psrc(1));
            alu.set_source_mod(0, AluInstr::mod_abs);;
            alu.set_source_mod(0, AluInstr::mod_neg);
      }
      TEST_F(InstrTest, test_alu_op2)
   {
      AluInstr alu(op2_add,
               new Register(130, 1, pin_none),
                     EXPECT_TRUE(alu.has_alu_flag(alu_last_instr));
            EXPECT_FALSE(alu.has_source_mod(0, AluInstr::mod_neg));
   EXPECT_FALSE(alu.has_source_mod(1, AluInstr::mod_neg));
            EXPECT_FALSE(alu.has_alu_flag(alu_src0_rel));
   EXPECT_FALSE(alu.has_alu_flag(alu_src1_rel));
                     EXPECT_EQ(alu.dest_chan(), 1);
            ASSERT_TRUE(dest);
   EXPECT_EQ(dest->sel(), 130);
   EXPECT_EQ(dest->chan(), 1);
                     auto src0 = alu.psrc(0);
            EXPECT_EQ(src0->sel(), 129);
   EXPECT_EQ(src0->chan(), 2);
            auto src1 = alu.psrc(1);
            EXPECT_EQ(src1->sel(), 129);
   EXPECT_EQ(src1->chan(), 3);
            EXPECT_FALSE(alu.psrc(2));
      }
      TEST_F(InstrTest, test_alu_op3)
   {
      AluInstr alu(op3_cnde,
               new Register(130, 1, pin_none),
   new Register(129, 2, pin_chan),
            EXPECT_TRUE(alu.has_alu_flag(alu_write));
   EXPECT_TRUE(alu.has_alu_flag(alu_last_instr));
   EXPECT_TRUE(alu.end_group());
                     EXPECT_EQ(alu.dest_chan(), 1);
            ASSERT_TRUE(dest);
   EXPECT_EQ(dest->sel(), 130);
   EXPECT_EQ(dest->chan(), 1);
                     auto src0 = alu.psrc(0);
            EXPECT_EQ(src0->sel(), 129);
   EXPECT_EQ(src0->chan(), 2);
            auto src1 = alu.psrc(1);
            EXPECT_EQ(src1->sel(), 129);
   EXPECT_EQ(src1->chan(), 3);
            auto src2 = alu.psrc(2);
            EXPECT_EQ(src2->sel(), 131);
   EXPECT_EQ(src2->chan(), 1);
               }
      TEST_F(InstrTest, test_alu_op1_comp)
   {
      auto r128z = new Register(128, 2, pin_none);
   auto r128zc = new Register(128, 2, pin_chan);
   auto r128y = new Register(128, 1, pin_none);
   auto r129x = new Register(129, 0, pin_none);
   auto r129xc = new Register(129, 0, pin_chan);
   auto r129y = new Register(129, 1, pin_none);
            AluInstr alu1(op1_mov, r128z, r129x, {alu_write});
   EXPECT_NE(alu1, AluInstr(op1_mov, r128y, r129x, {alu_write}));
   EXPECT_NE(alu1, AluInstr(op1_mov, r128z, r129xc, {alu_write}));
   EXPECT_NE(alu1, AluInstr(op1_mov, r128z, r129y, {alu_write}));
   EXPECT_NE(alu1, AluInstr(op1_mov, r128z, r130x, {alu_write}));
   EXPECT_NE(alu1, AluInstr(op1_mov, r128z, r129x, {alu_write, alu_last_instr}));
   EXPECT_NE(alu1, AluInstr(op1_flt_to_int, r128z, r129x, {alu_write}));
               }
      TEST_F(InstrTest, test_alu_op2_comp)
   {
      auto r128x = new Register(128, 0, pin_none);
   auto r128y = new Register(128, 1, pin_none);
                     EXPECT_NE(
         EXPECT_NE(
         EXPECT_NE(
      }
      TEST_F(InstrTest, test_alu_op3_comp)
   {
      auto r128x = new Register(128, 0, pin_none);
   auto r128y = new Register(128, 1, pin_none);
                     EXPECT_NE(
      alu1,
   AluInstr(
      EXPECT_NE(
      alu1,
   AluInstr(
      EXPECT_NE(
      alu1,
   AluInstr(
   }
      TEST_F(InstrTest, test_alu_op3_ne)
   {
      auto R130x = new Register(130, 0, pin_none);
   auto R130y = new Register(130, 1, pin_none);
   auto R130z = new Register(130, 2, pin_none);
   auto R131z = new Register(131, 2, pin_none);
                     EXPECT_NE(
            EXPECT_NE(alu,
         EXPECT_NE(alu,
         EXPECT_NE(alu,
         EXPECT_NE(alu,
                  AluInstr alu_cf_changes = alu;
                     AluInstr alu_bs_changes = alu;
               };
      TEST_F(InstrTest, test_alu_op1_ne)
   {
      auto R130x = new Register(130, 0, pin_none);
   auto R130y = new Register(130, 1, pin_none);
                              EXPECT_NE(alu, AluInstr(op1_mov, R130z, R130y, {alu_write, alu_last_instr}));
   EXPECT_NE(alu, AluInstr(op1_mov, R130x, R130z, {alu_write, alu_last_instr}));
            AluInstr alu_cf_changes = alu;
                     AluInstr alu_bs_changes = alu;
               };
      TEST_F(InstrTest, test_alu_dot4_grouped)
   {
      auto R130x = new Register(130, 0, pin_none);
   auto R130y = new Register(130, 1, pin_none);
   auto R130z = new Register(130, 2, pin_none);
            auto R131x = new Register(131, 0, pin_none);
   auto R131y = new Register(131, 1, pin_none);
   auto R131z = new Register(131, 2, pin_none);
            auto R132x = new Register(132, 0, pin_chan);
   auto R132y = new Register(132, 1, pin_chan);
   auto R132z = new Register(132, 2, pin_chan);
                              EXPECT_NE(alu, AluInstr(op1_cos, R130x, R130y, {alu_write, alu_last_instr}));
            ValueFactory vf;
   auto group = alu.split(vf);
   group->fix_last_flag();
            auto i = group->begin();
   EXPECT_NE(i, group->end());
   ASSERT_TRUE(*i);
   check(**i, AluInstr(op2_dot4_ieee, R132x, R130x, R130y, {alu_write}));
   ++i;
   EXPECT_NE(i, group->end());
   ASSERT_TRUE(*i);
   check(**i, AluInstr(op2_dot4_ieee, R132y, R130z, R130w, {}));
   ++i;
   EXPECT_NE(i, group->end());
   ASSERT_TRUE(*i);
   check(**i, AluInstr(op2_dot4_ieee, R132z, R131x, R131y, {}));
   ++i;
   EXPECT_NE(i, group->end());
   ASSERT_TRUE(*i);
   check(**i, AluInstr(op2_dot4_ieee, R132w, R131z, R131w, {alu_last_instr}));
   ++i;
   EXPECT_NE(i, group->end());
   ASSERT_FALSE(*i);
   ++i;
      };
      #ifdef __cpp_exceptions
   TEST_F(InstrTest, test_alu_wrong_source_count)
   {
      EXPECT_THROW(AluInstr(op3_cnde,
                        new Register(130, 1, pin_none),
         EXPECT_THROW(AluInstr(op3_cnde,
                              EXPECT_THROW(AluInstr(op1_mov,
                        new Register(130, 1, pin_none),
         EXPECT_THROW(AluInstr(op2_add,
                              EXPECT_THROW(AluInstr(op2_add,
                        new Register(130, 1, pin_none),
   new Register(129, 2, pin_chan),
   }
      TEST_F(InstrTest, test_alu_write_no_dest)
   {
      EXPECT_THROW(AluInstr(op2_add,
                        nullptr,
   }
      #endif
      TEST_F(InstrTest, test_tex_basic)
   {
      TexInstr tex(
                     auto& dst = tex.dst();
            for (int i = 0; i < 4; ++i) {
      EXPECT_EQ(*dst[i], Register(129, i, pin_group));
   EXPECT_EQ(*src[i], Register(130, i, pin_group));
               EXPECT_EQ(tex.resource_id(), 17);
                     for (int i = 0; i < 3; ++i)
            EXPECT_FALSE(tex.has_tex_flag(TexInstr::x_unnormalized));
   EXPECT_FALSE(tex.has_tex_flag(TexInstr::y_unnormalized));
   EXPECT_FALSE(tex.has_tex_flag(TexInstr::z_unnormalized));
            tex.set_tex_flag(TexInstr::x_unnormalized);
   EXPECT_TRUE(tex.has_tex_flag(TexInstr::x_unnormalized));
   EXPECT_FALSE(tex.has_tex_flag(TexInstr::y_unnormalized));
   EXPECT_FALSE(tex.has_tex_flag(TexInstr::z_unnormalized));
            tex.set_tex_flag(TexInstr::y_unnormalized);
   EXPECT_TRUE(tex.has_tex_flag(TexInstr::x_unnormalized));
   EXPECT_TRUE(tex.has_tex_flag(TexInstr::y_unnormalized));
   EXPECT_FALSE(tex.has_tex_flag(TexInstr::z_unnormalized));
            tex.set_tex_flag(TexInstr::z_unnormalized);
   tex.set_tex_flag(TexInstr::w_unnormalized);
   EXPECT_TRUE(tex.has_tex_flag(TexInstr::x_unnormalized));
   EXPECT_TRUE(tex.has_tex_flag(TexInstr::y_unnormalized));
   EXPECT_TRUE(tex.has_tex_flag(TexInstr::z_unnormalized));
                              tex.set_dest_swizzle({4, 7, 0, 1});
   EXPECT_EQ(tex.dest_swizzle(0), 4);
   EXPECT_EQ(tex.dest_swizzle(1), 7);
   EXPECT_EQ(tex.dest_swizzle(2), 0);
            tex.set_dest_swizzle({7, 2, 5, 0});
   EXPECT_EQ(tex.dest_swizzle(0), 7);
   EXPECT_EQ(tex.dest_swizzle(1), 2);
   EXPECT_EQ(tex.dest_swizzle(2), 5);
            tex.set_offset(0, 2);
   tex.set_offset(1, -1);
            EXPECT_EQ(tex.get_offset(0), 4);
   EXPECT_EQ(tex.get_offset(1), -2);
      }
      TEST_F(InstrTest, test_tex_gather4)
   {
      TexInstr tex(
                     auto& dst = tex.dst();
            for (int i = 0; i < 4; ++i) {
      EXPECT_EQ(*dst[i], Register(131, i, pin_group));
   EXPECT_EQ(*src[i], Register(132, i, pin_group));
               EXPECT_EQ(tex.resource_id(), 2);
            for (int i = 0; i < 3; ++i)
            EXPECT_FALSE(tex.has_tex_flag(TexInstr::x_unnormalized));
   EXPECT_FALSE(tex.has_tex_flag(TexInstr::y_unnormalized));
   EXPECT_FALSE(tex.has_tex_flag(TexInstr::z_unnormalized));
            tex.set_gather_comp(2);
      }
      TEST_F(InstrTest, test_tex_neq)
   {
      TexInstr tex_ref(
                  EXPECT_NE(
      tex_ref,
   TexInstr(
      EXPECT_NE(
      tex_ref,
   TexInstr(
      EXPECT_NE(
      tex_ref,
   TexInstr(
         EXPECT_NE(
      tex_ref,
   TexInstr(
      EXPECT_NE(
      tex_ref,
   TexInstr(
      EXPECT_NE(
      tex_ref,
   TexInstr(
      EXPECT_NE(
      tex_ref,
   TexInstr(
         EXPECT_NE(tex_ref,
            TexInstr(TexInstr::sample,
            RegisterVec4(129),
   {0, 1, 2, 3},
   RegisterVec4(130, false, {7, 1, 2, 3}),
   EXPECT_NE(tex_ref,
            TexInstr(TexInstr::sample,
            RegisterVec4(129),
   {0, 1, 2, 3},
   RegisterVec4(130, false, {0, 7, 2, 3}),
   EXPECT_NE(tex_ref,
            TexInstr(TexInstr::sample,
            RegisterVec4(129),
   {0, 1, 2, 3},
   RegisterVec4(130, false, {0, 1, 7, 3}),
   EXPECT_NE(tex_ref,
            TexInstr(TexInstr::sample,
            RegisterVec4(129),
   {0, 1, 2, 3},
            EXPECT_NE(
      tex_ref,
   TexInstr(
      EXPECT_NE(
      tex_ref,
   TexInstr(
         /*
   auto tex_with_sampler_offset = tex_ref;
   tex_with_sampler_offset.set_sampler_offset(new LiteralConstant( 2));
            auto tex_cmp1 = tex_ref;
            tex_cmp1.set_tex_flag(TexInstr::x_unnormalized); EXPECT_NE(tex_ref, tex_cmp1);
   auto tex_cmp2 = tex_ref; tex_cmp2.set_tex_flag(TexInstr::y_unnormalized);
   EXPECT_NE(tex_ref, tex_cmp2); auto tex_cmp3 = tex_ref;
   tex_cmp3.set_tex_flag(TexInstr::z_unnormalized); EXPECT_NE(tex_ref, tex_cmp3); auto
   tex_cmp4 = tex_ref; tex_cmp4.set_tex_flag(TexInstr::w_unnormalized); EXPECT_NE(tex_ref,
            for (int i = 0; i < 3; ++i) {
      auto tex_ofs = tex_ref;
   tex_ofs.set_offset(i, 1);
               for (int i = 0; i < 4; ++i) {
      auto tex_swz = tex_ref;
   RegisterVec4::Swizzle dst_swz = {0,1,2,3};
   dst_swz[i] = 7;
   tex_swz.set_dest_swizzle(dst_swz);
               auto tex_cmp_mode = tex_ref;
   tex_cmp_mode.set_inst_mode(1);
      }
      TEST_F(InstrTest, test_export_basic)
   {
               EXPECT_EQ(exp0.export_type(), ExportInstr::param);
   EXPECT_EQ(exp0.location(), 60);
   EXPECT_EQ(exp0.value(), RegisterVec4(200));
            ExportInstr exp1(ExportInstr::param, 60, RegisterVec4(200));
   exp1.set_is_last_export(true);
            EXPECT_EQ(exp0, exp0);
            ExportInstr exp2(ExportInstr::pos, 60, RegisterVec4(200));
   EXPECT_EQ(exp2.export_type(), ExportInstr::pos);
            ExportInstr exp3(ExportInstr::param, 61, RegisterVec4(200));
   EXPECT_EQ(exp3.location(), 61);
            ExportInstr exp4(ExportInstr::param, 60, RegisterVec4(201));
   EXPECT_EQ(exp4.value(), RegisterVec4(201));
            EXPECT_NE(exp0,
         EXPECT_NE(exp0,
         EXPECT_NE(exp0,
         EXPECT_NE(exp0,
      }
      TEST_F(InstrTest, test_fetch_basic)
   {
      FetchInstr fetch(vc_fetch,
                  RegisterVec4(200),
   {0, 2, 1, 3},
   new Register(201, 2, pin_none),
   0,
   vertex_data,
   fmt_8,
   vtx_nf_norm,
         EXPECT_EQ(fetch.opcode(), vc_fetch);
   EXPECT_EQ(fetch.dst(), RegisterVec4(200));
   EXPECT_EQ(fetch.dest_swizzle(0), 0);
   EXPECT_EQ(fetch.dest_swizzle(1), 2);
   EXPECT_EQ(fetch.dest_swizzle(2), 1);
            EXPECT_EQ(fetch.src(), Register(201, 2, pin_none));
            EXPECT_EQ(fetch.resource_id(), 1);
            EXPECT_EQ(fetch.fetch_type(), vertex_data);
   EXPECT_EQ(fetch.data_format(), fmt_8);
   EXPECT_EQ(fetch.num_format(), vtx_nf_norm);
            EXPECT_EQ(fetch.mega_fetch_count(), 0);
   EXPECT_EQ(fetch.array_base(), 0);
   EXPECT_EQ(fetch.array_size(), 0);
            for (int i = 0; i < FetchInstr::unknown; ++i) {
                  EXPECT_NE(fetch,
            FetchInstr(vc_get_buf_resinfo,
               RegisterVec4(200),
   {0, 2, 1, 3},
   new Register(201, 2, pin_none),
   0,
   vertex_data,
   fmt_8,
         EXPECT_NE(fetch,
            FetchInstr(vc_fetch,
               RegisterVec4(201),
   {0, 2, 1, 3},
   new Register(201, 2, pin_none),
   0,
   vertex_data,
   fmt_8,
         EXPECT_NE(fetch,
            FetchInstr(vc_fetch,
               RegisterVec4(200),
   {1, 2, 0, 3},
   new Register(201, 2, pin_none),
   0,
   vertex_data,
   fmt_8,
         EXPECT_NE(fetch,
            FetchInstr(vc_fetch,
               RegisterVec4(200),
   {0, 2, 1, 3},
   new Register(200, 2, pin_none),
   0,
   vertex_data,
   fmt_8,
         EXPECT_NE(fetch,
            FetchInstr(vc_fetch,
               RegisterVec4(200),
   {0, 2, 1, 3},
   new Register(201, 2, pin_none),
   8,
   vertex_data,
   fmt_8,
         EXPECT_NE(fetch,
            FetchInstr(vc_fetch,
               RegisterVec4(200),
   {0, 2, 1, 3},
   new Register(201, 2, pin_none),
   0,
   instance_data,
   fmt_8,
         EXPECT_NE(fetch,
            FetchInstr(vc_fetch,
               RegisterVec4(200),
   {0, 2, 1, 3},
   new Register(201, 2, pin_none),
   0,
   vertex_data,
   fmt_8_8,
         EXPECT_NE(fetch,
            FetchInstr(vc_fetch,
               RegisterVec4(200),
   {0, 2, 1, 3},
   new Register(201, 2, pin_none),
   0,
   vertex_data,
   fmt_8,
         EXPECT_NE(fetch,
            FetchInstr(vc_fetch,
               RegisterVec4(200),
   {0, 2, 1, 3},
   new Register(201, 2, pin_none),
   0,
   vertex_data,
   fmt_8,
         EXPECT_NE(fetch,
            FetchInstr(vc_fetch,
               RegisterVec4(200),
   {0, 2, 1, 3},
   new Register(201, 2, pin_none),
   0,
   vertex_data,
   fmt_8,
         EXPECT_NE(fetch,
            FetchInstr(vc_fetch,
               RegisterVec4(200),
   {0, 2, 1, 3},
   new Register(201, 2, pin_none),
   0,
   vertex_data,
   fmt_8,
         auto fetch1 = fetch;
   fetch1.set_mfc(31);
   EXPECT_NE(fetch1, fetch);
   EXPECT_EQ(fetch1.mega_fetch_count(), 31);
   EXPECT_TRUE(
            auto fetch2 = fetch;
   fetch2.set_array_base(32);
   EXPECT_NE(fetch, fetch2);
            auto fetch3 = fetch;
   fetch3.set_array_size(16);
   EXPECT_NE(fetch, fetch3);
            auto fetch4 = fetch;
   fetch4.set_element_size(3);
   EXPECT_NE(fetch, fetch4);
      }
      TEST_F(InstrTest, test_fetch_basic2)
   {
      FetchInstr fetch(vc_get_buf_resinfo,
                  RegisterVec4(201),
   {0, 1, 3, 4},
   new Register(202, 3, pin_none),
   1,
   no_index_offset,
   fmt_32_32,
   vtx_nf_int,
         EXPECT_EQ(fetch.opcode(), vc_get_buf_resinfo);
   EXPECT_EQ(fetch.dst(), RegisterVec4(201));
   EXPECT_EQ(fetch.dest_swizzle(0), 0);
   EXPECT_EQ(fetch.dest_swizzle(1), 1);
   EXPECT_EQ(fetch.dest_swizzle(2), 3);
            EXPECT_EQ(fetch.src(), Register(202, 3, pin_none));
            EXPECT_EQ(fetch.resource_id(), 3);
            EXPECT_EQ(fetch.fetch_type(), no_index_offset);
   EXPECT_EQ(fetch.data_format(), fmt_32_32);
   EXPECT_EQ(fetch.num_format(), vtx_nf_int);
            EXPECT_EQ(fetch.mega_fetch_count(), 0);
   EXPECT_EQ(fetch.array_base(), 0);
   EXPECT_EQ(fetch.array_size(), 0);
            for (int i = 0; i < FetchInstr::unknown; ++i) {
                  auto fetch1 = fetch;
   fetch1.set_mfc(15);
   EXPECT_NE(fetch1, fetch);
   EXPECT_EQ(fetch1.mega_fetch_count(), 15);
   EXPECT_TRUE(
            auto fetch2 = fetch;
   fetch2.set_array_base(128);
   EXPECT_NE(fetch, fetch2);
            auto fetch3 = fetch;
   fetch3.set_array_size(8);
   EXPECT_NE(fetch, fetch3);
            auto fetch4 = fetch;
   fetch4.set_element_size(1);
   EXPECT_NE(fetch, fetch4);
      }
