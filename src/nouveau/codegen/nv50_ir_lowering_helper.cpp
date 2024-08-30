   /*
   * Copyright 2018 Red Hat Inc.
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
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   *
   * Authors: Karol Herbst <kherbst@redhat.com>
   */
      #include "nv50_ir_lowering_helper.h"
      namespace nv50_ir {
      bool
   LoweringHelper::visit(Instruction *insn)
   {
      switch (insn->op) {
   case OP_ABS:
         case OP_CVT:
         case OP_MAX:
   case OP_MIN:
         case OP_MOV:
         case OP_NEG:
         case OP_SAT:
         case OP_SLCT:
         case OP_AND:
   case OP_NOT:
   case OP_OR:
   case OP_XOR:
         default:
            }
      bool
   LoweringHelper::handleABS(Instruction *insn)
   {
      DataType dTy = insn->dType;
   if (!(dTy == TYPE_U64 || dTy == TYPE_S64))
                     Value *neg = bld.getSSA(8);
   Value *negComp[2], *srcComp[2];
   Value *lo = bld.getSSA(), *hi = bld.getSSA();
   bld.mkOp2(OP_SUB, dTy, neg, bld.mkImm((uint64_t)0), insn->getSrc(0));
   bld.mkSplit(negComp, 4, neg);
   bld.mkSplit(srcComp, 4, insn->getSrc(0));
   bld.mkCmp(OP_SLCT, CC_LT, TYPE_S32, lo, TYPE_S32, negComp[0], srcComp[0], srcComp[1]);
   bld.mkCmp(OP_SLCT, CC_LT, TYPE_S32, hi, TYPE_S32, negComp[1], srcComp[1], srcComp[1]);
   insn->op = OP_MERGE;
   insn->setSrc(0, lo);
               }
      bool
   LoweringHelper::handleCVT(Instruction *insn)
   {
      DataType dTy = insn->dType;
                     /* We can't convert from 32bit floating point to 8bit integer and from 64bit
   * floating point to any integer smaller than 32bit, hence add an instruction
   * to convert to a 32bit integer first.
   */
   if (((typeSizeof(dTy) == 1) && isFloatType(sTy)) ||
      ((typeSizeof(dTy) <= 2) && sTy == TYPE_F64)) {
   Value *tmp = insn->getDef(0);
            insn->setType(tmpTy, sTy);
   insn->setDef(0, bld.getSSA());
                                 /* We can't convert from a 64 bit integer to any integer smaller than 64 bit
   * directly, hence split the value first and then cvt / mov to the target
   * type.
   */
   if (isIntType(dTy) && typeSizeof(dTy) <= 4 &&
      isIntType(sTy) && typeSizeof(sTy) == 8) {
   DataType tmpTy = (isSignedIntType(dTy)) ? TYPE_S32 : TYPE_U32;
            bld.mkSplit(src, 4, insn->getSrc(0));
            /* For a 32 bit integer, we just need to mov the value to it's
   * destination. */
   if (typeSizeof(dTy) == 4) {
         } else { /* We're smaller than 32 bit, hence convert. */
      insn->op = OP_CVT;
                           /* We can't convert to signed 64 bit integrers, hence shift the upper word
   * and merge. For sources smaller than 32 bit, convert to 32 bit first.
   */
   if (dTy == TYPE_S64 && isSignedIntType(sTy) && typeSizeof(sTy) <= 4) {
      Value *tmpExtbf;
            if (typeSizeof(sTy) < 4) {
      unsigned int interval = typeSizeof(sTy) == 1 ? 0x800 : 0x1000;
   tmpExtbf = bld.getSSA();
   bld.mkOp2(OP_EXTBF, TYPE_S32, tmpExtbf, insn->getSrc(0),
            } else {
                           insn->op = OP_MERGE;
                        if (dTy == TYPE_U64 && isUnsignedIntType(sTy) && typeSizeof(sTy) <= 4) {
      insn->op = OP_MERGE;
                           }
      bool
   LoweringHelper::handleMAXMIN(Instruction *insn)
   {
      DataType dTy = insn->dType;
   if (!(dTy == TYPE_U64 || dTy == TYPE_S64))
            DataType sTy = typeOfSize(4, false, isSignedIntType(dTy));
            Value *flag = bld.getSSA(1, FILE_FLAGS);
   Value *src0[2];
   Value *src1[2];
            bld.mkSplit(src0, 4, insn->getSrc(0));
            def[0] = bld.getSSA();
            Instruction *hi = bld.mkOp2(insn->op, sTy, def[1], src0[1], src1[1]);
   hi->subOp = NV50_IR_SUBOP_MINMAX_HIGH;
            Instruction *lo = bld.mkOp2(insn->op, sTy, def[0], src0[0], src1[0]);
   lo->subOp = NV50_IR_SUBOP_MINMAX_LOW;
            insn->op = OP_MERGE;
   insn->setSrc(0, def[0]);
               }
      bool
   LoweringHelper::handleMOV(Instruction *insn)
   {
               if (typeSizeof(dTy) != 8)
                     if (reg.file != FILE_IMMEDIATE)
                     Value *hi = bld.getSSA();
            bld.loadImm(lo, (uint32_t)(reg.data.u64 & 0xffffffff));
            insn->op = OP_MERGE;
   insn->setSrc(0, lo);
               }
      bool
   LoweringHelper::handleNEG(Instruction *insn)
   {
      if (typeSizeof(insn->dType) != 8 || isFloatType(insn->dType))
                     insn->op = OP_SUB;
   insn->setSrc(1, insn->getSrc(0));
   insn->setSrc(0, bld.mkImm((uint64_t)0));
      }
      bool
   LoweringHelper::handleSAT(Instruction *insn)
   {
               if (typeSizeof(dTy) != 8 || !isFloatType(dTy))
                     Value *tmp = bld.mkOp2v(OP_MAX, dTy, bld.getSSA(8), insn->getSrc(0), bld.loadImm(bld.getSSA(8), 0.0));
   insn->op = OP_MIN;
   insn->setSrc(0, tmp);
   insn->setSrc(1, bld.loadImm(bld.getSSA(8), 1.0));
      }
      bool
   LoweringHelper::handleSLCT(CmpInstruction *insn)
   {
      DataType dTy = insn->dType;
            if (typeSizeof(dTy) != 8 || typeSizeof(sTy) == 8)
            CondCode cc = insn->getCondition();
   DataType hdTy = typeOfSize(4, isFloatType(dTy), isSignedIntType(dTy));
            Value *src0[2];
   Value *src1[2];
            bld.mkSplit(src0, 4, insn->getSrc(0));
            def[0] = bld.getSSA();
            bld.mkCmp(OP_SLCT, cc, hdTy, def[0], sTy, src0[0], src1[0], insn->getSrc(2));
            insn->op = OP_MERGE;
   insn->setSrc(0, def[0]);
   insn->setSrc(1, def[1]);
               }
      bool
   LoweringHelper::handleLogOp(Instruction *insn)
   {
      DataType dTy = insn->dType;
            if (typeSizeof(dTy) != 8)
                     Value *src0[2];
   Value *src1[2];
   Value *def0 = bld.getSSA();
            bld.mkSplit(src0, 4, insn->getSrc(0));
   if (insn->srcExists(1))
            Instruction *lo = bld.mkOp1(insn->op, sTy, def0, src0[0]);
   Instruction *hi = bld.mkOp1(insn->op, sTy, def1, src0[1]);
   if (insn->srcExists(1)) {
      lo->setSrc(1, src1[0]);
               insn->op = OP_MERGE;
   insn->setSrc(0, def0);
               }
      } // namespace nv50_ir
