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
   #include "nv50_ir.h"
   #include "nv50_ir_build_util.h"
      #include "nv50_ir_target_nvc0.h"
   #include "nv50_ir_lowering_gv100.h"
      #include <limits>
      namespace nv50_ir {
      bool
   GV100LegalizeSSA::handleCMP(Instruction *i)
   {
               bld.mkCmp(OP_SET, reverseCondCode(i->asCmp()->setCond), TYPE_U8, pred,
         bld.mkOp3(OP_SELP, TYPE_U32, i->getDef(0), i->getSrc(0), i->getSrc(1), pred);
      }
      // NIR deals with most of these for us, but codegen generates more in pointer
   // calculations from other lowering passes.
   bool
   GV100LegalizeSSA::handleIADD64(Instruction *i)
   {
      Value *carry = bld.getSSA(1, FILE_PREDICATE);
   Value *def[2] = { bld.getSSA(), bld.getSSA() };
            for (int s = 0; s < 2; s++) {
      if (i->getSrc(s)->reg.size == 8) {
         } else {
      src[s][0] = i->getSrc(s);
                  bld.mkOp2(OP_ADD, TYPE_U32, def[0], src[0][0], src[1][0])->
         bld.mkOp2(OP_ADD, TYPE_U32, def[1], src[0][1], src[1][1])->
         bld.mkOp2(OP_MERGE, i->dType, i->getDef(0), def[0], def[1]);
      }
      bool
   GV100LegalizeSSA::handleIMAD_HIGH(Instruction *i)
   {
      Value *def = bld.getSSA(8), *defs[2];
            if (i->srcExists(2) &&
      (!i->getSrc(2)->asImm() || i->getSrc(2)->asImm()->reg.data.u32)) {
   Value *src2s[2] = { bld.getSSA(), bld.getSSA() };
   bld.mkMov(src2s[0], bld.mkImm(0));
   bld.mkMov(src2s[1], i->getSrc(2));
      } else {
                  bld.mkOp3(OP_MAD, isSignedType(i->sType) ? TYPE_S64 : TYPE_U64, def,
            bld.mkSplit(defs, 4, def);
   i->def(0).replace(defs[1], false);
      }
      // XXX: We should be able to do this in GV100LoweringPass, but codegen messes
   //      up somehow and swaps the condcode without swapping the sources.
   //      - tests/spec/glsl-1.50/execution/geometry/primitive-id-in.shader_test
   bool
   GV100LegalizeSSA::handleIMNMX(Instruction *i)
   {
               bld.mkCmp(OP_SET, (i->op == OP_MIN) ? CC_LT : CC_GT, i->dType, pred,
         bld.mkOp3(OP_SELP, i->dType, i->getDef(0), i->getSrc(0), i->getSrc(1), pred);
      }
      bool
   GV100LegalizeSSA::handleIMUL(Instruction *i)
   {
      if (i->subOp == NV50_IR_SUBOP_MUL_HIGH)
            bld.mkOp3(OP_MAD, i->dType, i->getDef(0), i->getSrc(0), i->getSrc(1),
            }
      bool
   GV100LegalizeSSA::handleLOP2(Instruction *i)
   {
      uint8_t src0 = NV50_IR_SUBOP_LOP3_LUT_SRC0;
   uint8_t src1 = NV50_IR_SUBOP_LOP3_LUT_SRC1;
            if (i->src(0).mod & Modifier(NV50_IR_MOD_NOT))
         if (i->src(1).mod & Modifier(NV50_IR_MOD_NOT))
            switch (i->op) {
   case OP_AND: subOp = src0 & src1; break;
   case OP_OR : subOp = src0 | src1; break;
   case OP_XOR: subOp = src0 ^ src1; break;
   default:
                  bld.mkOp3(OP_LOP3_LUT, TYPE_U32, i->getDef(0), i->getSrc(0), i->getSrc(1),
            }
      bool
   GV100LegalizeSSA::handleNOT(Instruction *i)
   {
      bld.mkOp3(OP_LOP3_LUT, TYPE_U32, i->getDef(0), bld.mkImm(0), i->getSrc(0),
            }
      bool
   GV100LegalizeSSA::handlePREEX2(Instruction *i)
   {
      i->def(0).replace(i->src(0), false);
      }
      bool
   GV100LegalizeSSA::handleQUADON(Instruction *i)
   {
      bld.mkBMov(i->getDef(0), bld.mkTSVal(TS_MACTIVE));
   Instruction *b = bld.mkBMov(bld.mkTSVal(TS_PQUAD_MACTIVE), i->getDef(0));
   b->fixed = 1;
      }
      bool
   GV100LegalizeSSA::handleQUADPOP(Instruction *i)
   {
      Instruction *b = bld.mkBMov(bld.mkTSVal(TS_MACTIVE), i->getSrc(0));
   b->fixed = 1;
      }
      bool
   GV100LegalizeSSA::handleSET(Instruction *i)
   {
      Value *src2 = i->srcExists(2) ? i->getSrc(2) : NULL;
   Value *pred = bld.getSSA(1, FILE_PREDICATE), *met;
            if (isFloatType(i->dType)) {
      if (i->sType == TYPE_F32)
            } else {
                  xsetp = bld.mkCmp(i->op, i->asCmp()->setCond, TYPE_U8, pred, i->sType,
         xsetp->src(0).mod = i->src(0).mod;
   xsetp->src(1).mod = i->src(1).mod;
   xsetp->setSrc(2, src2);
            i = bld.mkOp3(OP_SELP, TYPE_U32, i->getDef(0), bld.mkImm(0), met, pred);
   i->src(2).mod = Modifier(NV50_IR_MOD_NOT);
      }
      bool
   GV100LegalizeSSA::handleSHFL(Instruction *i)
   {
      Instruction *sync = new_Instruction(func, OP_WARPSYNC, TYPE_NONE);
   sync->fixed = 1;
   sync->setSrc(0, bld.mkImm(0xffffffff));
   i->bb->insertBefore(i, sync);
      }
      bool
   GV100LegalizeSSA::handleShift(Instruction *i)
   {
      Value *zero = bld.mkImm(0);
   Value *src1 = i->getSrc(1);
   Value *src0, *src2;
            if (i->op == OP_SHL && i->src(0).getFile() == FILE_GPR) {
      src0 = i->getSrc(0);
      } else {
      src0 = zero;
   src2 = i->getSrc(0);
      }
   if (i->subOp & NV50_IR_SUBOP_SHIFT_WRAP)
            bld.mkOp3(OP_SHF, i->dType, i->getDef(0), src0, src1, src2)->subOp = subOp;
      }
      bool
   GV100LegalizeSSA::handleSUB(Instruction *i)
   {
      Instruction *xadd =
         xadd->src(0).mod = i->src(0).mod;
   xadd->src(1).mod = i->src(1).mod ^ Modifier(NV50_IR_MOD_NEG);
   xadd->ftz = i->ftz;
      }
      bool
   GV100LegalizeSSA::visit(Instruction *i)
   {
               bld.setPosition(i, false);
   if (i->sType == TYPE_F32 && i->dType != TYPE_F16 &&
      prog->getType() != Program::TYPE_COMPUTE)
         switch (i->op) {
   case OP_AND:
   case OP_OR:
   case OP_XOR:
      if (i->def(0).getFile() != FILE_PREDICATE)
            case OP_NOT:
      lowered = handleNOT(i);
      case OP_SHL:
   case OP_SHR:
      lowered = handleShift(i);
      case OP_SET:
   case OP_SET_AND:
   case OP_SET_OR:
   case OP_SET_XOR:
      if (i->def(0).getFile() != FILE_PREDICATE)
            case OP_SLCT:
      lowered = handleCMP(i);
      case OP_PREEX2:
      lowered = handlePREEX2(i);
      case OP_MUL:
      if (!isFloatType(i->dType))
            case OP_MAD:
      if (!isFloatType(i->dType) && i->subOp == NV50_IR_SUBOP_MUL_HIGH)
            case OP_SHFL:
      lowered = handleSHFL(i);
      case OP_QUADON:
      lowered = handleQUADON(i);
      case OP_QUADPOP:
      lowered = handleQUADPOP(i);
      case OP_SUB:
      lowered = handleSUB(i);
      case OP_MAX:
   case OP_MIN:
      if (!isFloatType(i->dType))
            case OP_ADD:
      if (!isFloatType(i->dType) && typeSizeof(i->dType) == 8)
            case OP_PFETCH:
      handlePFETCH(i);
      case OP_LOAD:
      handleLOAD(i);
      default:
                  if (lowered)
               }
      bool
   GV100LoweringPass::handleDMNMX(Instruction *i)
   {
      Value *pred = bld.getSSA(1, FILE_PREDICATE);
            bld.mkCmp(OP_SET, (i->op == OP_MIN) ? CC_LT : CC_GT, TYPE_U32, pred,
         bld.mkSplit(src0, 4, i->getSrc(0));
   bld.mkSplit(src1, 4, i->getSrc(1));
   bld.mkSplit(dest, 4, i->getDef(0));
   bld.mkOp3(OP_SELP, TYPE_U32, dest[0], src0[0], src1[0], pred);
   bld.mkOp3(OP_SELP, TYPE_U32, dest[1], src0[1], src1[1], pred);
   bld.mkOp2(OP_MERGE, TYPE_U64, i->getDef(0), dest[0], dest[1]);
      }
      bool
   GV100LoweringPass::handleEXTBF(Instruction *i)
   {
      Value *bit = bld.getScratch();
   Value *cnt = bld.getScratch();
   Value *mask = bld.getScratch();
            bld.mkOp3(OP_PERMT, TYPE_U32, bit, i->getSrc(1), bld.mkImm(0x4440), zero);
   bld.mkOp3(OP_PERMT, TYPE_U32, cnt, i->getSrc(1), bld.mkImm(0x4441), zero);
   bld.mkOp2(OP_BMSK, TYPE_U32, mask, bit, cnt);
   bld.mkOp2(OP_AND, TYPE_U32, mask, i->getSrc(0), mask);
   bld.mkOp2(OP_SHR, TYPE_U32, i->getDef(0), mask, bit);
   if (isSignedType(i->dType))
               }
      bool
   GV100LoweringPass::handleFLOW(Instruction *i)
   {
      i->op = OP_BRA;
      }
      bool
   GV100LoweringPass::handleI2I(Instruction *i)
   {
      bld.mkCvt(OP_CVT, TYPE_F32, i->getDef(0), i->sType, i->getSrc(0))->
         bld.mkCvt(OP_CVT, i->dType, i->getDef(0), TYPE_F32, i->getDef(0));
      }
      bool
   GV100LoweringPass::handleINSBF(Instruction *i)
   {
      Value *bit = bld.getScratch();
   Value *cnt = bld.getScratch();
   Value *mask = bld.getScratch();
   Value *src0 = bld.getScratch();
            bld.mkOp3(OP_PERMT, TYPE_U32, bit, i->getSrc(1), bld.mkImm(0x4440), zero);
   bld.mkOp3(OP_PERMT, TYPE_U32, cnt, i->getSrc(1), bld.mkImm(0x4441), zero);
            bld.mkOp2(OP_AND, TYPE_U32, src0, i->getSrc(0), mask);
            bld.mkOp2(OP_SHL, TYPE_U32, mask, mask, bit);
   bld.mkOp3(OP_LOP3_LUT, TYPE_U32, i->getDef(0), src0, i->getSrc(2), mask)->
               }
      bool
   GV100LoweringPass::handlePINTERP(Instruction *i)
   {
      Value *src2 = i->srcExists(2) ? i->getSrc(2) : NULL;
            ipa = bld.mkOp2(OP_LINTERP, TYPE_F32, i->getDef(0), i->getSrc(0), src2);
   ipa->ipa = i->ipa;
            if (i->getInterpMode() == NV50_IR_INTERP_SC) {
      ipa->setDef(1, bld.getSSA(1, FILE_PREDICATE));
                  }
      bool
   GV100LoweringPass::handlePREFLOW(Instruction *i)
   {
         }
      bool
   GV100LoweringPass::handlePRESIN(Instruction *i)
   {
      const float f = 1.0 / (2.0 * 3.14159265);
   bld.mkOp2(OP_MUL, i->dType, i->getDef(0), i->getSrc(0), bld.mkImm(f));
      }
      bool
   GV100LoweringPass::visit(Instruction *i)
   {
                        switch (i->op) {
   case OP_BREAK:
   case OP_CONT:
      lowered = handleFLOW(i);
      case OP_PREBREAK:
   case OP_PRECONT:
      lowered = handlePREFLOW(i);
      case OP_CVT:
      if (i->src(0).getFile() != FILE_PREDICATE &&
      i->def(0).getFile() != FILE_PREDICATE &&
   !isFloatType(i->dType) && !isFloatType(i->sType))
         case OP_EXTBF:
      lowered = handleEXTBF(i);
      case OP_INSBF:
      lowered = handleINSBF(i);
      case OP_MAX:
   case OP_MIN:
      if (i->dType == TYPE_F64)
            case OP_PINTERP:
      lowered = handlePINTERP(i);
      case OP_PRESIN:
      lowered = handlePRESIN(i);
      default:
                  if (lowered)
               }
      } // namespace nv50_ir
