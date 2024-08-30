   /*
   * Copyright 2020 Red Hat Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
   #include "nv50_ir_emit_gv100.h"
   #include "nv50_ir_sched_gm107.h"
      namespace nv50_ir {
      /*******************************************************************************
   * instruction format helpers
   ******************************************************************************/
      #define FA_NODEF (1 << 0)
   #define FA_RRR   (1 << 1)
   #define FA_RRI   (1 << 2)
   #define FA_RRC   (1 << 3)
   #define FA_RIR   (1 << 4)
   #define FA_RCR   (1 << 5)
      #define FA_SRC_MASK 0x0ff
   #define FA_SRC_NEG  0x100
   #define FA_SRC_ABS  0x200
      #define EMPTY -1
   #define __(a) (a) // no source modifiers
   #define _A(a) ((a) | FA_SRC_ABS)
   #define N_(a) ((a) | FA_SRC_NEG)
   #define NA(a) ((a) | FA_SRC_NEG | FA_SRC_ABS)
      void
   CodeEmitterGV100::emitFormA_I32(int src)
   {
      emitIMMD(32, 32, insn->src(src));
   if (insn->src(src).mod.abs())
         if (insn->src(src).mod.neg())
      }
      void
   CodeEmitterGV100::emitFormA_RRC(uint16_t op, int src1, int src2)
   {
      emitInsn(op);
   if (src1 >= 0) {
      emitNEG (75, (src1 & FA_SRC_MASK), (src1 & FA_SRC_NEG));
   emitABS (74, (src1 & FA_SRC_MASK), (src1 & FA_SRC_ABS));
      }
   if (src2 >= 0) {
      emitNEG (63, (src2 & FA_SRC_MASK), (src2 & FA_SRC_NEG));
   emitABS (62, (src2 & FA_SRC_MASK), (src2 & FA_SRC_ABS));
         }
      void
   CodeEmitterGV100::emitFormA_RRI(uint16_t op, int src1, int src2)
   {
      emitInsn(op);
   if (src1 >= 0) {
      emitNEG (75, (src1 & FA_SRC_MASK), (src1 & FA_SRC_NEG));
   emitABS (74, (src1 & FA_SRC_MASK), (src1 & FA_SRC_ABS));
      }
   if (src2 >= 0)
      }
      void
   CodeEmitterGV100::emitFormA_RRR(uint16_t op, int src1, int src2)
   {
      emitInsn(op);
   if (src2 >= 0) {
      emitNEG (75, (src2 & FA_SRC_MASK), (src2 & FA_SRC_NEG));
   emitABS (74, (src2 & FA_SRC_MASK), (src2 & FA_SRC_ABS));
               if (src1 >= 0) {
      emitNEG (63, (src1 & FA_SRC_MASK), (src1 & FA_SRC_NEG));
   emitABS (62, (src1 & FA_SRC_MASK), (src1 & FA_SRC_ABS));
         }
      void
   CodeEmitterGV100::emitFormA(uint16_t op, uint8_t forms,
         {
      switch ((src1 < 0) ? FILE_GPR : insn->src(src1 & FA_SRC_MASK).getFile()) {
   case FILE_GPR:
      switch ((src2 < 0) ? FILE_GPR : insn->src(src2 & FA_SRC_MASK).getFile()) {
   case FILE_GPR:
      assert(forms & FA_RRR);
   emitFormA_RRR((1 << 9) | op, src1, src2);
      case FILE_IMMEDIATE:
      assert(forms & FA_RRI);
   emitFormA_RRI((2 << 9) | op, src1, src2);
      case FILE_MEMORY_CONST:
      assert(forms & FA_RRC);
   emitFormA_RRC((3 << 9) | op, src1, src2);
      default:
      assert(!"bad src2 file");
      }
      case FILE_IMMEDIATE:
      assert((src2 < 0) || insn->src(src2 & FA_SRC_MASK).getFile() == FILE_GPR);
   assert(forms & FA_RIR);
   emitFormA_RRI((4 << 9) | op, src2, src1);
      case FILE_MEMORY_CONST:
      assert((src2 < 0) || insn->src(src2 & FA_SRC_MASK).getFile() == FILE_GPR);
   assert(forms & FA_RCR);
   emitFormA_RRC((5 << 9) | op, src2, src1);
      default:
      assert(!"bad src1 file");
               if (src0 >= 0) {
      assert(insn->src(src0 & FA_SRC_MASK).getFile() == FILE_GPR);
   emitABS(73, (src0 & FA_SRC_MASK), (src0 & FA_SRC_ABS));
   emitNEG(72, (src0 & FA_SRC_MASK), (src0 & FA_SRC_NEG));
               if (!(forms & FA_NODEF))
      }
      /*******************************************************************************
   * control
   ******************************************************************************/
      void
   CodeEmitterGV100::emitBRA()
   {
      const FlowInstruction *insn = this->insn->asFlow();
                     emitInsn (0x947);
   emitField(34, 48, target);
   emitPRED (87);
      }
      void
   CodeEmitterGV100::emitEXIT()
   {
      emitInsn (0x94d);
   emitNOT  (90);
   emitPRED (87);
   emitField(85, 1, 0); // .NO_ATEXIT
      }
      void
   CodeEmitterGV100::emitKILL()
   {
      emitInsn(0x95b);
      }
      void
   CodeEmitterGV100::emitNOP()
   {
         }
      void
   CodeEmitterGV100::emitWARPSYNC()
   {
      emitFormA(0x148, FA_NODEF | FA_RRR | FA_RIR | FA_RCR, EMPTY, __(0), EMPTY);
   emitNOT  (90);
      }
      /*******************************************************************************
   * movement / conversion
   ******************************************************************************/
      void
   CodeEmitterGV100::emitCS2R()
   {
      emitInsn(0x805);
   emitSYS (72, insn->src(0));
      }
      void
   CodeEmitterGV100::emitF2F()
   {
      if (typeSizeof(insn->sType) != 8 && typeSizeof(insn->dType) != 8)
         else
         emitField(84, 2, util_logbase2(typeSizeof(insn->sType)));
   emitFMZ  (80, 1);
   emitRND  (78);
   emitField(75, 2, util_logbase2(typeSizeof(insn->dType)));
      }
      void
   CodeEmitterGV100::emitF2I()
   {
      if (typeSizeof(insn->sType) != 8 && typeSizeof(insn->dType) != 8)
         else
         emitField(84, 2, util_logbase2(typeSizeof(insn->sType)));
   emitFMZ  (80, 1);
   emitRND  (78);
   emitField(77, 1, 0); // .NTZ
   emitField(75, 2, util_logbase2(typeSizeof(insn->dType)));
      }
      void
   CodeEmitterGV100::emitFRND()
   {
               switch (insn->op) {
   case OP_CVT:
      switch (insn->rnd) {
   case ROUND_NI: subop = 0; break;
   case ROUND_MI: subop = 1; break;
   case ROUND_PI: subop = 2; break;
   case ROUND_ZI: subop = 3; break;
   default:
      assert(!"invalid FRND mode");
      }
      case OP_FLOOR: subop = 1; break;
   case OP_CEIL : subop = 2; break;
   case OP_TRUNC: subop = 3; break;
   default:
      assert(!"invalid FRND opcode");
               if (typeSizeof(insn->sType) != 8 && typeSizeof(insn->dType) != 8)
         else
         emitField(84, 2, util_logbase2(typeSizeof(insn->sType)));
   emitFMZ  (80, 1);
   emitField(78, 2, subop);
      }
      void
   CodeEmitterGV100::emitI2F()
   {
      if (typeSizeof(insn->sType) != 8 && typeSizeof(insn->dType) != 8)
         else
         emitField(84, 2, util_logbase2(typeSizeof(insn->sType)));
   emitRND  (78);
   emitField(75, 2, util_logbase2(typeSizeof(insn->dType)));
   emitField(74, 1, isSignedType(insn->sType));
   if (typeSizeof(insn->sType) == 2)
         else
      }
      void
   CodeEmitterGV100::emitMOV()
   {
      switch (insn->def(0).getFile()) {
   case FILE_GPR:
      switch (insn->src(0).getFile()) {
   case FILE_GPR:
   case FILE_MEMORY_CONST:
   case FILE_IMMEDIATE:
      emitFormA(0x002, FA_RRR | FA_RIR | FA_RCR, EMPTY, __(0), EMPTY);
   emitField(72, 4, insn->lanes);
      case FILE_PREDICATE:
      emitInsn (0x807);
   emitGPR  (16, insn->def(0));
   emitGPR  (24);
   emitField(32, 32, 0xffffffff);
   emitField(90,  1, 1);
   emitPRED (87, insn->src(0));
      case FILE_BARRIER:
   case FILE_THREAD_STATE:
      emitInsn (0x355);
   emitBTS  (24, insn->src(0));
   emitGPR  (16, insn->def(0));
      default:
      assert(!"bad src file");
      }
      case FILE_PREDICATE:
      emitInsn (0x20c);
   emitPRED (87);
   emitPRED (84);
   emitNOT  (71);
   emitPRED (68);
   emitPRED (81, insn->def(0));
   emitCond3(76, CC_NE);
   emitGPR  (24, insn->src(0));
   emitGPR  (32);
      case FILE_BARRIER:
   case FILE_THREAD_STATE:
      switch (insn->src(0).getFile()) {
   case FILE_GPR:
      emitInsn (0x356);
   emitGPR  (32, insn->src(0));
   emitBTS  (24, insn->def(0));
      case FILE_BARRIER:
      emitInsn (0xf56);
   emitBTS  (24, insn->def(0));
   emitBTS  (16, insn->src(0));
      case FILE_THREAD_STATE:
      assert(insn->def(0).getFile() == FILE_BARRIER);
   emitInsn (0xf55);
   emitBTS  (24, insn->src(0));
   emitBTS  (16, insn->def(0));
      default:
      assert(!"bad src file");
      }
   emitField(84, 1, insn->getDef(0)->reg.data.ts == TS_PQUAD_MACTIVE ? 1 : 0);
      default:
      assert(!"bad dst file");
         }
      void
   CodeEmitterGV100::emitPRMT()
   {
      emitFormA(0x016, FA_RRR | FA_RRI | FA_RRC | FA_RIR | FA_RCR, __(0), __(1), __(2));
      }
      void
   CodeEmitterGV100::emitS2R()
   {
      emitInsn(0x919);
   emitSYS (72, insn->src(0));
      }
      void
   gv100_selpFlip(const FixupEntry *entry, uint32_t *code, const FixupData& data)
   {
      int loc = entry->loc;
   bool val = false;
   switch (entry->ipa) {
   case 0:
      val = data.force_persample_interp;
      case 1:
      val = data.msaa;
      }
   if (val)
         else
      }
      void
   CodeEmitterGV100::emitSEL()
   {
      emitFormA(0x007, FA_RRR | FA_RIR | FA_RCR, __(0), __(1), EMPTY);
   emitNOT  (90, insn->src(2));
   emitPRED (87, insn->src(2));
   if (insn->subOp >= 1)
      }
      void
   CodeEmitterGV100::emitSHFL()
   {
      switch (insn->src(1).getFile()) {
   case FILE_GPR:
      switch (insn->src(2).getFile()) {
   case FILE_GPR:
      emitInsn(0x389);
   emitGPR (64, insn->src(2));
      case FILE_IMMEDIATE:
      emitInsn(0x589);
   emitIMMD(40, 13, insn->src(2));
      default:
      assert(!"bad src2 file");
      }
   emitGPR(32, insn->src(1));
      case FILE_IMMEDIATE:
      switch (insn->src(2).getFile()) {
   case FILE_GPR:
      emitInsn(0x989);
   emitGPR (64, insn->src(2));
      case FILE_IMMEDIATE:
      emitInsn(0xf89);
   emitIMMD(40, 13, insn->src(2));
      default:
      assert(!"bad src2 file");
      }
   emitIMMD(53, 5, insn->src(1));
      default:
      assert(!"bad src1 file");
               if (insn->defExists(1))
         else
            emitField(58, 2, insn->subOp);
   emitGPR  (24, insn->src(0));
      }
      /*******************************************************************************
   * fp32
   ******************************************************************************/
      void
   CodeEmitterGV100::emitFADD()
   {
      if (insn->src(1).getFile() == FILE_GPR)
         else
         emitFMZ  (80, 1);
   emitRND  (78);
      }
      void
   CodeEmitterGV100::emitFFMA()
   {
      emitFormA(0x023, FA_RRR | FA_RRI | FA_RRC | FA_RIR | FA_RCR, NA(0), NA(1), NA(2));
   emitField(80, 1, insn->ftz);
   emitRND  (78);
   emitSAT  (77);
      }
      void
   CodeEmitterGV100::emitFMNMX()
   {
      emitFormA(0x009, FA_RRR | FA_RIR | FA_RCR, NA(0), NA(1), EMPTY);
   emitField(90, 1, insn->op == OP_MAX);
   emitPRED (87);
      }
      void
   CodeEmitterGV100::emitFMUL()
   {
      emitFormA(0x020, FA_RRR | FA_RIR | FA_RCR, NA(0), NA(1), EMPTY);
   emitField(80, 1, insn->ftz);
   emitPDIV (84);
   emitRND  (78);
   emitSAT  (77);
      }
      void
   CodeEmitterGV100::emitFSET_BF()
   {
               emitFormA(0x00a, FA_RRR | FA_RIR | FA_RCR, NA(0), NA(1), EMPTY);
   emitFMZ  (80, 1);
            if (insn->op != OP_SET) {
      switch (insn->op) {
   case OP_SET_AND: emitField(74, 2, 0); break;
   case OP_SET_OR : emitField(74, 2, 1); break;
   case OP_SET_XOR: emitField(74, 2, 2); break;
   default:
      assert(!"invalid set op");
      }
   emitNOT (90, insn->src(2));
      } else {
            }
      void
   CodeEmitterGV100::emitFSETP()
   {
               emitFormA(0x00b, FA_NODEF | FA_RRR | FA_RIR | FA_RCR, NA(0), NA(1), EMPTY);
   emitFMZ  (80, 1);
            if (insn->op != OP_SET) {
      switch (insn->op) {
   case OP_SET_AND: emitField(74, 2, 0); break;
   case OP_SET_OR : emitField(74, 2, 1); break;
   case OP_SET_XOR: emitField(74, 2, 2); break;
   default:
      assert(!"invalid set op");
      }
   emitNOT (90, insn->src(2));
      } else {
                  if (insn->defExists(1))
         else
            }
      void
   CodeEmitterGV100::emitFSWZADD()
   {
               // NP/PN swapped vs SM60
   for (int i = 0; i < 4; i++) {
      uint8_t p = ((insn->subOp >> (i * 2)) & 3);
   if (p == 1 || p == 2)
                     emitInsn (0x822);
   emitFMZ  (80, 1);
   emitRND  (78);
   emitField(77, 1, insn->lanes); /* abused for .ndv */
   emitGPR  (64, insn->src(1));
   emitField(32, 8, subOp);
   emitGPR  (24, insn->src(0));
      }
      void
   CodeEmitterGV100::emitMUFU()
   {
               switch (insn->op) {
   case OP_COS : mufu = 0; break;
   case OP_SIN : mufu = 1; break;
   case OP_EX2 : mufu = 2; break;
   case OP_LG2 : mufu = 3; break;
   case OP_RCP : mufu = 4 + 2 * insn->subOp; break;
   case OP_RSQ : mufu = 5 + 2 * insn->subOp; break;
   case OP_SQRT: mufu = 8; break;
   default:
      assert(!"invalid mufu");
               emitFormA(0x108, FA_RRR | FA_RIR | FA_RCR, EMPTY, NA(0), EMPTY);
      }
      /*******************************************************************************
   * fp64
   ******************************************************************************/
      void
   CodeEmitterGV100::emitDADD()
   {
      emitFormA(0x029, FA_RRR | FA_RRI | FA_RRC, NA(0), EMPTY, NA(1));
      }
      void
   CodeEmitterGV100::emitDFMA()
   {
      emitFormA(0x02b, FA_RRR | FA_RRI | FA_RRC | FA_RIR | FA_RCR, NA(0), NA(1), NA(2));
      }
      void
   CodeEmitterGV100::emitDMUL()
   {
      emitFormA(0x028, FA_RRR | FA_RIR | FA_RCR, NA(0), NA(1), EMPTY);
      }
      void
   CodeEmitterGV100::emitDSETP()
   {
               if (insn->src(1).getFile() == FILE_GPR)
         else
            if (insn->op != OP_SET) {
      switch (insn->op) {
   case OP_SET_AND: emitField(74, 2, 0); break;
   case OP_SET_OR : emitField(74, 2, 1); break;
   case OP_SET_XOR: emitField(74, 2, 2); break;
   default:
      assert(!"invalid set op");
      }
   emitNOT (90, insn->src(2));
      } else {
                  if (insn->defExists(1))
         else
         emitPRED (81, insn->def(0));
      }
      /*******************************************************************************
   * integer
   ******************************************************************************/
      void
   CodeEmitterGV100::emitBMSK()
   {
      emitFormA(0x01b, FA_RRR | FA_RIR | FA_RCR, __(0), __(1), EMPTY);
      }
      void
   CodeEmitterGV100::emitBREV()
   {
         }
      void
   CodeEmitterGV100::emitFLO()
   {
      emitFormA(0x100, FA_RRR | FA_RIR | FA_RCR, EMPTY, __(0), EMPTY);
   emitPRED (81);
   emitField(74, 1, insn->subOp == NV50_IR_SUBOP_BFIND_SAMT);
   emitField(73, 1, isSignedType(insn->dType));
      }
      void
   CodeEmitterGV100::emitIABS()
   {
         }
      void
   CodeEmitterGV100::emitIADD3()
   {
   //   emitFormA(0x010, FA_RRR | FA_RIR | FA_RCR, N_(0), N_(1), N_(2));
      emitFormA(0x010, FA_RRR | FA_RIR | FA_RCR, N_(0), N_(1), EMPTY);
   emitGPR  (64); //XXX: fix when switching back to N_(2)
   emitPRED (84, NULL); // .CC1
   emitPRED (81, insn->flagsDef >= 0 ? insn->getDef(insn->flagsDef) : NULL);
   if (insn->flagsSrc >= 0) {
      emitField(74, 1, 1); // .X
   emitPRED (87, insn->getSrc(insn->flagsSrc));
         }
      void
   CodeEmitterGV100::emitIMAD()
   {
      emitFormA(0x024, FA_RRR | FA_RRI | FA_RRC | FA_RIR | FA_RCR, __(0), __(1), N_(2));
      }
      void
   CodeEmitterGV100::emitIMAD_WIDE()
   {
      emitFormA(0x025, FA_RRR |          FA_RRC | FA_RIR | FA_RCR, __(0), __(1), N_(2));
   emitPRED (81);
      }
      void
   CodeEmitterGV100::emitISETP()
   {
                        if (insn->op != OP_SET) {
      switch (insn->op) {
   case OP_SET_AND: emitField(74, 2, 0); break;
   case OP_SET_OR : emitField(74, 2, 1); break;
   case OP_SET_XOR: emitField(74, 2, 2); break;
   default:
      assert(!"invalid set op");
      }
   emitNOT (90, insn->src(2));
      } else {
                  //XXX: CC->pred
   if (insn->flagsSrc >= 0) {
      assert(0);
      } else {
      emitNOT (71);
   if (!insn->subOp)
               if (insn->defExists(1))
         else
         emitPRED (81, insn->def(0));
   emitCond3(76, insn->setCond);
            if (insn->subOp) { // .EX
      assert(0);
   emitField(72, 1, 1);
         }
      void
   CodeEmitterGV100::emitLEA()
   {
               emitFormA(0x011, FA_RRR | FA_RIR | FA_RCR, N_(0), N_(2), EMPTY);
   emitPRED (81);
   emitIMMD (75, 5, insn->src(1));
      }
      void
   CodeEmitterGV100::emitLOP3_LUT()
   {
      emitFormA(0x012, FA_RRR | FA_RIR | FA_RCR, __(0), __(1), __(2));
   emitField(90, 1, 1);
   emitPRED (87);
   emitPRED (81);
   emitField(80, 1, 0); // .PAND
      }
      void
   CodeEmitterGV100::emitPOPC()
   {
      emitFormA(0x109, FA_RRR | FA_RIR | FA_RCR, EMPTY, __(0), EMPTY);
      }
      void
   CodeEmitterGV100::emitSGXT()
   {
      emitFormA(0x01a, FA_RRR | FA_RIR | FA_RCR, __(0), __(1), EMPTY);
   emitField(75, 1, 0); // .W
      }
      void
   CodeEmitterGV100::emitSHF()
   {
      emitFormA(0x019, FA_RRR | FA_RRI | FA_RRC | FA_RIR | FA_RCR, __(0), __(1), __(2));
   emitField(80, 1, !!(insn->subOp & NV50_IR_SUBOP_SHF_HI));
   emitField(76, 1, !!(insn->subOp & NV50_IR_SUBOP_SHF_R));
            switch (insn->sType) {
   case TYPE_S64: emitField(73, 2, 0); break;
   case TYPE_U64: emitField(73, 2, 1); break;
   case TYPE_S32: emitField(73, 2, 2); break;
   case TYPE_U32:
   default:
      emitField(73, 2, 3);
         }
      /*******************************************************************************
   * load/stores
   ******************************************************************************/
      void
   CodeEmitterGV100::emitALD()
   {
      emitInsn (0x321);
   emitField(74, 2, (insn->getDef(0)->reg.size / 4) - 1);
   emitGPR  (32, insn->src(0).getIndirect(1));
   emitO    (79);
   emitField(77, 1, insn->subOp); // .PHYS
   emitP    (76);
   emitADDR (24, 40, 10, 0, insn->src(0));
      }
      void
   CodeEmitterGV100::emitAST()
   {
      emitInsn (0x322);
   emitField(74, 2, (typeSizeof(insn->dType) / 4) - 1);
   emitGPR  (64, insn->src(0).getIndirect(1));
   emitField(77, 1, insn->subOp); // .PHYS
   emitP    (76);
   emitADDR (24, 40, 10, 0, insn->src(0));
      }
      void
   CodeEmitterGV100::emitATOM()
   {
               if (insn->subOp != NV50_IR_SUBOP_ATOM_CAS) {
               if (insn->subOp == NV50_IR_SUBOP_ATOM_EXCH)
         else
                  switch (insn->dType) {
   case TYPE_U32 : dType = 0; break;
   case TYPE_S32 : dType = 1; break;
   case TYPE_U64 : dType = 2; break;
   case TYPE_F32 : dType = 3; break;
   case TYPE_B128: dType = 4; break;
   case TYPE_S64 : dType = 5; break;
   default:
      assert(!"unexpected dType");
   dType = 0;
      }
      } else {
               switch (insn->dType) {
   case TYPE_U32: dType = 0; break;
   case TYPE_U64: dType = 2; break;
   default:
      assert(!"unexpected dType");
   dType = 0;
      }
   emitField(73, 3, dType);
               emitPRED (81);
   if (targ->getChipset() < 0x170) {
      emitField(79, 2, 2); // .INVALID0/./.STRONG/.INVALID3
      } else {
         }
   emitField(72, 1, insn->src(0).getIndirect(0)->getSize() == 8);
   emitGPR  (32, insn->src(1));
   emitADDR (24, 40, 24, 0, insn->src(0));
      }
      void
   CodeEmitterGV100::emitATOMS()
   {
               if (insn->subOp == NV50_IR_SUBOP_ATOM_CAS) {
      switch (insn->dType) {
   case TYPE_U32: dType = 0; break;
   case TYPE_S32: dType = 1; break;
   case TYPE_U64: dType = 2; break;
   default: assert(!"unexpected dType"); dType = 0; break;
            emitInsn (0x38d);
   emitField(87, 1, 0); // ATOMS.CAS/ATOMS.CAST
   emitField(73, 2, dType);
      } else {
               if (insn->subOp == NV50_IR_SUBOP_ATOM_EXCH)
         else
                  switch (insn->dType) {
   case TYPE_U32: dType = 0; break;
   case TYPE_S32: dType = 1; break;
   case TYPE_U64: dType = 2; break;
   default: assert(!"unexpected dType"); dType = 0; break;
                        emitGPR  (32, insn->src(1));
   emitADDR (24, 40, 24, 0, insn->src(0));
      }
      void
   gv100_interpApply(const FixupEntry *entry, uint32_t *code, const FixupData& data)
   {
      int ipa = entry->ipa;
            if (data.force_persample_interp &&
      (ipa & NV50_IR_INTERP_SAMPLE_MASK) == NV50_IR_INTERP_DEFAULT &&
   (ipa & NV50_IR_INTERP_MODE_MASK) != NV50_IR_INTERP_FLAT) {
               int sample;
   switch (ipa & NV50_IR_INTERP_SAMPLE_MASK) {
   case NV50_IR_INTERP_DEFAULT : sample = 0; break;
   case NV50_IR_INTERP_CENTROID: sample = 1; break;
   case NV50_IR_INTERP_OFFSET  : sample = 2; break;
   default: unreachable("invalid sample mode");
            int interp;
   switch (ipa & NV50_IR_INTERP_MODE_MASK) {
   case NV50_IR_INTERP_LINEAR     :
   case NV50_IR_INTERP_PERSPECTIVE: interp = 0; break;
   case NV50_IR_INTERP_FLAT       : interp = 1; break;
   case NV50_IR_INTERP_SC         : interp = 2; break;
   default: unreachable("invalid ipa mode");
            code[loc + 2] &= ~(0xf << 12);
   code[loc + 2] |= sample << 12;
      }
      void
   CodeEmitterGV100::emitIPA()
   {
      emitInsn (0x326);
            switch (insn->getInterpMode()) {
   case NV50_IR_INTERP_LINEAR     :
   case NV50_IR_INTERP_PERSPECTIVE: emitField(78, 2, 0); break;
   case NV50_IR_INTERP_FLAT       : emitField(78, 2, 1); break;
   case NV50_IR_INTERP_SC         : emitField(78, 2, 2); break;
   default:
      assert(!"invalid ipa mode");
               switch (insn->getSampleMode()) {
   case NV50_IR_INTERP_DEFAULT : emitField(76, 2, 0); break;
   case NV50_IR_INTERP_CENTROID: emitField(76, 2, 1); break;
   case NV50_IR_INTERP_OFFSET  : emitField(76, 2, 2); break;
   default:
      assert(!"invalid sample mode");
               if (insn->getSampleMode() != NV50_IR_INTERP_OFFSET) {
      emitGPR  (32);
      } else {
      emitGPR  (32, insn->src(1));
               assert(!insn->src(0).isIndirect(0));
   emitADDR (-1, 64, 8, 2, insn->src(0));
      }
      void
   CodeEmitterGV100::emitISBERD()
   {
      emitInsn(0x923);
   emitGPR (24, insn->src(0));
      }
      void
   CodeEmitterGV100::emitLDSTc(int posm, int poso)
   {
      int mode = 0;
   int order = 1;
            switch (insn->cache) {
   case CACHE_CA: mode = 0; order = 1; sm80 = 0x0; break; // .CTA
   case CACHE_CG: mode = 2; order = 2; sm80 = 0x7; break; // .STRONG.GPU
   case CACHE_CV: mode = 3; order = 2; sm80 = 0xa; break; // .STRONG.SYS
   default:
      assert(!"invalid caching mode");
               if (targ->getChipset() < 0x170) {
      emitField(poso, 2, order);
      } else {
            }
      void
   CodeEmitterGV100::emitLDSTs(int pos, DataType type)
   {
               switch (typeSizeof(type)) {
   case  1: data = isSignedType(type) ? 1 : 0; break;
   case  2: data = isSignedType(type) ? 3 : 2; break;
   case  4: data = 4; break;
   case  8: data = 5; break;
   case 16: data = 6; break;
   default:
      assert(!"bad type");
                  }
      void
   CodeEmitterGV100::emitLD()
   {
      emitInsn (0x980);
   if (targ->getChipset() < 0x170) {
      emitField(79, 2, 2); // .CONSTANT/./.STRONG/.MMIO
      } else {
         }
   emitLDSTs(73, insn->dType);
   emitField(72, 1, insn->src(0).getIndirect(0)->getSize() == 8);
   emitADDR (24, 32, 32, 0, insn->src(0));
      }
      void
   CodeEmitterGV100::emitLDC()
   {
      emitFormA(0x182, FA_RCR, EMPTY, __(0), EMPTY);
   emitField(78, 2, insn->subOp);
   emitLDSTs(73, insn->dType);
      }
      void
   CodeEmitterGV100::emitLDL()
   {
      emitInsn (0x983);
   emitField(84, 3, 1); // .EF/./.EL/.LU/.EU/.NA/.INVALID6/.INVALID7
   emitLDSTs(73, insn->dType);
   emitADDR (24, 40, 24, 0, insn->src(0));
      }
      void
   CodeEmitterGV100::emitLDS()
   {
      emitInsn (0x984);
   emitLDSTs(73, insn->dType);
   emitADDR (24, 40, 24, 0, insn->src(0));
      }
      void
   CodeEmitterGV100::emitOUT()
   {
      const int cut  = insn->op == OP_RESTART || insn->subOp;
            if (insn->op != OP_FINAL)
         else {
      emitFormA(0x124, FA_RRR | FA_RIR, __(0), EMPTY, EMPTY);
   if (targ->getChipset() >= 0x170)
      }
      }
      void
   CodeEmitterGV100::emitRED()
   {
               switch (insn->dType) {
   case TYPE_U32: dType = 0; break;
   case TYPE_S32: dType = 1; break;
   case TYPE_U64: dType = 2; break;
   case TYPE_F32: dType = 3; break;
   case TYPE_B128: dType = 4; break;
   case TYPE_S64: dType = 5; break;
   default: assert(!"unexpected dType"); dType = 0; break;
            emitInsn (0x98e);
   emitField(87, 3, insn->subOp);
   emitField(84, 3, 1); // 0=.EF, 1=, 2=.EL, 3=.LU, 4=.EU, 5=.NA
   if (targ->getChipset() < 0x170) {
      emitField(79, 2, 2); // .INVALID0/./.STRONG/.INVALID3
      } else {
         }
   emitField(73, 3, dType);
   emitField(72, 1, insn->src(0).getIndirect(0)->getSize() == 8);
   emitGPR  (32, insn->src(1));
      }
      void
   CodeEmitterGV100::emitST()
   {
      emitInsn (0x385);
   if (targ->getChipset() < 0x170) {
      emitField(79, 2, 2); // .INVALID0/./.STRONG/.MMIO
      } else {
         }
   emitLDSTs(73, insn->dType);
   emitField(72, 1, insn->src(0).getIndirect(0)->getSize() == 8);
   emitGPR  (64, insn->src(1));
      }
      void
   CodeEmitterGV100::emitSTL()
   {
      emitInsn (0x387);
   emitField(84, 3, 1); // .EF/./.EL/.LU/.EU/.NA/.INVALID6/.INVALID7
   emitLDSTs(73, insn->dType);
   emitADDR (24, 40, 24, 0, insn->src(0));
      }
      void
   CodeEmitterGV100::emitSTS()
   {
      emitInsn (0x388);
   emitLDSTs(73, insn->dType);
   emitADDR (24, 40, 24, 0, insn->src(0));
      }
      /*******************************************************************************
   * texture
   ******************************************************************************/
      void
   CodeEmitterGV100::emitTEXs(int pos)
   {
      int src1 = insn->predSrc == 1 ? 2 : 1;
   if (insn->srcExists(src1))
         else
      }
      void
   CodeEmitterGV100::emitTEX()
   {
      const TexInstruction *insn = this->insn->asTex();
            if (!insn->tex.levelZero) {
      switch (insn->op) {
   case OP_TEX: lodm = 0; break;
   case OP_TXB: lodm = 2; break;
   case OP_TXL: lodm = 3; break;
   default:
      assert(!"invalid tex op");
         } else {
                  if (insn->tex.rIndirectSrc < 0) {
      emitInsn (0xb60);
   emitField(54, 5, prog->driver->io.auxCBSlot);
      } else {
      emitInsn (0x361);
      }
   emitField(90, 1, insn->tex.liveOnly); // .NODEP
   emitField(87, 3, lodm);
   emitField(84, 3, 1); // 0=.EF, 1=, 2=.EL, 3=.LU, 4=.EU, 5=.NA
   emitField(78, 1, insn->tex.target.isShadow()); // .DC
   emitField(77, 1, insn->tex.derivAll); // .NDV
   emitField(76, 1, insn->tex.useOffsets == 1); // .AOFFI
   emitPRED (81);
   emitGPR  (64, insn->def(1));
   emitGPR  (16, insn->def(0));
   emitGPR  (24, insn->src(0));
   emitTEXs (32);
   emitField(63, 1, insn->tex.target.isArray());
   emitField(61, 2, insn->tex.target.isCube() ? 3 :
            }
      void
   CodeEmitterGV100::emitTLD()
   {
               if (insn->tex.rIndirectSrc < 0) {
      emitInsn (0xb66);
   emitField(54, 5, prog->driver->io.auxCBSlot);
      } else {
      emitInsn (0x367);
      }
   emitField(90, 1, insn->tex.liveOnly);
   emitField(87, 3, insn->tex.levelZero ? 1 /* .LZ */ : 3 /* .LL */);
   emitPRED (81);
   emitField(78, 1, insn->tex.target.isMS());
   emitField(76, 1, insn->tex.useOffsets == 1);
   emitField(72, 4, insn->tex.mask);
   emitGPR  (64, insn->def(1));
   emitField(63, 1, insn->tex.target.isArray());
   emitField(61, 2, insn->tex.target.isCube() ? 3 :
         emitTEXs (32);
   emitGPR  (24, insn->src(0));
      }
      void
   CodeEmitterGV100::emitTLD4()
   {
               int offsets = 0;
   switch (insn->tex.useOffsets) {
   case 4: offsets = 2; break;
   case 1: offsets = 1; break;
   case 0: offsets = 0; break;
   default: assert(!"invalid offsets count"); break;
            if (insn->tex.rIndirectSrc < 0) {
      emitInsn (0xb63);
   emitField(54, 5, prog->driver->io.auxCBSlot);
      } else {
      emitInsn (0x364);
      }
   emitField(90, 1, insn->tex.liveOnly);
   emitField(87, 2, insn->tex.gatherComp);
   emitField(84, 1, 1); // !.EF
   emitPRED (81);
   emitField(78, 1, insn->tex.target.isShadow());
   emitField(76, 2, offsets);
   emitField(72, 4, insn->tex.mask);
   emitGPR  (64, insn->def(1));
   emitField(63, 1, insn->tex.target.isArray());
   emitField(61, 2, insn->tex.target.isCube() ? 3 :
         emitTEXs (32);
   emitGPR  (24, insn->src(0));
      }
      void
   CodeEmitterGV100::emitTMML()
   {
               if (insn->tex.rIndirectSrc < 0) {
      emitInsn (0xb69);
   emitField(54, 5, prog->driver->io.auxCBSlot);
      } else {
      emitInsn (0x36a);
      }
   emitField(90, 1, insn->tex.liveOnly);
   emitField(77, 1, insn->tex.derivAll);
   emitField(72, 4, insn->tex.mask);
   emitGPR  (64, insn->def(1));
   emitField(63, 1, insn->tex.target.isArray());
   emitField(61, 2, insn->tex.target.isCube() ? 3 :
         emitTEXs (32);
   emitGPR  (24, insn->src(0));
      }
      void
   CodeEmitterGV100::emitTXD()
   {
               if (insn->tex.rIndirectSrc < 0) {
      emitInsn (0xb6c);
   emitField(54, 5, prog->driver->io.auxCBSlot);
      } else {
      emitInsn (0x36d);
      }
   emitField(90, 1, insn->tex.liveOnly);
   emitPRED (81);
   emitField(76, 1, insn->tex.useOffsets == 1);
   emitField(72, 4, insn->tex.mask);
   emitGPR  (64, insn->def(1));
   emitField(63, 1, insn->tex.target.isArray());
   emitField(61, 2, insn->tex.target.isCube() ? 3 :
         emitTEXs (32);
   emitGPR  (24, insn->src(0));
      }
      void
   CodeEmitterGV100::emitTXQ()
   {
      const TexInstruction *insn = this->insn->asTex();
            switch (insn->tex.query) {
   case TXQ_DIMS           : type = 0x00; break;
   case TXQ_TYPE           : type = 0x01; break;
   case TXQ_SAMPLE_POSITION: type = 0x02; break;
   default:
      assert(!"invalid txq query");
               if (insn->tex.rIndirectSrc < 0) {
      emitInsn (0xb6f);
   emitField(54, 5, prog->driver->io.auxCBSlot);
      } else {
      emitInsn (0x370);
      }
   emitField(90, 1, insn->tex.liveOnly);
   emitField(72, 4, insn->tex.mask);
   emitGPR  (64, insn->def(1));
   emitField(62, 2, type);
   emitGPR  (24, insn->src(0));
      }
      /*******************************************************************************
   * surface
   ******************************************************************************/
      void
   CodeEmitterGV100::emitSUHandle(const int s)
   {
                        if (insn->src(s).getFile() == FILE_GPR) {
         } else {
      assert(0);
   //XXX: not done
   ImmediateValue *imm = insn->getSrc(s)->asImm();
   assert(imm);
   emitField(0x33, 1, 1);
         }
      void
   CodeEmitterGV100::emitSUTarget()
   {
      const TexInstruction *insn = this->insn->asTex();
                     if (insn->tex.target == TEX_TARGET_BUFFER) {
         } else if (insn->tex.target == TEX_TARGET_1D_ARRAY) {
         } else if (insn->tex.target == TEX_TARGET_2D ||
               } else if (insn->tex.target == TEX_TARGET_2D_ARRAY ||
            insn->tex.target == TEX_TARGET_CUBE ||
      } else if (insn->tex.target == TEX_TARGET_3D) {
         } else {
         }
      }
      void
   CodeEmitterGV100::emitSUATOM()
   {
      const TexInstruction *insn = this->insn->asTex();
            if (insn->subOp == NV50_IR_SUBOP_ATOM_CAS)
         else
                     // destination type
   switch (insn->dType) {
   case TYPE_S32: type = 1; break;
   case TYPE_U64: type = 2; break;
   case TYPE_F32: type = 3; break;
   case TYPE_S64: type = 5; break;
   default:
      assert(insn->dType == TYPE_U32);
               // atomic operation
   if (insn->subOp == NV50_IR_SUBOP_ATOM_CAS) {
         } else if (insn->subOp == NV50_IR_SUBOP_ATOM_EXCH) {
         } else {
                  emitField(87, 4, subOp);
   emitPRED (81);
   if (targ->getChipset() < 0x170)
         emitField(73, 3, type);
   emitField(72, 1, 0); // .BA
   emitGPR  (32, insn->src(1));
   emitGPR  (24, insn->src(0));
               }
      void
   CodeEmitterGV100::emitSULD()
   {
      const TexInstruction *insn = this->insn->asTex();
            if (insn->op == OP_SULDB) {
      emitInsn(0x99a);
            switch (insn->dType) {
   case TYPE_U8:   type = 0; break;
   case TYPE_S8:   type = 1; break;
   case TYPE_U16:  type = 2; break;
   case TYPE_S16:  type = 3; break;
   case TYPE_U32:  type = 4; break;
   case TYPE_U64:  type = 5; break;
   case TYPE_B128: type = 6; break;
   default:
      assert(0);
      }
      } else {
      emitInsn(0x998);
   emitSUTarget();
               emitPRED (81);
            emitGPR  (16, insn->def(0));
               }
      void
   CodeEmitterGV100::emitSUST()
   {
                  #if 0
      if (insn->op == OP_SUSTB)
      #endif
               emitLDSTc(77, 79);
   emitField(72, 4, 0xf); // rgba
   emitGPR(32, insn->src(1));
   emitGPR(24, insn->src(0));
      }
      /*******************************************************************************
   * misc
   ******************************************************************************/
      void
   CodeEmitterGV100::emitAL2P()
   {
      emitInsn (0x920);
   emitO    (79);
   emitField(74, 2, (insn->getDef(0)->reg.size / 4) - 1);
   emitField(40, 11, insn->src(0).get()->reg.data.offset);
   emitGPR  (24, insn->src(0).getIndirect(0));
      }
      void
   CodeEmitterGV100::emitBAR()
   {
               //XXX: ILLEGAL_INSTR_PARAM - why?
   if (targ->getChipset() >= 0x170) {
      emitNOP();
               // 80
   //    01: DEFER_BLOCKING
   // 78:77
   //    00: SYNC
   //    01: ARV
   //    02: RED
   //    03: SCAN
   // 75:74
   //    00: RED.POPC
   //    01: RED.AND
            switch (insn->subOp) {
   case NV50_IR_SUBOP_BAR_RED_POPC: subop = 0x02; redop = 0x00; break;
   case NV50_IR_SUBOP_BAR_RED_AND : subop = 0x02; redop = 0x01; break;
   case NV50_IR_SUBOP_BAR_RED_OR  : subop = 0x02; redop = 0x02; break;
   case NV50_IR_SUBOP_BAR_ARRIVE  : subop = 0x01; break;
   default:
      subop = 0x00;
   assert(insn->subOp == NV50_IR_SUBOP_BAR_SYNC);
               if (insn->src(0).getFile() == FILE_GPR) {
      emitInsn ((1 << 9) | 0x11d);
      } else {
      ImmediateValue *imm = insn->getSrc(0)->asImm();
   assert(imm);
   if (insn->src(1).getFile() == FILE_GPR) {
      emitInsn ((4 << 9) | 0x11d);
      } else {
         }
               emitField(77, 2, subop);
            if (insn->srcExists(2) && (insn->predSrc != 2)) {
      emitField(90, 1, insn->src(2).mod == Modifier(NV50_IR_MOD_NOT));
      } else {
            }
      void
   CodeEmitterGV100::emitCCTL()
   {
      if (insn->src(0).getFile() == FILE_MEMORY_GLOBAL)
         else
         emitField(87, 4, insn->subOp);
   emitField(72, 1, insn->src(0).getIndirect(0)->getSize() == 8);
      }
      void
   CodeEmitterGV100::emitMEMBAR()
   {
      emitInsn (0x992);
   switch (NV50_IR_SUBOP_MEMBAR_SCOPE(insn->subOp)) {
   case NV50_IR_SUBOP_MEMBAR_CTA: emitField(76, 3, 0); break;
   case NV50_IR_SUBOP_MEMBAR_GL : emitField(76, 3, 2); break;
   case NV50_IR_SUBOP_MEMBAR_SYS: emitField(76, 3, 3); break;
   default:
      assert(!"invalid scope");
         }
      void
   CodeEmitterGV100::emitPIXLD()
   {
      emitInsn (0x925);
   switch (insn->subOp) {
   case NV50_IR_SUBOP_PIXLD_COVMASK : emitField(78, 3, 1); break; // .COVMASK
   case NV50_IR_SUBOP_PIXLD_SAMPLEID: emitField(78, 3, 3); break; // .MY_INDEX
   default:
      assert(0);
      }
   emitPRED (71);
      }
      void
   CodeEmitterGV100::emitPLOP3_LUT()
   {
               switch (insn->op) {
   case OP_AND: op[0] = 0xf0 & 0xcc; break;
   case OP_OR : op[0] = 0xf0 | 0xcc; break;
   case OP_XOR: op[0] = 0xf0 ^ 0xcc; break;
   default:
      assert(!"invalid PLOP3");
               emitInsn(0x81c);
   emitNOT (90, insn->src(0));
   emitPRED(87, insn->src(0));
   emitPRED(84); // def(1)
   emitPRED(81, insn->def(0));
   emitNOT (80, insn->src(1));
   emitPRED(77, insn->src(1));
   emitField(72, 5, op[0] >> 3);
   emitNOT (71); // src(2)
   emitPRED(68); // src(2)
   emitField(64, 3, op[0] & 7);
      }
      void
   CodeEmitterGV100::emitVOTE()
   {
      const ImmediateValue *imm;
            int r = -1, p = -1;
   for (int i = 0; insn->defExists(i); i++) {
      if (insn->def(i).getFile() == FILE_GPR)
         else if (insn->def(i).getFile() == FILE_PREDICATE)
               emitInsn (0x806);
   emitField(72, 2, insn->subOp);
   if (r >= 0)
         else
         if (p >= 0)
         else
            switch (insn->src(0).getFile()) {
   case FILE_PREDICATE:
      emitField(90, 1, insn->src(0).mod == Modifier(NV50_IR_MOD_NOT));
   emitPRED (87, insn->src(0));
      case FILE_IMMEDIATE:
      imm = insn->getSrc(0)->asImm();
   assert(imm);
   u32 = imm->reg.data.u32;
   assert(u32 == 0 || u32 == 1);
   emitField(90, 1, u32 == 0);
   emitPRED (87);
      default:
      assert(!"Unhandled src");
         }
      bool
   CodeEmitterGV100::emitInstruction(Instruction *i)
   {
               switch (insn->op) {
   case OP_ABS:
      assert(!isFloatType(insn->dType));
   emitIABS();
      case OP_ADD:
      if (isFloatType(insn->dType)) {
      if (insn->dType == TYPE_F32)
         else
      } else {
         }
      case OP_AFETCH:
      emitAL2P();
      case OP_AND:
   case OP_OR:
   case OP_XOR:
      if (insn->def(0).getFile() == FILE_PREDICATE) {
         } else {
      assert(!"invalid logop");
      }
      case OP_ATOM:
      if (insn->src(0).getFile() == FILE_MEMORY_SHARED)
         else
      if (!insn->defExists(0) && insn->subOp < NV50_IR_SUBOP_ATOM_CAS)
         else
         case OP_BAR:
      emitBAR();
      case OP_BFIND:
      emitFLO();
      case OP_BMSK:
      emitBMSK();
      case OP_BREV:
      emitBREV();
      case OP_BRA:
   case OP_JOIN: //XXX
      emitBRA();
      case OP_CCTL:
      emitCCTL();
      case OP_CEIL:
   case OP_CVT:
   case OP_FLOOR:
   case OP_TRUNC:
      if (insn->op == OP_CVT && (insn->def(0).getFile() == FILE_PREDICATE ||
                              insn->def(0).getFile() == FILE_BARRIER ||
      } else if (isFloatType(insn->dType)) {
      if (isFloatType(insn->sType)) {
      if (insn->sType == insn->dType)
         else
      } else {
            } else {
      if (isFloatType(insn->sType)) {
         } else {
      assert(!"I2I");
         }
      case OP_COS:
   case OP_EX2:
   case OP_LG2:
   case OP_RCP:
   case OP_RSQ:
   case OP_SIN:
   case OP_SQRT:
      emitMUFU();
      case OP_DISCARD:
      emitKILL();
      case OP_EMIT:
   case OP_FINAL:
   case OP_RESTART:
      emitOUT();
      case OP_EXIT:
      emitEXIT();
      case OP_EXPORT:
      emitAST();
      case OP_FMA:
   case OP_MAD:
      if (isFloatType(insn->dType)) {
      if (insn->dType == TYPE_F32)
         else
      } else {
      if (typeSizeof(insn->dType) != 8)
         else
      }
      case OP_JOINAT: //XXX
      emitNOP();
      case OP_LINTERP:
      emitIPA();
      case OP_LOAD:
      switch (insn->src(0).getFile()) {
   case FILE_MEMORY_CONST : emitLDC(); break;
   case FILE_MEMORY_LOCAL : emitLDL(); break;
   case FILE_MEMORY_SHARED: emitLDS(); break;
   case FILE_MEMORY_GLOBAL: emitLD(); break;
   default:
      assert(!"invalid load");
   emitNOP();
      }
      case OP_LOP3_LUT:
      emitLOP3_LUT();
      case OP_MAX:
   case OP_MIN:
      if (isFloatType(insn->dType)) {
      if (insn->dType == TYPE_F32) {
         } else {
      assert(!"invalid FMNMX");
         } else {
      assert(!"invalid MNMX");
      }
      case OP_MEMBAR:
      emitMEMBAR();
      case OP_MOV:
      emitMOV();
      case OP_MUL:
      if (isFloatType(insn->dType)) {
      if (insn->dType == TYPE_F32)
         else
      } else {
      assert(!"invalid IMUL");
      }
      case OP_PERMT:
      emitPRMT();
      case OP_PFETCH:
      emitISBERD();
      case OP_PIXLD:
      emitPIXLD();
      case OP_POPCNT:
      emitPOPC();
      case OP_QUADOP:
      emitFSWZADD();
      case OP_RDSV:
      if (targ->isCS2RSV(insn->getSrc(0)->reg.data.sv.sv))
         else
            case OP_SELP:
      emitSEL();
      case OP_SET:
   case OP_SET_AND:
   case OP_SET_OR:
   case OP_SET_XOR:
      if (insn->def(0).getFile() != FILE_PREDICATE) {
      if (isFloatType(insn->dType)) {
      if (insn->dType == TYPE_F32) {
         } else {
      assert(!"invalid FSET");
         } else {
      assert(!"invalid SET");
         } else {
      if (isFloatType(insn->sType))
      if (insn->sType == TYPE_F64)
         else
      else
      }
      case OP_SGXT:
      emitSGXT();
      case OP_SHF:
      emitSHF();
      case OP_SHFL:
      emitSHFL();
      case OP_SHLADD:
      emitLEA();
      case OP_STORE:
      switch (insn->src(0).getFile()) {
   case FILE_MEMORY_LOCAL : emitSTL(); break;
   case FILE_MEMORY_SHARED: emitSTS(); break;
   case FILE_MEMORY_GLOBAL: emitST(); break;
   default:
      assert(!"invalid store");
   emitNOP();
      }
      case OP_SULDB:
   case OP_SULDP:
      emitSULD();
      case OP_SUREDB:
   case OP_SUREDP:
      emitSUATOM();
      case OP_SUSTB:
   case OP_SUSTP:
      emitSUST();
      case OP_TEX:
   case OP_TXB:
   case OP_TXL:
      emitTEX();
      case OP_TXD:
      emitTXD();
      case OP_TXF:
      emitTLD();
      case OP_TXG:
      emitTLD4();
      case OP_TXLQ:
      emitTMML();
      case OP_TXQ:
      emitTXQ();
      case OP_VFETCH:
      emitALD();
      case OP_VOTE:
      emitVOTE();
      case OP_WARPSYNC:
      emitWARPSYNC();
      default:
      assert(!"invalid opcode");
   emitNOP();
               code[3] &= 0x000001ff;
   code[3] |= insn->sched << 9;
   code += 4;
   codeSize += 16;
      }
      void
   CodeEmitterGV100::prepareEmission(BasicBlock *bb)
   {
      Function *func = bb->getFunction();
   Instruction *i;
                     for (; j >= 0; --j) {
      BasicBlock *in = func->bbArray[j];
            if (exit && exit->op == OP_BRA && exit->asFlow()->target.bb == bb) {
                                       }
   bb->binPos = in->binPos + in->binSize;
   if (in->binSize) // no more no-op branches to bb
      }
            if (!bb->getExit())
            for (i = bb->getEntry(); i; i = i->next) {
      i->encSize = getMinEncodingSize(i);
                           }
      void
   CodeEmitterGV100::prepareEmission(Function *func)
   {
      SchedDataCalculatorGM107 sched(targ);
   CodeEmitter::prepareEmission(func);
      }
      void
   CodeEmitterGV100::prepareEmission(Program *prog)
   {
      for (ArrayList::Iterator fi = prog->allFuncs.iterator();
      !fi.end(); fi.next()) {
   Function *func = reinterpret_cast<Function *>(fi.get());
   func->binPos = prog->binSize;
   prepareEmission(func);
                  }
      CodeEmitterGV100::CodeEmitterGV100(TargetGV100 *target)
         {
      code = NULL;
   codeSize = codeSizeLimit = 0;
      }
   };
