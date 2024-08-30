      #include "../sfn_instr_alu.h"
   #include "../sfn_instr_export.h"
   #include "../sfn_instr_fetch.h"
   #include "../sfn_instr_lds.h"
   #include "../sfn_instr_mem.h"
   #include "../sfn_instr_tex.h"
   #include "../sfn_instrfactory.h"
      #include "gtest/gtest.h"
   #include <sstream>
      namespace r600 {
      using std::istringstream;
   using std::ostringstream;
   using std::string;
      class TestInstrFromString : public ::testing::Test {
   public:
                     protected:
      void add_dest_from_string(const char *init);
            void check(const Instr& eval, const Instr& expect);
            void SetUp() override;
               };
      TEST_F(TestInstrFromString, test_alu_mov)
   {
               AluInstr expect(op1_mov,
                           }
      TEST_F(TestInstrFromString, test_alu_lds_read_ret)
   {
                           }
      TEST_F(TestInstrFromString, test_alu_mov_literal)
   {
      AluInstr expect(op1_mov,
                           }
      TEST_F(TestInstrFromString, test_alu_mov_neg)
   {
      add_dest_from_string("R1999.x");
   AluInstr expect(op1_mov,
                                 }
      TEST_F(TestInstrFromString, test_alu_mov_abs)
   {
      add_dest_from_string("R1999.x");
   AluInstr expect(op1_mov,
                                 }
      TEST_F(TestInstrFromString, test_alu_mov_neg_abs)
   {
      add_dest_from_string("R1999.x");
   AluInstr expect(op1_mov,
                     expect.set_source_mod(0, AluInstr::mod_abs);
               }
      TEST_F(TestInstrFromString, test_alu_add)
   {
      add_dest_from_string("R1998.z");
            AluInstr expect(op2_add,
                  new Register(2000, 1, pin_none),
         }
      TEST_F(TestInstrFromString, test_alu_add_clmap)
   {
      add_dest_from_string("R1998.z");
   add_dest_from_string("R1999.w");
   AluInstr expect(op2_add,
                  new Register(2000, 1, pin_none),
         }
      TEST_F(TestInstrFromString, test_alu_add_neg2)
   {
      add_dest_from_string("R1998.z");
   add_dest_from_string("R1999.w");
   AluInstr expect(op2_add,
                  new Register(2000, 1, pin_none),
                  }
      TEST_F(TestInstrFromString, test_alu_sete_update_pref)
   {
      add_dest_from_string("R1998.z");
   add_dest_from_string("R1999.w");
   AluInstr expect(op2_sete,
                  new Register(2000, 1, pin_none),
      expect.set_source_mod(1, AluInstr::mod_neg);
      }
      TEST_F(TestInstrFromString, test_alu_sete_update_pref_empty_dest)
   {
      add_dest_from_string("R1998.z");
   add_dest_from_string("R1999.w");
   AluInstr expect(op2_sete,
                  new Register(2000, 0, pin_none),
         }
      TEST_F(TestInstrFromString, test_alu_setne_update_exec)
   {
      add_dest_from_string("R1998.z");
   add_dest_from_string("R1999.w");
   AluInstr expect(op2_setne,
                  new Register(2000, 1, pin_none),
      expect.set_source_mod(1, AluInstr::mod_neg);
      }
      TEST_F(TestInstrFromString, test_alu_add_abs2)
   {
      add_dest_from_string("R1998.z");
   add_dest_from_string("R1999.w");
   AluInstr expect(op2_add,
                  new Register(2000, 1, pin_none),
      expect.set_source_mod(1, AluInstr::mod_abs);
      }
      TEST_F(TestInstrFromString, test_alu_add_abs2_neg2)
   {
      add_dest_from_string("R1998.z");
   add_dest_from_string("R1999.w");
   AluInstr expect(op2_add,
                  new Register(2000, 1, pin_none),
      expect.set_source_mod(1, AluInstr::mod_neg);
               }
      TEST_F(TestInstrFromString, test_alu_muladd)
   {
      add_dest_from_string("R1998.z");
   add_dest_from_string("R1999.w");
   add_dest_from_string("R2000.y");
   AluInstr expect(op3_muladd_ieee,
                  new Register(2000, 1, pin_none),
   new Register(1999, 3, pin_none),
         }
      TEST_F(TestInstrFromString, test_alu_muladd_neg3)
   {
      add_dest_from_string("R1998.z");
   add_dest_from_string("R1999.w");
   add_dest_from_string("R2000.y");
   AluInstr expect(op3_muladd_ieee,
                  new Register(2000, 1, pin_none),
   new Register(1999, 3, pin_none),
         }
      TEST_F(TestInstrFromString, test_alu_mov_bs)
   {
      add_dest_from_string("R1999.x");
   for (auto& [expect_bs, str] : AluInstr::bank_swizzle_map) {
               AluInstr expect(op1_mov,
                                    }
      TEST_F(TestInstrFromString, test_alu_dot4_ieee)
   {
      add_dest_from_string("R199.x");
   add_dest_from_string("R199.y");
   add_dest_from_string("R199.z");
   add_dest_from_string("R199.w");
   add_dest_from_string("R198.x");
   add_dest_from_string("R198.y");
   add_dest_from_string("R198.z");
   add_dest_from_string("R198.w");
   auto init = std::string("ALU DOT4_IEEE R2000.y : R199.x R198.w + R199.y R198.z + "
            AluInstr expect(op2_dot4_ieee,
                  new Register(2000, 1, pin_none),
   {new Register(199, 0, pin_none),
   new Register(198, 3, pin_none),
   new Register(199, 1, pin_none),
   new Register(198, 2, pin_none),
   new Register(199, 2, pin_none),
   new Register(198, 1, pin_none),
   new Register(199, 3, pin_none),
            }
      TEST_F(TestInstrFromString, test_alu_dot4_with_mods)
   {
      add_dest_from_string("R199.x");
   add_dest_from_string("R199.y");
   add_dest_from_string("R199.z");
   add_dest_from_string("R199.w");
   add_dest_from_string("R198.x");
   add_dest_from_string("R198.y");
   add_dest_from_string("R198.z");
   add_dest_from_string("R198.w");
   auto init = std::string("ALU DOT4_IEEE R2000.y : -R199.x R198.w + R199.y |R198.z| + "
            AluInstr expect(op2_dot4_ieee,
                  new Register(2000, 1, pin_none),
   {new Register(199, 0, pin_none),
   new Register(198, 3, pin_none),
   new Register(199, 1, pin_none),
   new Register(198, 2, pin_none),
   new Register(199, 2, pin_none),
   new Register(198, 1, pin_none),
   new Register(199, 3, pin_none),
         expect.set_source_mod(0, AluInstr::mod_neg);
   expect.set_source_mod(3, AluInstr::mod_abs);
   expect.set_source_mod(4, AluInstr::mod_neg);
   expect.set_source_mod(4, AluInstr::mod_abs);
            check(init, expect);
            std::ostringstream print_str;
   print_str << *instr;
         }
         TEST_F(TestInstrFromString, test_alu_mov_cf)
   {
      add_dest_from_string("R1999.x");
   for (auto& [expect_cf, str] : AluInstr::cf_map) {
               AluInstr expect(op1_mov,
                                    }
      TEST_F(TestInstrFromString, test_alu_interp_xy)
   {
      add_dest_from_string("R0.y@fully");
   auto init =
            auto r0y = new Register(0, 1, pin_fully);
   r0y->set_flag(Register::pin_start);
   AluInstr expect(op2_interp_zw,
                  new Register(1024, 2, pin_chan),
                  }
      TEST_F(TestInstrFromString, test_alu_interp_xy_no_write)
   {
      add_dest_from_string("R0.x@fully");
            auto r0x = new Register(0, 0, pin_fully);
            AluInstr expect(op2_interp_xy,
                  new Register(1024, 0, pin_chan),
                  }
      TEST_F(TestInstrFromString, test_alu_mov_cf_bs)
   {
      add_dest_from_string("R1999.x");
   auto init = std::string("ALU MOV R2000.y : R1999.x {WL} VEC_210 POP_AFTER");
   AluInstr expect(op1_mov,
                     expect.set_cf_type(cf_alu_pop_after);
   expect.set_bank_swizzle(alu_vec_210);
      }
      TEST_F(TestInstrFromString, test_tex_sample_basic)
   {
      add_dest_vec4_from_string("R2000.xyzw");
   auto init = std::string("TEX SAMPLE R1000.xyzw : R2000.xyzw RID:10 SID:1 NNNN");
   TexInstr expect(
            }
      TEST_F(TestInstrFromString, test_tex_ld_basic)
   {
      add_dest_vec4_from_string("R2002.xyzw");
   auto init = std::string("TEX LD R1001.xyzw : R2002.xyzw RID:27 SID:7 NNNN");
   TexInstr expect(
            }
      TEST_F(TestInstrFromString, test_tex_sample_with_offset)
   {
      add_dest_vec4_from_string("R2002.xyzw");
   auto init =
            TexInstr expect(
         expect.set_offset(0, 1);
   expect.set_offset(1, -2);
               }
      TEST_F(TestInstrFromString, test_tex_gather4_x)
   {
      add_dest_vec4_from_string("R2002.xyzw");
   auto init =
         TexInstr expect(
            }
      TEST_F(TestInstrFromString, test_tex_gather4_y)
   {
      add_dest_vec4_from_string("R2002.xyzw");
   auto init =
         TexInstr expect(
         expect.set_gather_comp(1);
      }
      TEST_F(TestInstrFromString, test_tex_sampler_with_offset)
   {
      add_dest_vec4_from_string("R2002.xyzw");
   auto init =
         TexInstr expect(TexInstr::sample,
                  RegisterVec4(1001),
   {0, 1, 2, 3},
   RegisterVec4(2002),
   7,
         }
      TEST_F(TestInstrFromString, test_export_param_60)
   {
               ExportInstr expect(ExportInstr::param, 60, RegisterVec4(1001));
      }
      TEST_F(TestInstrFromString, test_export_pos_61)
   {
               ExportInstr expect(ExportInstr::pos, 61, RegisterVec4(1002, false, {1, 4, 5, 7}));
      }
      TEST_F(TestInstrFromString, test_export_last_pixel_0)
   {
               ExportInstr expect(ExportInstr::pixel, 0, RegisterVec4(1002, false, {2, 3, 0, 1}));
   expect.set_is_last_export(true);
      }
      TEST_F(TestInstrFromString, test_fetch_basic)
   {
               FetchInstr expect(vc_fetch,
                     RegisterVec4(1002),
   {0, 4, 5, 1},
   new Register(201, 2, pin_none),
   0,
   vertex_data,
   fmt_8,
   vtx_nf_norm,
   expect.set_mfc(31);
   expect.set_element_size(3);
      }
      TEST_F(TestInstrFromString, test_query_buffer_size)
   {
      QueryBufferSizeInstr expect(RegisterVec4(1002),
                        FetchInstr expect_fetch(vc_get_buf_resinfo,
                           RegisterVec4(1002),
   RegisterVec4::Swizzle({0, 1, 2, 3}),
   new Register(0, 7, pin_fully),
   0,
   no_index_offset,
   fmt_32_32_32_32,
   expect_fetch.set_fetch_flag(FetchInstr::format_comp_signed);
      }
      TEST_F(TestInstrFromString, test_load_from_buffer)
   {
      add_dest_from_string("R201.x");
   add_dest_from_string("R202.x");
   string init = "LOAD_BUF R200.xzwy : R201.x + 16b RID:10 + R202.x";
   LoadFromBuffer expect(RegisterVec4(200),
                        RegisterVec4::Swizzle({0, 2, 3, 1}),
   new Register(201, 0, pin_none),
               auto instr = from_string(init);
   FetchInstr expect_fetch(vc_fetch,
                           RegisterVec4(200),
   RegisterVec4::Swizzle({0, 2, 3, 1}),
   new Register(201, 0, pin_none),
   16,
   no_index_offset,
   fmt_32_32_32_32_float,
   expect_fetch.set_fetch_flag(FetchInstr::format_comp_signed);
   expect_fetch.set_mfc(16);
      }
      TEST_F(TestInstrFromString, test_load_from_scratch)
   {
         add_dest_from_string("R201.x");
            LoadFromScratch expect(RegisterVec4(200),
                              FetchInstr expect_fetch(vc_read_scratch,
                           RegisterVec4(200),
   RegisterVec4::Swizzle({0, 2, 3, 1}),
   new Register(201, 0, pin_none),
   0,
   no_index_offset,
   fmt_32_32_32_32,
   expect_fetch.set_element_size(3);
   expect_fetch.set_print_skip(FetchInstr::EPrintSkip::mfc);
   expect_fetch.set_print_skip(FetchInstr::EPrintSkip::fmt);
   expect_fetch.set_print_skip(FetchInstr::EPrintSkip::ftype);
   expect_fetch.set_fetch_flag(FetchInstr::EFlags::uncached);
   expect_fetch.set_fetch_flag(FetchInstr::EFlags::indexed);
   expect_fetch.set_fetch_flag(FetchInstr::EFlags::wait_ack);
               }
      TEST_F(TestInstrFromString, test_write_scratch_to_offset)
   {
      add_dest_vec4_from_string("R1.xyzw");
   string init = "WRITE_SCRATCH 20 R1.xyzw AL:4 ALO:16";
   ScratchIOInstr expect(RegisterVec4(1), 20, 4, 16, 0xf);
            add_dest_vec4_from_string("R2.xyzw");
   string init2 = "WRITE_SCRATCH 10 R2.xy_w AL:8 ALO:8";
   ScratchIOInstr expect2(RegisterVec4(2), 10, 8, 8, 0xb);
      }
      TEST_F(TestInstrFromString, test_write_scratch_to_index)
   {
      add_dest_vec4_from_string("R1.xyzw");
   add_dest_from_string("R3.x");
   string init = "WRITE_SCRATCH @R3.x[10] R1.xyzw AL:4 ALO:16";
   ScratchIOInstr expect(RegisterVec4(1), new Register(3, 0, pin_none), 4, 16, 0xf, 10);
            add_dest_vec4_from_string("R2.xyzw");
   add_dest_from_string("R4.x");
   string init2 = "WRITE_SCRATCH @R4.x[20] R2.xy__ AL:4 ALO:16";
   ScratchIOInstr expect2(RegisterVec4(2), new Register(4, 0, pin_none), 4, 16, 0x3, 20);
      }
      TEST_F(TestInstrFromString, test_load_from_scratch_fixed_offset)
   {
               LoadFromScratch expect(RegisterVec4(200),
                              FetchInstr expect_fetch(vc_read_scratch,
                           RegisterVec4(200),
   RegisterVec4::Swizzle({0, 2, 3, 1}),
   new Register(0, 7, pin_none),
   0,
   no_index_offset,
   fmt_32_32_32_32,
   expect_fetch.set_element_size(3);
   expect_fetch.set_print_skip(FetchInstr::EPrintSkip::mfc);
   expect_fetch.set_print_skip(FetchInstr::EPrintSkip::fmt);
   expect_fetch.set_print_skip(FetchInstr::EPrintSkip::ftype);
   expect_fetch.set_fetch_flag(FetchInstr::EFlags::uncached);
   expect_fetch.set_fetch_flag(FetchInstr::EFlags::wait_ack);
   expect_fetch.set_array_base(10);
               }
      TEST_F(TestInstrFromString, test_lds_read_3_values)
   {
      add_dest_from_string("R5.x@free");
   add_dest_from_string("R5.y@free");
            auto init =
            std::vector<PRegister, Allocator<PRegister>> dests(3);
            for (int i = 0; i < 3; ++i) {
      dests[i] = new Register(10 + i, 0, pin_free);
               LDSReadInstr expect(dests, srcs);
      }
      TEST_F(TestInstrFromString, test_lds_read_2_values)
   {
      add_dest_from_string("R5.x@free");
                     std::vector<PRegister, Allocator<PRegister>> dests(2);
            for (int i = 0; i < 2; ++i) {
      dests[i] = new Register(11 + i, 0, pin_free);
               LDSReadInstr expect(dests, srcs);
      }
      TEST_F(TestInstrFromString, test_lds_write_1_value)
   {
      auto init = "LDS WRITE __.x [ R1.x ] : R2.y";
   add_dest_from_string("R1.x");
            LDSAtomicInstr expect(DS_OP_WRITE,
                           }
      TEST_F(TestInstrFromString, test_lds_write_2_value)
   {
               add_dest_from_string("R1.x");
            LDSAtomicInstr expect(DS_OP_WRITE2,
                           }
      TEST_F(TestInstrFromString, test_lds_write_atomic_add_ret)
   {
               add_dest_from_string("R1.x");
            LDSAtomicInstr expect(DS_OP_ADD_RET,
                           }
      TEST_F(TestInstrFromString, test_lds_write_atomic_add)
   {
               add_dest_from_string("R1.x");
            LDSAtomicInstr expect(DS_OP_ADD,
                           }
      TEST_F(TestInstrFromString, test_writeTF)
   {
                                    }
      TestInstrFromString::TestInstrFromString() {}
      PInst
   TestInstrFromString::from_string(const std::string& s)
   {
         }
      void
   TestInstrFromString::check(const Instr& eval, const Instr& expect)
   {
         }
      void
   TestInstrFromString::check(const string& init, const Instr& expect)
   {
      auto instr = from_string(init);
   ASSERT_TRUE(instr);
      }
      void
   TestInstrFromString::add_dest_from_string(const char *init)
   {
         }
      void
   TestInstrFromString::add_dest_vec4_from_string(const char *init)
   {
      RegisterVec4::Swizzle dummy;
      }
      void
   TestInstrFromString::SetUp()
   {
      MemoryPool::instance().initialize();
      }
      void
   TestInstrFromString::TearDown()
   {
         }
      } // namespace r600
