   /*
   * Copyright © 2020 Valve Corporation
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
   #include <llvm/Config/llvm-config.h>
      #include "helpers.h"
   #include "sid.h"
      using namespace aco;
      BEGIN_TEST(assembler.s_memtime)
      for (unsigned i = GFX6; i <= GFX10; i++) {
      if (!setup_cs(NULL, (amd_gfx_level)i))
            //~gfx[6-7]>> c7800000
   //~gfx[6-7]!  bf810000
   //~gfx[8-9]>> s_memtime s[0:1] ; c0900000 00000000
   //~gfx10>> s_memtime s[0:1] ; f4900000 fa000000
                  END_TEST
      BEGIN_TEST(assembler.branch_3f)
      if (!setup_cs(NULL, (amd_gfx_level)GFX10))
            //! BB0:
   //! s_branch BB1                                                ; bf820040
   //! s_nop 0                                                     ; bf800000
            for (unsigned i = 0; i < 0x3f; i++)
                                 END_TEST
      BEGIN_TEST(assembler.long_jump.unconditional_forwards)
      if (!setup_cs(NULL, (amd_gfx_level)GFX10))
            //!BB0:
   //! s_getpc_b64 s[0:1]                                          ; be801f00
   //! s_addc_u32 s0, s0, 0x20014                                  ; 8200ff00 00020014
   //! s_bitcmp1_b32 s0, 0                                         ; bf0d8000
   //! s_bitset0_b32 s0, 0                                         ; be801b80
   //! s_setpc_b64 s[0:1]                                          ; be802000
                     //! s_nop 0                                                     ; bf800000
   //!(then repeated 32767 times)
   for (unsigned i = 0; i < INT16_MAX + 1; i++)
            //! BB2:
   //! s_endpgm                                                    ; bf810000
            program->blocks[2].linear_preds.push_back(0u);
               END_TEST
      BEGIN_TEST(assembler.long_jump.conditional_forwards)
      if (!setup_cs(NULL, (amd_gfx_level)GFX10))
            //! BB0:
   //! s_cbranch_scc1 BB1                                          ; bf850006
   //! s_getpc_b64 s[0:1]                                          ; be801f00
   //! s_addc_u32 s0, s0, 0x20014                                  ; 8200ff00 00020014
   //! s_bitcmp1_b32 s0, 0                                         ; bf0d8000
   //! s_bitset0_b32 s0, 0                                         ; be801b80
   //! s_setpc_b64 s[0:1]                                          ; be802000
                     //! BB1:
   //! s_nop 0 ; bf800000
   //!(then repeated 32767 times)
   for (unsigned i = 0; i < INT16_MAX + 1; i++)
            //! BB2:
   //! s_endpgm                                                    ; bf810000
            program->blocks[1].linear_preds.push_back(0u);
   program->blocks[2].linear_preds.push_back(0u);
               END_TEST
      BEGIN_TEST(assembler.long_jump.unconditional_backwards)
      if (!setup_cs(NULL, (amd_gfx_level)GFX10))
            //!BB0:
   //! s_nop 0                                                     ; bf800000
   //!(then repeated 32767 times)
   for (unsigned i = 0; i < INT16_MAX + 1; i++)
            //! s_getpc_b64 s[0:1]                                          ; be801f00
   //! s_addc_u32 s0, s0, 0xfffdfffc                               ; 8200ff00 fffdfffc
   //! s_bitcmp1_b32 s0, 0                                         ; bf0d8000
   //! s_bitset0_b32 s0, 0                                         ; be801b80
   //! s_setpc_b64 s[0:1]                                          ; be802000
            //! BB1:
   //! s_endpgm                                                    ; bf810000
            program->blocks[0].linear_preds.push_back(0u);
               END_TEST
      BEGIN_TEST(assembler.long_jump.conditional_backwards)
      if (!setup_cs(NULL, (amd_gfx_level)GFX10))
            //!BB0:
   //! s_nop 0                                                     ; bf800000
   //!(then repeated 32767 times)
   for (unsigned i = 0; i < INT16_MAX + 1; i++)
            //! s_cbranch_execz BB1                                         ; bf880006
   //! s_getpc_b64 s[0:1]                                          ; be801f00
   //! s_addc_u32 s0, s0, 0xfffdfff8                               ; 8200ff00 fffdfff8
   //! s_bitcmp1_b32 s0, 0                                         ; bf0d8000
   //! s_bitset0_b32 s0, 0                                         ; be801b80
   //! s_setpc_b64 s[0:1]                                          ; be802000
            //! BB1:
   //! s_endpgm                                                    ; bf810000
            program->blocks[0].linear_preds.push_back(0u);
               END_TEST
      BEGIN_TEST(assembler.long_jump .3f)
      if (!setup_cs(NULL, (amd_gfx_level)GFX10))
            //! BB0:
   //! s_branch BB1                                                ; bf820040
   //! s_nop 0                                                     ; bf800000
            for (unsigned i = 0; i < 0x3f - 6; i++) // a unconditional long jump is 6 dwords
                  bld.reset(program->create_and_insert_block());
   for (unsigned i = 0; i < INT16_MAX + 1; i++)
                  program->blocks[1].linear_preds.push_back(0u);
   program->blocks[2].linear_preds.push_back(0u);
               END_TEST
      BEGIN_TEST(assembler.long_jump.constaddr)
      if (!setup_cs(NULL, (amd_gfx_level)GFX10))
            //>> s_getpc_b64 s[0:1]                                          ; be801f00
                     for (unsigned i = 0; i < INT16_MAX + 1; i++)
                     //>> s_getpc_b64 s[0:1]                                          ; be801f00
   //! s_add_u32 s0, s0, 32                                         ; 8000ff00 00000020
   bld.sop1(aco_opcode::p_constaddr_getpc, Definition(PhysReg(0), s2), Operand::zero());
   bld.sop2(aco_opcode::p_constaddr_addlo, Definition(PhysReg(0), s1), bld.def(s1, scc),
            program->blocks[2].linear_preds.push_back(0u);
               END_TEST
      BEGIN_TEST(assembler.long_jump.discard_early_exit)
      if (!setup_cs(NULL, (amd_gfx_level)GFX10))
            //! BB0:
   //! s_cbranch_scc1 BB1                                          ; bf850006
   //! s_getpc_b64 s[0:1]                                          ; be801f00
   //! s_addc_u32 s0, s0, 0x20014                                  ; 8200ff00 00020014
   //! s_bitcmp1_b32 s0, 0                                         ; bf0d8000
   //! s_bitset0_b32 s0, 0                                         ; be801b80
   //! s_setpc_b64 s[0:1]                                          ; be802000
                     //! BB1:
   //! s_nop 1                                                     ; bf800001
   //!(then repeated 32766 times)
   //! s_endpgm                                                    ; bf810000
   for (unsigned i = 0; i < INT16_MAX; i++)
            //! BB2:
   //! s_endpgm                                                    ; bf810000
            program->blocks[1].linear_preds.push_back(0u);
   program->blocks[2].linear_preds.push_back(0u);
               END_TEST
      BEGIN_TEST(assembler.v_add3)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      if (!setup_cs(NULL, (amd_gfx_level)i))
            //~gfx9>> v_add3_u32 v0, 0, 0, 0 ; d1ff0000 02010080
   //~gfx10>> v_add3_u32 v0, 0, 0, 0 ; d76d0000 02010080
   aco_ptr<VALU_instruction> add3{
         add3->operands[0] = Operand::zero();
   add3->operands[1] = Operand::zero();
   add3->operands[2] = Operand::zero();
   add3->definitions[0] = Definition(PhysReg(0), v1);
                  END_TEST
      BEGIN_TEST(assembler.v_add3_clamp)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      if (!setup_cs(NULL, (amd_gfx_level)i))
            //~gfx9>> integer addition + clamp ; d1ff8000 02010080
   //~gfx10>> integer addition + clamp ; d76d8000 02010080
   aco_ptr<VALU_instruction> add3{
         add3->operands[0] = Operand::zero();
   add3->operands[1] = Operand::zero();
   add3->operands[2] = Operand::zero();
   add3->definitions[0] = Definition(PhysReg(0), v1);
   add3->clamp = 1;
                  END_TEST
      BEGIN_TEST(assembler.smem_offset)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      if (!setup_cs(NULL, (amd_gfx_level)i))
            Definition dst(PhysReg(7), s1);
   Operand sbase(PhysReg(6), s2);
            //~gfx9>> s_load_dword s7, s[6:7], s5 ; c00001c3 00000005
   //~gfx10>> s_load_dword s7, s[6:7], s5 ; f40001c3 0a000000
   bld.smem(aco_opcode::s_load_dword, dst, sbase, offset);
   //~gfx9! s_load_dword s7, s[6:7], 0x42 ; c00201c3 00000042
   //~gfx10! s_load_dword s7, s[6:7], 0x42 ; f40001c3 fa000042
   bld.smem(aco_opcode::s_load_dword, dst, sbase, Operand::c32(0x42));
   if (i >= GFX9) {
      //~gfx9! s_load_dword s7, s[6:7], s5 offset:0x42 ; c00241c3 0a000042
   //~gfx10! s_load_dword s7, s[6:7], s5 offset:0x42 ; f40001c3 0a000042
                     END_TEST
      BEGIN_TEST(assembler.p_constaddr)
      if (!setup_cs(NULL, GFX9))
            Definition dst0 = bld.def(s2);
   Definition dst1 = bld.def(s2);
   dst0.setFixed(PhysReg(0));
            //>> s_getpc_b64 s[0:1] ; be801c00
   //! s_add_u32 s0, s0, 44 ; 8000ff00 0000002c
            //! s_getpc_b64 s[2:3] ; be821c00
   //! s_add_u32 s2, s2, 64 ; 8002ff02 00000040
            aco::lower_to_hw_instr(program.get());
      END_TEST
      BEGIN_TEST(assembler.vopc_sdwa)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      if (!setup_cs(NULL, (amd_gfx_level)i))
            //~gfx9>> v_cmp_lt_u32_sdwa vcc, 0, 0 src0_sel:DWORD src1_sel:DWORD ; 7d9300f9 86860080
   //~gfx10>> v_cmp_lt_u32_sdwa vcc, 0, 0 src0_sel:DWORD src1_sel:DWORD   ; 7d8300f9 86860080
   bld.vopc_sdwa(aco_opcode::v_cmp_lt_u32, Definition(vcc, s2), Operand::zero(),
            //~gfx9! v_cmp_lt_u32_sdwa s[44:45], 0, 0 src0_sel:DWORD src1_sel:DWORD ; 7d9300f9 8686ac80
   //~gfx10! v_cmp_lt_u32_sdwa s[44:45], 0, 0 src0_sel:DWORD src1_sel:DWORD ; 7d8300f9 8686ac80
   bld.vopc_sdwa(aco_opcode::v_cmp_lt_u32, Definition(PhysReg(0x2c), s2), Operand::zero(),
            //~gfx9! v_cmp_lt_u32_sdwa exec, 0, 0 src0_sel:DWORD src1_sel:DWORD ; 7d9300f9 8686fe80
   //~gfx10! v_cmp_lt_u32_sdwa exec, 0, 0 src0_sel:DWORD src1_sel:DWORD  ; 7d8300f9 8686fe80
   bld.vopc_sdwa(aco_opcode::v_cmp_lt_u32, Definition(exec, s2), Operand::zero(),
            if (i == GFX10) {
      //~gfx10! v_cmpx_lt_u32_sdwa 0, 0 src0_sel:DWORD src1_sel:DWORD ; 7da300f9 86860080
   bld.vopc_sdwa(aco_opcode::v_cmpx_lt_u32, Definition(exec, s2), Operand::zero(),
      } else {
      //~gfx9! v_cmpx_lt_u32_sdwa vcc, 0, 0 src0_sel:DWORD src1_sel:DWORD ; 7db300f9 86860080
                  //~gfx9! v_cmpx_lt_u32_sdwa s[44:45], 0, 0 src0_sel:DWORD src1_sel:DWORD ; 7db300f9 8686ac80
   bld.vopc_sdwa(aco_opcode::v_cmpx_lt_u32, Definition(PhysReg(0x2c), s2),
                     END_TEST
      BEGIN_TEST(assembler.gfx11.smem)
      if (!setup_cs(NULL, GFX11))
            Definition dst = bld.def(s1);
            Operand op_s1(bld.tmp(s1));
            Operand op_s2(bld.tmp(s2));
            Operand op_s4(bld.tmp(s4));
            //>> s_dcache_inv                                                ; f4840000 f8000000
            //! s_load_b32 s4, s[16:17], 0x2a                               ; f4000108 f800002a
            //! s_load_b32 s4, s[16:17], s8                                 ; f4000108 10000000
            //! s_load_b32 s4, s[16:17], s8 offset:0x2a                     ; f4000108 1000002a
            //! s_buffer_load_b32 s4, s[32:35], s8 glc                      ; f4204110 10000000
            //! s_buffer_load_b32 s4, s[32:35], s8 dlc                      ; f4202110 10000000
               END_TEST
      BEGIN_TEST(assembler.gfx11.mubuf)
      if (!setup_cs(NULL, GFX11))
            Definition dst = bld.def(v1);
            Operand op_s4(bld.tmp(s4));
            Operand op_v1(bld.tmp(v1));
            Operand op_v2(bld.tmp(v2));
            Operand op_s1(bld.tmp(s1));
            Operand op_m0(bld.tmp(s1));
            //! llvm_version: #llvm_ver
            /* Addressing */
   //>> buffer_load_b32 v42, off, s[32:35], s30                     ; e0500000 1e082a80
            //! buffer_load_b32 v42, off, s[32:35], 42                      ; e0500000 aa082a80
            //! buffer_load_b32 v42, v10, s[32:35], s30 offen               ; e0500000 1e482a0a
            //! buffer_load_b32 v42, v10, s[32:35], s30 idxen               ; e0500000 1e882a0a
   bld.mubuf(aco_opcode::buffer_load_dword, dst, op_s4, op_v1, op_s1, 0, false)->mubuf().idxen =
            //! buffer_load_b32 v42, v[20:21], s[32:35], s30 idxen offen    ; e0500000 1ec82a14
   bld.mubuf(aco_opcode::buffer_load_dword, dst, op_s4, op_v2, op_s1, 0, true)->mubuf().idxen =
            //! buffer_load_b32 v42, off, s[32:35], s30 offset:84           ; e0500054 1e082a80
            /* Various flags */
   //! buffer_load_b32 v42, off, s[32:35], 0 glc                   ; e0504000 80082a80
   bld.mubuf(aco_opcode::buffer_load_dword, dst, op_s4, Operand(v1), Operand::zero(), 0, false)
      ->mubuf()
         //! buffer_load_b32 v42, off, s[32:35], 0 dlc                   ; e0502000 80082a80
   bld.mubuf(aco_opcode::buffer_load_dword, dst, op_s4, Operand(v1), Operand::zero(), 0, false)
      ->mubuf()
         //! buffer_load_b32 v42, off, s[32:35], 0 slc                   ; e0501000 80082a80
   bld.mubuf(aco_opcode::buffer_load_dword, dst, op_s4, Operand(v1), Operand::zero(), 0, false)
      ->mubuf()
         //; if llvm_ver >= 16:
   //;    insert_pattern('buffer_load_b32 v[42:43], off, s[32:35], 0 tfe              ; e0500000 80282a80')
   //; else:
   //;    insert_pattern('buffer_load_b32 v42, off, s[32:35], 0 tfe                   ; e0500000 80282a80')
   bld.mubuf(aco_opcode::buffer_load_dword, dst, op_s4, Operand(v1), Operand::zero(), 0, false)
      ->mubuf()
         /* LDS */
   //! buffer_load_lds_b32 off, s[32:35], 0                        ; e0c40000 80080080
   bld.mubuf(aco_opcode::buffer_load_dword, op_s4, Operand(v1), Operand::zero(), op_m0, 0, false)
      ->mubuf()
         //! buffer_load_lds_i8 off, s[32:35], 0                         ; e0b80000 80080080
   bld.mubuf(aco_opcode::buffer_load_sbyte, op_s4, Operand(v1), Operand::zero(), op_m0, 0, false)
      ->mubuf()
         //! buffer_load_lds_i16 off, s[32:35], 0                        ; e0c00000 80080080
   bld.mubuf(aco_opcode::buffer_load_sshort, op_s4, Operand(v1), Operand::zero(), op_m0, 0, false)
      ->mubuf()
         //! buffer_load_lds_u8 off, s[32:35], 0                         ; e0b40000 80080080
   bld.mubuf(aco_opcode::buffer_load_ubyte, op_s4, Operand(v1), Operand::zero(), op_m0, 0, false)
      ->mubuf()
         //! buffer_load_lds_u16 off, s[32:35], 0                        ; e0bc0000 80080080
   bld.mubuf(aco_opcode::buffer_load_ushort, op_s4, Operand(v1), Operand::zero(), op_m0, 0, false)
      ->mubuf()
         //! buffer_load_lds_format_x off, s[32:35], 0                   ; e0c80000 80080080
   bld.mubuf(aco_opcode::buffer_load_format_x, op_s4, Operand(v1), Operand::zero(), op_m0, 0, false)
      ->mubuf()
         /* Stores */
   //! buffer_store_b32 v10, off, s[32:35], s30                    ; e0680000 1e080a80
            //! buffer_store_b64 v[20:21], v10, s[32:35], s30 offen         ; e06c0000 1e48140a
               END_TEST
      BEGIN_TEST(assembler.gfx11.mtbuf)
      if (!setup_cs(NULL, GFX11))
            Definition dst = bld.def(v1);
            Operand op_s4(bld.tmp(s4));
            Operand op_v1(bld.tmp(v1));
            Operand op_v2(bld.tmp(v2));
            Operand op_s1(bld.tmp(s1));
            unsigned dfmt = V_008F0C_BUF_DATA_FORMAT_32_32;
            //! llvm_version: #llvm_ver
            /* Addressing */
   //>> tbuffer_load_format_x v42, off, s[32:35], s30 format:[BUF_FMT_32_32_FLOAT] ; e9900000 1e082a80
   bld.mtbuf(aco_opcode::tbuffer_load_format_x, dst, op_s4, Operand(v1), op_s1, dfmt, nfmt, 0,
            //! tbuffer_load_format_x v42, off, s[32:35], 42 format:[BUF_FMT_32_32_FLOAT] ; e9900000 aa082a80
   bld.mtbuf(aco_opcode::tbuffer_load_format_x, dst, op_s4, Operand(v1), Operand::c32(42), dfmt,
            //! tbuffer_load_format_x v42, v10, s[32:35], s30 format:[BUF_FMT_32_32_FLOAT] offen ; e9900000 1e482a0a
            //! tbuffer_load_format_x v42, v10, s[32:35], s30 format:[BUF_FMT_32_32_FLOAT] idxen ; e9900000 1e882a0a
   bld.mtbuf(aco_opcode::tbuffer_load_format_x, dst, op_s4, op_v1, op_s1, dfmt, nfmt, 0, false)
      ->mtbuf()
         //! tbuffer_load_format_x v42, v[20:21], s[32:35], s30 format:[BUF_FMT_32_32_FLOAT] idxen offen ; e9900000 1ec82a14
   bld.mtbuf(aco_opcode::tbuffer_load_format_x, dst, op_s4, op_v2, op_s1, dfmt, nfmt, 0, true)
      ->mtbuf()
         //! tbuffer_load_format_x v42, off, s[32:35], s30 format:[BUF_FMT_32_32_FLOAT] offset:84 ; e9900054 1e082a80
   bld.mtbuf(aco_opcode::tbuffer_load_format_x, dst, op_s4, Operand(v1), op_s1, dfmt, nfmt, 84,
            /* Various flags */
   //! tbuffer_load_format_x v42, off, s[32:35], 0 format:[BUF_FMT_32_32_FLOAT] glc ; e9904000 80082a80
   bld.mtbuf(aco_opcode::tbuffer_load_format_x, dst, op_s4, Operand(v1), Operand::zero(), dfmt,
            ->mtbuf()
         //! tbuffer_load_format_x v42, off, s[32:35], 0 format:[BUF_FMT_32_32_FLOAT] dlc ; e9902000 80082a80
   bld.mtbuf(aco_opcode::tbuffer_load_format_x, dst, op_s4, Operand(v1), Operand::zero(), dfmt,
            ->mtbuf()
         //! tbuffer_load_format_x v42, off, s[32:35], 0 format:[BUF_FMT_32_32_FLOAT] slc ; e9901000 80082a80
   bld.mtbuf(aco_opcode::tbuffer_load_format_x, dst, op_s4, Operand(v1), Operand::zero(), dfmt,
            ->mtbuf()
         //; if llvm_ver >= 16:
   //;    insert_pattern('tbuffer_load_format_x v42, off, s[32:35], 0 format:[BUF_FMT_32_32_FLOAT] ; e9900000 80282a80')
   //; else:
   //;    insert_pattern('tbuffer_load_format_x v42, off, s[32:35], 0 format:[BUF_FMT_32_32_FLOAT] tfe ; e9900000 80282a80')
   bld.mtbuf(aco_opcode::tbuffer_load_format_x, dst, op_s4, Operand(v1), Operand::zero(), dfmt,
            ->mtbuf()
         /* Stores */
   //! tbuffer_store_format_x v10, off, s[32:35], s30 format:[BUF_FMT_32_32_FLOAT] ; e9920000 1e080a80
   bld.mtbuf(aco_opcode::tbuffer_store_format_x, op_s4, Operand(v1), op_s1, op_v1, dfmt, nfmt, 0,
            //! tbuffer_store_format_xy v[20:21], v10, s[32:35], s30 format:[BUF_FMT_32_32_FLOAT] offen ; e9928000 1e48140a
               END_TEST
      BEGIN_TEST(assembler.gfx11.mimg)
      if (!setup_cs(NULL, GFX11))
            Definition dst_v1 = bld.def(v1);
            Definition dst_v4 = bld.def(v4);
            Operand op_s4(bld.tmp(s4));
            Operand op_s8(bld.tmp(s8));
            Operand op_v1(bld.tmp(v1));
            Operand op_v2(bld.tmp(v2));
            Operand op_v4(bld.tmp(v4));
            //>> image_sample v[84:87], v10, s[64:71], s[32:35] dmask:0xf dim:SQ_RSRC_IMG_1D ; f06c0f00 2010540a
            //! image_sample v[84:87], v[20:21], s[64:71], s[32:35] dmask:0xf dim:SQ_RSRC_IMG_2D ; f06c0f04 20105414
   bld.mimg(aco_opcode::image_sample, dst_v4, op_s8, op_s4, Operand(v1), op_v2)->mimg().dim =
            //! image_sample v42, v10, s[64:71], s[32:35] dmask:0x1 dim:SQ_RSRC_IMG_1D ; f06c0100 20102a0a
            /* Various flags */
   //! image_sample v[84:87], v10, s[64:71], s[32:35] dmask:0xf dim:SQ_RSRC_IMG_1D dlc ; f06c2f00 2010540a
            //! image_sample v[84:87], v10, s[64:71], s[32:35] dmask:0xf dim:SQ_RSRC_IMG_1D glc ; f06c4f00 2010540a
            //! image_sample v[84:87], v10, s[64:71], s[32:35] dmask:0xf dim:SQ_RSRC_IMG_1D slc ; f06c1f00 2010540a
            //! image_sample v[84:88], v10, s[64:71], s[32:35] dmask:0xf dim:SQ_RSRC_IMG_1D tfe ; f06c0f00 2030540a
            //! image_sample v[84:87], v10, s[64:71], s[32:35] dmask:0xf dim:SQ_RSRC_IMG_1D lwe ; f06c0f00 2050540a
            //! image_sample v[84:87], v10, s[64:71], s[32:35] dmask:0xf dim:SQ_RSRC_IMG_1D r128 ; f06c8f00 2010540a
            //! image_sample v[84:87], v10, s[64:71], s[32:35] dmask:0xf dim:SQ_RSRC_IMG_1D a16 ; f06d0f00 2010540a
            //! image_sample v[84:85], v10, s[64:71], s[32:35] dmask:0xf dim:SQ_RSRC_IMG_1D d16 ; f06e0f00 2010540a
            /* NSA */
   //! image_sample v[84:87], [v10, v40], s[64:71], s[32:35] dmask:0xf dim:SQ_RSRC_IMG_2D ; f06c0f05 2010540a 00000028
   bld.mimg(aco_opcode::image_sample, dst_v4, op_s8, op_s4, Operand(v1), op_v1,
            ->mimg()
         /* Stores */
   //! image_store v[30:33], v10, s[64:71] dmask:0xf dim:SQ_RSRC_IMG_1D ; f0180f00 00101e0a
            //! image_atomic_add v10, v20, s[64:71] dmask:0xf dim:SQ_RSRC_IMG_2D ; f0300f04 00100a14
   bld.mimg(aco_opcode::image_atomic_add, Definition(op_v1.physReg(), v1), op_s8, Operand(s4),
            ->mimg()
            END_TEST
      BEGIN_TEST(assembler.gfx11.flat)
      if (!setup_cs(NULL, GFX11))
            Definition dst_v1 = bld.def(v1);
            Operand op_s1(bld.tmp(s1));
            Operand op_s2(bld.tmp(s2));
            Operand op_v1(bld.tmp(v1));
            Operand op_v2(bld.tmp(v2));
            /* Addressing */
   //>> flat_load_b32 v42, v[20:21]                                 ; dc500000 2a7c0014
            //! global_load_b32 v42, v[20:21], off                          ; dc520000 2a7c0014
            //! global_load_b32 v42, v10, s[64:65]                          ; dc520000 2a40000a
            //! scratch_load_b32 v42, v10, off                              ; dc510000 2afc000a
            //! scratch_load_b32 v42, off, s32                              ; dc510000 2a200080
            //! scratch_load_b32 v42, v10, s32                              ; dc510000 2aa0000a
            //! global_load_b32 v42, v[20:21], off offset:-42               ; dc521fd6 2a7c0014
            //! global_load_b32 v42, v[20:21], off offset:84                ; dc520054 2a7c0014
            /* Various flags */
   //! flat_load_b32 v42, v[20:21] slc                             ; dc508000 2a7c0014
            //! flat_load_b32 v42, v[20:21] glc                             ; dc504000 2a7c0014
            //! flat_load_b32 v42, v[20:21] dlc                             ; dc502000 2a7c0014
            /* Stores */
   //! flat_store_b32 v[20:21], v10                                ; dc680000 007c0a14
               END_TEST
      BEGIN_TEST(assembler.gfx11.exp)
      if (!setup_cs(NULL, GFX11))
            Operand op[4];
   for (unsigned i = 0; i < 4; i++)
            Operand op_m0(bld.tmp(s1));
            //>> exp mrt3 v1, v0, v3, v2                                     ; f800003f 02030001
            //! exp mrt3 v1, off, v0, off                                   ; f8000035 80008001
            //! exp mrt3 v1, v0, v3, v2 done                                ; f800083f 02030001
            //>> exp mrt3 v1, v0, v3, v2 row_en                              ; f800203f 02030001
               END_TEST
      BEGIN_TEST(assembler.gfx11.vinterp)
      if (!setup_cs(NULL, GFX11))
            Definition dst = bld.def(v1);
            Operand op0(bld.tmp(v1));
            Operand op1(bld.tmp(v1));
            Operand op2(bld.tmp(v1));
            //>> v_interp_p10_f32 v42, v10, v20, v30 wait_exp:7              ; cd00072a 047a290a
            //! v_interp_p10_f32 v42, v10, v20, v30 wait_exp:6              ; cd00062a 047a290a
            //! v_interp_p2_f32 v42, v10, v20, v30                          ; cd01002a 047a290a
            //! v_interp_p10_f32 v42, -v10, v20, v30                        ; cd00002a 247a290a
   bld.vinterp_inreg(aco_opcode::v_interp_p10_f32_inreg, dst, op0, op1, op2, 0)
      ->vinterp_inreg()
         //! v_interp_p10_f32 v42, v10, -v20, v30                        ; cd00002a 447a290a
   bld.vinterp_inreg(aco_opcode::v_interp_p10_f32_inreg, dst, op0, op1, op2, 0)
      ->vinterp_inreg()
         //! v_interp_p10_f32 v42, v10, v20, -v30                        ; cd00002a 847a290a
   bld.vinterp_inreg(aco_opcode::v_interp_p10_f32_inreg, dst, op0, op1, op2, 0)
      ->vinterp_inreg()
         //! v_interp_p10_f16_f32 v42, v10, v20, v30 op_sel:[1,0,0,0]    ; cd02082a 047a290a
            //! v_interp_p2_f16_f32 v42, v10, v20, v30 op_sel:[0,1,0,0]     ; cd03102a 047a290a
            //! v_interp_p10_rtz_f16_f32 v42, v10, v20, v30 op_sel:[0,0,1,0] ; cd04202a 047a290a
            //! v_interp_p2_rtz_f16_f32 v42, v10, v20, v30 op_sel:[0,0,0,1] ; cd05402a 047a290a
            //! v_interp_p10_f32 v42, v10, v20, v30 clamp                   ; cd00802a 047a290a
   bld.vinterp_inreg(aco_opcode::v_interp_p10_f32_inreg, dst, op0, op1, op2, 0)
      ->vinterp_inreg()
            END_TEST
      BEGIN_TEST(assembler.gfx11.ldsdir)
      if (!setup_cs(NULL, GFX11))
            Definition dst = bld.def(v1);
            Operand op(bld.tmp(s1));
            //>> lds_direct_load v42 wait_vdst:15                            ; ce1f002a
            //! lds_direct_load v42 wait_vdst:6                             ; ce16002a
            //! lds_direct_load v42                                         ; ce10002a
            //! lds_param_load v42, attr56.x wait_vdst:8                    ; ce08e02a
            //! lds_param_load v42, attr56.x                                ; ce00e02a
            //! lds_param_load v42, attr34.y                                ; ce00892a
            //! lds_param_load v42, attr12.z                                ; ce00322a
               END_TEST
      BEGIN_TEST(assembler.gfx11.vop12c_v128)
      if (!setup_cs(NULL, GFX11))
            Definition dst_v0 = bld.def(v1);
            Definition dst_v128 = bld.def(v1);
            Operand op_v1(bld.tmp(v1));
            Operand op_v2(bld.tmp(v1));
            Operand op_v129(bld.tmp(v1));
            Operand op_v130(bld.tmp(v1));
            //! llvm_version: #llvm_ver
            //>> BB0:
   //; if llvm_ver >= 16:
   //;    insert_pattern('v_mul_f16_e32 v0, v1, v2 ; Error: VGPR_32_Lo128: unknown register 128 ; 6a000501')
   //; else:
   //;    insert_pattern('v_mul_f16_e32 v0, v1, v2                                    ; 6a000501')
            //! v_mul_f16_e64 v128, v1, v2                                  ; d5350080 00020501
            //! v_mul_f16_e64 v0, v129, v2                                  ; d5350000 00020581
            //! v_mul_f16_e64 v0, v1, v130                                  ; d5350000 00030501
            //! v_rcp_f16_e64 v128, v1                                      ; d5d40080 00000101
            //! v_cmp_eq_f16_e64 vcc, v129, v2                              ; d402006a 00020581
            //! v_mul_f16_e64_dpp v128, v1, v2 row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d5350080 000204fa ff0d2101
            //! v_mul_f16_e64_dpp v0, v129, v2 row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d5350000 000204fa ff0d2181
            //! v_mul_f16_e64_dpp v0, v1, v130 row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d5350000 000304fa ff0d2101
            //! v_mul_f16_e64_dpp v128, v1, v2 dpp8:[0,0,0,0,0,0,0,0] fi:1  ; d5350080 000204ea 00000001
            //! v_mul_f16_e64_dpp v0, v129, v2 dpp8:[0,0,0,0,0,0,0,0] fi:1  ; d5350000 000204ea 00000081
            //! v_mul_f16_e64_dpp v0, v1, v130 dpp8:[0,0,0,0,0,0,0,0] fi:1  ; d5350000 000304ea 00000001
            //! v_fma_f16 v128, v1, v2, 0x60                                ; d6480080 03fe0501 00000060
            //! v_fma_f16 v128, v1, 0x60, v2                                ; d6480080 0409ff01 00000060
            //! v_rcp_f16_e64_dpp v128, -v1 row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d5d40080 200000fa ff1d2101
            //! v_rcp_f16_e64_dpp v128, |v1| row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d5d40180 000000fa ff2d2101
            //! v_mul_f16_e64_dpp v128, -v1, v2 row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d5350080 200204fa ff1d2101
   bld.vop2_dpp(aco_opcode::v_mul_f16, dst_v128, op_v1, op_v2, dpp_row_rr(1))->dpp16().neg[0] =
            //! v_mul_f16_e64_dpp v128, |v1|, v2 row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d5350180 000204fa ff2d2101
   bld.vop2_dpp(aco_opcode::v_mul_f16, dst_v128, op_v1, op_v2, dpp_row_rr(1))->dpp16().abs[0] =
            //! v_cmp_eq_f16_e64_dpp vcc, -v129, v2 row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d402006a 200204fa ff1d2181
   bld.vopc_dpp(aco_opcode::v_cmp_eq_f16, bld.def(s2, vcc), op_v129, op_v2, dpp_row_rr(1))
      ->dpp16()
         //! v_cmp_eq_f16_e64_dpp vcc, |v129|, v2 row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d402016a 000204fa ff2d2181
   bld.vopc_dpp(aco_opcode::v_cmp_eq_f16, bld.def(s2, vcc), op_v129, op_v2, dpp_row_rr(1))
      ->dpp16()
            END_TEST
      BEGIN_TEST(assembler.vop3_dpp)
      if (!setup_cs(NULL, GFX11))
            Definition dst_v0 = bld.def(v1);
            Definition dst_non_vcc = bld.def(s2);
            Operand op_v1(bld.tmp(v1));
            Operand op_v2(bld.tmp(v1));
            Operand op_s1(bld.tmp(s1));
            //>> BB0:
   //! v_fma_f32_e64_dpp v0, v1, v2, s1 clamp row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d6138000 000604fa ff0d2101
   bld.vop3_dpp(aco_opcode::v_fma_f32, dst_v0, op_v1, op_v2, op_s1, dpp_row_rr(1))->valu().clamp =
            //! v_fma_mix_f32_e64_dpp v0, |v1|, |v2|, |s1| op_sel:[1,0,0] op_sel_hi:[1,0,1] row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; cc204f00 080604fa ffad2101
   bld.vop3p_dpp(aco_opcode::v_fma_mix_f32, dst_v0, op_v1, op_v2, op_s1, 0x1, 0x5, dpp_row_rr(1))
      ->valu()
         //! v_fma_f32_e64_dpp v0, -v1, -v2, -s1 dpp8:[0,0,0,0,0,0,0,0] fi:1 ; d6130000 e00604ea 00000001
            //! v_fma_mix_f32_e64_dpp v0, -v1, -v2, s1 op_sel_hi:[1,1,1] dpp8:[0,0,0,0,0,0,0,0] fi:1 ; cc204000 780604ea 00000001
   bld.vop3p_dpp8(aco_opcode::v_fma_mix_f32, dst_v0, op_v1, op_v2, op_s1, 0x0, 0x7)->valu().neg =
            //! v_add_f32_e64_dpp v0, v1, v2 clamp row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d5038000 000204fa ff0d2101
   bld.vop2_e64_dpp(aco_opcode::v_add_f32, dst_v0, op_v1, op_v2, dpp_row_rr(1))->valu().clamp =
            //! v_sqrt_f32_e64_dpp v0, v1 clamp row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d5b38000 000000fa ff0d2101
            //! v_cmp_lt_f32_e64_dpp s[4:5], |v1|, |v2| row_ror:1 row_mask:0xf bank_mask:0xf bound_ctrl:1 fi:1 ; d4110304 000204fa ffad2101
   bld.vopc_e64_dpp(aco_opcode::v_cmp_lt_f32, dst_non_vcc, op_v1, op_v2, dpp_row_rr(1))->valu().abs =
            //! v_add_f32_e64_dpp v0, v1, v2 mul:4 dpp8:[0,0,0,0,0,0,0,0] fi:1 ; d5030000 100204ea 00000001
            //! v_sqrt_f32_e64_dpp v0, v1 clamp dpp8:[0,0,0,0,0,0,0,0] fi:1 ; d5b38000 000000ea 00000001
            //! v_cmp_lt_f32_e64_dpp s[4:5], |v1|, v2 dpp8:[0,0,0,0,0,0,0,0] fi:1 ; d4110104 000204ea 00000001
               END_TEST
