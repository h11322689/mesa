   /*
   * Copyright 2011 Christoph Bumiller
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
   */
      #include "nv50_ir_target_nvc0.h"
      namespace nv50_ir {
      // Argh, all these assertions ...
      class CodeEmitterNVC0 : public CodeEmitter
   {
   public:
               virtual bool emitInstruction(Instruction *);
   virtual uint32_t getMinEncodingSize(const Instruction *) const;
         private:
                              private:
      void emitForm_A(const Instruction *, uint64_t);
   void emitForm_B(const Instruction *, uint64_t);
                     void setAddress16(const ValueRef&);
   void setAddress24(const ValueRef&);
   void setAddressByFile(const ValueRef&);
   void setImmediate(const Instruction *, const int s); // needs op already set
   void setImmediateS8(const ValueRef&);
   void setSUConst16(const Instruction *, const int s);
   void setSUPred(const Instruction *, const int s);
            void emitCondCode(CondCode cc, int pos);
   void emitInterpMode(const Instruction *);
   void emitLoadStoreType(DataType ty);
   void emitSUGType(DataType);
   void emitSUAddr(const TexInstruction *);
   void emitSUDim(const TexInstruction *);
                              void roundMode_A(const Instruction *);
   void roundMode_C(const Instruction *);
                              void emitLOAD(const Instruction *);
   void emitSTORE(const Instruction *);
   void emitMOV(const Instruction *);
   void emitATOM(const Instruction *);
   void emitMEMBAR(const Instruction *);
            void emitINTERP(const Instruction *);
   void emitAFETCH(const Instruction *);
   void emitPFETCH(const Instruction *);
   void emitVFETCH(const Instruction *);
   void emitEXPORT(const Instruction *);
            void emitUADD(const Instruction *);
   void emitFADD(const Instruction *);
   void emitDADD(const Instruction *);
   void emitUMUL(const Instruction *);
   void emitFMUL(const Instruction *);
   void emitDMUL(const Instruction *);
   void emitIMAD(const Instruction *);
   void emitISAD(const Instruction *);
   void emitSHLADD(const Instruction *a);
   void emitFMAD(const Instruction *);
   void emitDMAD(const Instruction *);
            void emitNOT(Instruction *);
   void emitLogicOp(const Instruction *, uint8_t subOp);
   void emitPOPC(const Instruction *);
   void emitINSBF(const Instruction *);
   void emitEXTBF(const Instruction *);
   void emitBFIND(const Instruction *);
   void emitPERMT(const Instruction *);
                     void emitCVT(Instruction *);
   void emitMINMAX(const Instruction *);
            void emitSET(const CmpInstruction *);
   void emitSLCT(const CmpInstruction *);
            void emitTEXBAR(const Instruction *);
   void emitTEX(const TexInstruction *);
   void emitTEXCSAA(const TexInstruction *);
                     void emitFlow(const Instruction *);
            void emitSUCLAMPMode(uint16_t);
   void emitSUCalc(Instruction *);
   void emitSULDGB(const TexInstruction *);
            void emitSULDB(const TexInstruction *);
   void emitSUSTx(const TexInstruction *);
            void emitVSHL(const Instruction *);
                                       inline void defId(const ValueDef&, const int pos);
   inline void defId(const Instruction *, int d, const int pos);
   inline void srcId(const ValueRef&, const int pos);
   inline void srcId(const ValueRef *, const int pos);
   inline void srcId(const Instruction *, int s, const int pos);
               };
      // for better visibility
   #define HEX64(h, l) 0x##h##l##ULL
      #define SDATA(a) ((a).rep()->reg.data)
   #define DDATA(a) ((a).rep()->reg.data)
      void CodeEmitterNVC0::srcId(const ValueRef& src, const int pos)
   {
         }
      void CodeEmitterNVC0::srcId(const ValueRef *src, const int pos)
   {
         }
      void CodeEmitterNVC0::srcId(const Instruction *insn, int s, int pos)
   {
      int r = insn->srcExists(s) ? SDATA(insn->src(s)).id : 63;
      }
      void
   CodeEmitterNVC0::srcAddr32(const ValueRef& src, int pos, int shr)
   {
               code[pos / 32] |= offset << (pos % 32);
   if (pos && (pos < 32))
      }
      void CodeEmitterNVC0::defId(const ValueDef& def, const int pos)
   {
         }
      void CodeEmitterNVC0::defId(const Instruction *insn, int d, const int pos)
   {
      if (insn->defExists(d))
         else
      }
      bool CodeEmitterNVC0::isLIMM(const ValueRef& ref, DataType ty)
   {
               if (ty == TYPE_F32)
         else
      return imm && (imm->reg.data.s32 > 0x7ffff ||
   }
      void
   CodeEmitterNVC0::roundMode_A(const Instruction *insn)
   {
      switch (insn->rnd) {
   case ROUND_M: code[1] |= 1 << 23; break;
   case ROUND_P: code[1] |= 2 << 23; break;
   case ROUND_Z: code[1] |= 3 << 23; break;
   default:
      assert(insn->rnd == ROUND_N);
         }
      void
   CodeEmitterNVC0::emitNegAbs12(const Instruction *i)
   {
      if (i->src(1).mod.abs()) code[0] |= 1 << 6;
   if (i->src(0).mod.abs()) code[0] |= 1 << 7;
   if (i->src(1).mod.neg()) code[0] |= 1 << 8;
      }
      void CodeEmitterNVC0::emitCondCode(CondCode cc, int pos)
   {
               switch (cc) {
   case CC_LT:  val = 0x1; break;
   case CC_LTU: val = 0x9; break;
   case CC_EQ:  val = 0x2; break;
   case CC_EQU: val = 0xa; break;
   case CC_LE:  val = 0x3; break;
   case CC_LEU: val = 0xb; break;
   case CC_GT:  val = 0x4; break;
   case CC_GTU: val = 0xc; break;
   case CC_NE:  val = 0x5; break;
   case CC_NEU: val = 0xd; break;
   case CC_GE:  val = 0x6; break;
   case CC_GEU: val = 0xe; break;
   case CC_TR:  val = 0xf; break;
            case CC_A:  val = 0x14; break;
   case CC_NA: val = 0x13; break;
   case CC_S:  val = 0x15; break;
   case CC_NS: val = 0x12; break;
   case CC_C:  val = 0x16; break;
   case CC_NC: val = 0x11; break;
   case CC_O:  val = 0x17; break;
            default:
      val = 0;
   assert(!"invalid condition code");
      }
      }
      void
   CodeEmitterNVC0::emitPredicate(const Instruction *i)
   {
      if (i->predSrc >= 0) {
      assert(i->getPredicate()->reg.file == FILE_PREDICATE);
   srcId(i->src(i->predSrc), 10);
   if (i->cc == CC_NOT_P)
      } else {
            }
      void
   CodeEmitterNVC0::setAddressByFile(const ValueRef& src)
   {
      switch (src.getFile()) {
   case FILE_MEMORY_GLOBAL:
      srcAddr32(src, 26, 0);
      case FILE_MEMORY_LOCAL:
   case FILE_MEMORY_SHARED:
      setAddress24(src);
      default:
      assert(src.getFile() == FILE_MEMORY_CONST);
   setAddress16(src);
         }
      void
   CodeEmitterNVC0::setAddress16(const ValueRef& src)
   {
                        code[0] |= (sym->reg.data.offset & 0x003f) << 26;
      }
      void
   CodeEmitterNVC0::setAddress24(const ValueRef& src)
   {
                        code[0] |= (sym->reg.data.offset & 0x00003f) << 26;
      }
      void
   CodeEmitterNVC0::setImmediate(const Instruction *i, const int s)
   {
      const ImmediateValue *imm = i->src(s).get()->asImm();
            assert(imm);
            if ((code[0] & 0xf) == 0x1) {
      // double immediate
   uint64_t u64 = imm->reg.data.u64;
   assert(!(u64 & 0x00000fffffffffffULL));
   assert(!(code[1] & 0xc000));
   code[0] |= ((u64 >> 44) & 0x3f) << 26;
      } else
   if ((code[0] & 0xf) == 0x2) {
      // LIMM
   code[0] |= (u32 & 0x3f) << 26;
      } else
   if ((code[0] & 0xf) == 0x3 || (code[0] & 0xf) == 4) {
      // integer immediate
   assert((u32 & 0xfff80000) == 0 || (u32 & 0xfff80000) == 0xfff80000);
   assert(!(code[1] & 0xc000));
   u32 &= 0xfffff;
   code[0] |= (u32 & 0x3f) << 26;
      } else {
      // float immediate
   assert(!(u32 & 0x00000fff));
   assert(!(code[1] & 0xc000));
   code[0] |= ((u32 >> 12) & 0x3f) << 26;
         }
      void CodeEmitterNVC0::setImmediateS8(const ValueRef &ref)
   {
                                 code[0] |= (s8 & 0x3f) << 26;
      }
      void CodeEmitterNVC0::setPDSTL(const Instruction *i, const int d)
   {
                        code[0] |= (pred & 3) << 8;
      }
      void
   CodeEmitterNVC0::emitForm_A(const Instruction *i, uint64_t opc)
   {
      code[0] = opc;
                              int s1 = 26;
   if (i->srcExists(2) && i->getSrc(2)->reg.file == FILE_MEMORY_CONST)
            for (int s = 0; s < 3 && i->srcExists(s); ++s) {
      switch (i->getSrc(s)->reg.file) {
   case FILE_MEMORY_CONST:
      assert(!(code[1] & 0xc000));
   code[1] |= (s == 2) ? 0x8000 : 0x4000;
   code[1] |= i->getSrc(s)->reg.fileIndex << 10;
   setAddress16(i->src(s));
      case FILE_IMMEDIATE:
      assert(s == 1 ||
         assert(!(code[1] & 0xc000));
   setImmediate(i, s);
      case FILE_GPR:
      if ((s == 2) && ((code[0] & 0x7) == 2)) // LIMM: 3rd src == dst
         srcId(i->src(s), s ? ((s == 2) ? 49 : s1) : 20);
      default:
      if (i->op == OP_SELP) {
      // OP_SELP is used to implement shared+atomics on Fermi.
   assert(s == 2 && i->src(s).getFile() == FILE_PREDICATE);
      }
   // ignore here, can be predicate or flags, but must not be address
            }
      void
   CodeEmitterNVC0::emitForm_B(const Instruction *i, uint64_t opc)
   {
      code[0] = opc;
                              switch (i->src(0).getFile()) {
   case FILE_MEMORY_CONST:
      assert(!(code[1] & 0xc000));
   code[1] |= 0x4000 | (i->src(0).get()->reg.fileIndex << 10);
   setAddress16(i->src(0));
      case FILE_IMMEDIATE:
      assert(!(code[1] & 0xc000));
   setImmediate(i, 0);
      case FILE_GPR:
      srcId(i->src(0), 26);
      default:
      // ignore here, can be predicate or flags, but must not be address
         }
      void
   CodeEmitterNVC0::emitForm_S(const Instruction *i, uint32_t opc, bool pred)
   {
               int ss2a = 0;
   if (opc == 0x0d || opc == 0x0e)
            defId(i->def(0), 14);
            assert(pred || (i->predSrc < 0));
   if (pred)
            for (int s = 1; s < 3 && i->srcExists(s); ++s) {
      if (i->src(s).get()->reg.file == FILE_MEMORY_CONST) {
      assert(!(code[0] & (0x300 >> ss2a)));
   switch (i->src(s).get()->reg.fileIndex) {
   case 0:  code[0] |= 0x100 >> ss2a; break;
   case 1:  code[0] |= 0x200 >> ss2a; break;
   case 16: code[0] |= 0x300 >> ss2a; break;
   default:
      ERROR("invalid c[] space for short form\n");
      }
   if (s == 1)
         else
      } else
   if (i->src(s).getFile() == FILE_IMMEDIATE) {
      assert(s == 1);
      } else
   if (i->src(s).getFile() == FILE_GPR) {
               }
      void
   CodeEmitterNVC0::emitShortSrc2(const ValueRef &src)
   {
      if (src.getFile() == FILE_MEMORY_CONST) {
      switch (src.get()->reg.fileIndex) {
   case 0:  code[0] |= 0x100; break;
   case 1:  code[0] |= 0x200; break;
   case 16: code[0] |= 0x300; break;
   default:
      assert(!"unsupported file index for short op");
      }
      } else {
      srcId(src, 20);
         }
      void
   CodeEmitterNVC0::emitNOP(const Instruction *i)
   {
      code[0] = 0x000001e4;
   code[1] = 0x40000000;
      }
      void
   CodeEmitterNVC0::emitFMAD(const Instruction *i)
   {
               if (i->encSize == 8) {
      if (isLIMM(i->src(1), TYPE_F32)) {
         } else {
               if (i->src(2).mod.neg())
      }
            if (neg1)
            if (i->saturate)
            if (i->dnz)
         else
   if (i->ftz)
      } else {
      assert(!i->saturate && !i->src(2).mod.neg());
   emitForm_S(i, (i->src(2).getFile() == FILE_MEMORY_CONST) ? 0x2e : 0x0e,
         if (neg1)
         }
      void
   CodeEmitterNVC0::emitDMAD(const Instruction *i)
   {
                        if (i->src(2).mod.neg())
                     if (neg1)
            assert(!i->saturate);
      }
      void
   CodeEmitterNVC0::emitFMUL(const Instruction *i)
   {
                        if (i->encSize == 8) {
      if (isLIMM(i->src(1), TYPE_F32)) {
      assert(i->postFactor == 0); // constant folded, hopefully
      } else {
      emitForm_A(i, HEX64(58000000, 00000000));
   roundMode_A(i);
   code[1] |= ((i->postFactor > 0) ?
      }
   if (neg)
            if (i->saturate)
            if (i->dnz)
         else
   if (i->ftz)
      } else {
      assert(!neg && !i->saturate && !i->ftz && !i->postFactor);
         }
      void
   CodeEmitterNVC0::emitDMUL(const Instruction *i)
   {
               emitForm_A(i, HEX64(50000000, 00000001));
            if (neg)
            assert(!i->saturate);
   assert(!i->ftz);
   assert(!i->dnz);
      }
      void
   CodeEmitterNVC0::emitUMUL(const Instruction *i)
   {
      if (i->encSize == 8) {
      if (isLIMM(i->src(1), TYPE_U32)) {
         } else {
         }
   if (i->subOp == NV50_IR_SUBOP_MUL_HIGH)
         if (i->sType == TYPE_S32)
         if (i->dType == TYPE_S32)
      } else {
               if (i->sType == TYPE_S32)
         }
      void
   CodeEmitterNVC0::emitFADD(const Instruction *i)
   {
      if (i->encSize == 8) {
      if (isLIMM(i->src(1), TYPE_F32)) {
                                    if (i->src(1).mod.abs())
         if ((i->op == OP_SUB) != static_cast<bool>(i->src(1).mod.neg()))
      } else {
               roundMode_A(i);
                  emitNegAbs12(i);
      }
   if (i->ftz)
      } else {
      assert(!i->saturate && i->op != OP_SUB &&
                           if (i->src(0).mod.neg())
         }
      void
   CodeEmitterNVC0::emitDADD(const Instruction *i)
   {
      assert(i->encSize == 8);
   emitForm_A(i, HEX64(48000000, 00000001));
   roundMode_A(i);
   assert(!i->saturate);
   assert(!i->ftz);
   emitNegAbs12(i);
   if (i->op == OP_SUB)
      }
      void
   CodeEmitterNVC0::emitUADD(const Instruction *i)
   {
                        if (i->src(0).mod.neg())
         if (i->src(1).mod.neg())
         if (i->op == OP_SUB)
                     if (i->encSize == 8) {
      if (isLIMM(i->src(1), TYPE_U32)) {
      emitForm_A(i, HEX64(08000000, 00000002));
   if (i->flagsDef >= 0)
      } else {
      emitForm_A(i, HEX64(48000000, 00000003));
   if (i->flagsDef >= 0)
      }
            if (i->saturate)
         if (i->flagsSrc >= 0) // add carry
      } else {
      assert(!(addOp & 0x100));
   emitForm_S(i, (addOp >> 3) |
         }
      void
   CodeEmitterNVC0::emitIMAD(const Instruction *i)
   {
      uint8_t addOp =
            assert(i->encSize == 8);
            assert(addOp != 3);
            if (isSignedType(i->dType))
         if (isSignedType(i->sType))
                     if (i->flagsDef >= 0) code[1] |= 1 << 16;
            if (i->subOp == NV50_IR_SUBOP_MUL_HIGH)
      }
      void
   CodeEmitterNVC0::emitSHLADD(const Instruction *i)
   {
      uint8_t addOp = (i->src(0).mod.neg() << 1) | i->src(2).mod.neg();
   const ImmediateValue *imm = i->src(1).get()->asImm();
            code[0] = 0x00000003;
                     defId(i->def(0), 14);
            if (i->flagsDef >= 0)
            assert(!(imm->reg.data.u32 & 0xffffffe0));
            switch (i->src(2).getFile()) {
   case FILE_GPR:
      srcId(i->src(2), 26);
      case FILE_MEMORY_CONST:
      code[1] |= 0x4000;
   code[1] |= i->getSrc(2)->reg.fileIndex << 10;
   setAddress16(i->src(2));
      case FILE_IMMEDIATE:
      setImmediate(i, 2);
      default:
      assert(!"bad src2 file");
         }
      void
   CodeEmitterNVC0::emitMADSP(const Instruction *i)
   {
                        if (i->subOp == NV50_IR_SUBOP_MADSP_SD) {
         } else {
      code[0] |= (i->subOp & 0x00f) << 7;
   code[0] |= (i->subOp & 0x0f0) << 1;
   code[0] |= (i->subOp & 0x100) >> 3;
   code[0] |= (i->subOp & 0x200) >> 2;
               if (i->flagsDef >= 0)
      }
      void
   CodeEmitterNVC0::emitISAD(const Instruction *i)
   {
      assert(i->dType == TYPE_S32 || i->dType == TYPE_U32);
                     if (i->dType == TYPE_S32)
      }
      void
   CodeEmitterNVC0::emitNOT(Instruction *i)
   {
      assert(i->encSize == 8);
   if (i->getPredicate())
         i->setSrc(1, i->src(0));
      }
      void
   CodeEmitterNVC0::emitLogicOp(const Instruction *i, uint8_t subOp)
   {
      if (i->def(0).getFile() == FILE_PREDICATE) {
      code[0] = 0x00000004 | (subOp << 30);
                     defId(i->def(0), 17);
   srcId(i->src(0), 20);
   if (i->src(0).mod == Modifier(NV50_IR_MOD_NOT)) code[0] |= 1 << 23;
   srcId(i->src(1), 26);
            if (i->defExists(1)) {
         } else {
         }
   // (a OP b) OP c
   if (i->predSrc != 2 && i->srcExists(2)) {
      code[1] |= subOp << 21;
   srcId(i->src(2), 49);
      } else {
            } else
   if (i->encSize == 8) {
      if (isLIMM(i->src(1), TYPE_U32)) {
               if (i->flagsDef >= 0)
      } else {
               if (i->flagsDef >= 0)
      }
            if (i->flagsSrc >= 0) // carry
            if (i->src(0).mod & Modifier(NV50_IR_MOD_NOT)) code[0] |= 1 << 9;
      } else {
      emitForm_S(i, (subOp << 5) |
         }
      void
   CodeEmitterNVC0::emitPOPC(const Instruction *i)
   {
               if (i->src(0).mod & Modifier(NV50_IR_MOD_NOT)) code[0] |= 1 << 9;
      }
      void
   CodeEmitterNVC0::emitINSBF(const Instruction *i)
   {
         }
      void
   CodeEmitterNVC0::emitEXTBF(const Instruction *i)
   {
               if (i->dType == TYPE_S32)
         if (i->subOp == NV50_IR_SUBOP_EXTBF_REV)
      }
      void
   CodeEmitterNVC0::emitBFIND(const Instruction *i)
   {
               if (i->dType == TYPE_S32)
         if (i->src(0).mod == Modifier(NV50_IR_MOD_NOT))
         if (i->subOp == NV50_IR_SUBOP_BFIND_SAMT)
      }
      void
   CodeEmitterNVC0::emitPERMT(const Instruction *i)
   {
                  }
      void
   CodeEmitterNVC0::emitShift(const Instruction *i)
   {
      if (i->op == OP_SHR) {
      emitForm_A(i, HEX64(58000000, 00000003)
      } else {
                  if (i->subOp == NV50_IR_SUBOP_SHIFT_WRAP)
      }
      void
   CodeEmitterNVC0::emitPreOp(const Instruction *i)
   {
      if (i->encSize == 8) {
               if (i->op == OP_PREEX2)
            if (i->src(0).mod.abs()) code[0] |= 1 << 6;
      } else {
            }
      void
   CodeEmitterNVC0::emitSFnOp(const Instruction *i, uint8_t subOp)
   {
      if (i->encSize == 8) {
      code[0] = 0x00000000 | (subOp << 26);
                     defId(i->def(0), 14);
                              if (i->src(0).mod.abs()) code[0] |= 1 << 7;
      } else {
               assert(!i->src(0).mod.neg());
         }
      void
   CodeEmitterNVC0::emitMINMAX(const Instruction *i)
   {
                                 if (i->ftz)
         else
   if (!isFloatType(i->dType)) {
      op |= isSignedType(i->dType) ? 0x23 : 0x03;
      }
   if (i->dType == TYPE_F64)
            emitForm_A(i, op);
            if (i->flagsDef >= 0)
      }
      void
   CodeEmitterNVC0::roundMode_C(const Instruction *i)
   {
      switch (i->rnd) {
   case ROUND_M:  code[1] |= 1 << 17; break;
   case ROUND_P:  code[1] |= 2 << 17; break;
   case ROUND_Z:  code[1] |= 3 << 17; break;
   case ROUND_NI: code[0] |= 1 << 7; break;
   case ROUND_MI: code[0] |= 1 << 7; code[1] |= 1 << 17; break;
   case ROUND_PI: code[0] |= 1 << 7; code[1] |= 2 << 17; break;
   case ROUND_ZI: code[0] |= 1 << 7; code[1] |= 3 << 17; break;
   case ROUND_N: break;
   default:
      assert(!"invalid round mode");
         }
      void
   CodeEmitterNVC0::roundMode_CS(const Instruction *i)
   {
      switch (i->rnd) {
   case ROUND_M:
   case ROUND_MI: code[0] |= 1 << 16; break;
   case ROUND_P:
   case ROUND_PI: code[0] |= 2 << 16; break;
   case ROUND_Z:
   case ROUND_ZI: code[0] |= 3 << 16; break;
   default:
            }
      void
   CodeEmitterNVC0::emitCVT(Instruction *i)
   {
      const bool f2f = isFloatType(i->dType) && isFloatType(i->sType);
            switch (i->op) {
   case OP_CEIL:  i->rnd = f2f ? ROUND_PI : ROUND_P; break;
   case OP_FLOOR: i->rnd = f2f ? ROUND_MI : ROUND_M; break;
   case OP_TRUNC: i->rnd = f2f ? ROUND_ZI : ROUND_Z; break;
   default:
                  const bool sat = (i->op == OP_SAT) || i->saturate;
   const bool abs = (i->op == OP_ABS) || i->src(0).mod.abs();
            if (i->op == OP_NEG && i->dType == TYPE_U32)
         else
            if (i->encSize == 8) {
                        // cvt u16 f32 sets high bits to 0, so we don't have to use Value::Size()
   code[0] |= util_logbase2(typeSizeof(dType)) << 20;
            // for 8/16 source types, the byte/word is in subOp. word 1 is
   // represented as 2.
   if (!isFloatType(i->sType))
         else
            if (sat)
         if (abs)
         if (neg && i->op != OP_ABS)
            if (i->ftz)
            if (isSignedIntType(dType))
         if (isSignedIntType(i->sType))
            if (isFloatType(dType)) {
      if (!isFloatType(i->sType))
      } else {
      if (isFloatType(i->sType))
         else
         } else {
      if (i->op == OP_CEIL || i->op == OP_FLOOR || i->op == OP_TRUNC) {
         } else
   if (isFloatType(dType)) {
      if (isFloatType(i->sType))
         else
      } else {
                           if (neg) code[0] |= 1 << 16;
   if (sat) code[0] |= 1 << 18;
                  }
      void
   CodeEmitterNVC0::emitSET(const CmpInstruction *i)
   {
      uint32_t hi;
            if (i->sType == TYPE_F64)
         else
   if (!isFloatType(i->sType))
            if (isSignedIntType(i->sType))
         if (isFloatType(i->dType)) {
      if (isFloatType(i->sType))
         else
               switch (i->op) {
   case OP_SET_AND: hi = 0x10000000; break;
   case OP_SET_OR:  hi = 0x10200000; break;
   case OP_SET_XOR: hi = 0x10400000; break;
   default:
      hi = 0x100e0000;
      }
            if (i->op != OP_SET)
            if (i->def(0).getFile() == FILE_PREDICATE) {
      if (i->sType == TYPE_F32)
         else
            code[0] &= ~0xfc000;
   defId(i->def(0), 17);
   if (i->defExists(1))
         else
               if (i->ftz)
         if (i->flagsSrc >= 0)
            emitCondCode(i->setCond, 32 + 23);
      }
      void
   CodeEmitterNVC0::emitSLCT(const CmpInstruction *i)
   {
               switch (i->dType) {
   case TYPE_S32:
      op = HEX64(30000000, 00000023);
      case TYPE_U32:
      op = HEX64(30000000, 00000003);
      case TYPE_F32:
      op = HEX64(38000000, 00000000);
      default:
      assert(!"invalid type for SLCT");
   op = 0;
      }
                     if (i->src(2).mod.neg())
                     if (i->ftz)
      }
      void
   nvc0_selpFlip(const FixupEntry *entry, uint32_t *code, const FixupData& data)
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
      void CodeEmitterNVC0::emitSELP(const Instruction *i)
   {
               if (i->src(2).mod & Modifier(NV50_IR_MOD_NOT))
            if (i->subOp >= 1) {
            }
      void CodeEmitterNVC0::emitTEXBAR(const Instruction *i)
   {
      code[0] = 0x00000006 | (i->subOp << 26);
   code[1] = 0xf0000000;
   emitPredicate(i);
      }
      void CodeEmitterNVC0::emitTEXCSAA(const TexInstruction *i)
   {
      code[0] = 0x00000086;
            code[1] |= i->tex.r;
            if (i->tex.liveOnly)
            defId(i->def(0), 14);
      }
      static inline bool
   isNextIndependentTex(const TexInstruction *i)
   {
      if (!i->next || !isTextureOp(i->next->op))
         if (i->getDef(0)->interfers(i->next->getSrc(0)))
            }
      void
   CodeEmitterNVC0::emitTEX(const TexInstruction *i)
   {
               if (isNextIndependentTex(i))
         else
            if (i->tex.liveOnly)
            switch (i->op) {
   case OP_TEX: code[1] = 0x80000000; break;
   case OP_TXB: code[1] = 0x84000000; break;
   case OP_TXL: code[1] = 0x86000000; break;
   case OP_TXF: code[1] = 0x90000000; break;
   case OP_TXG: code[1] = 0xa0000000; break;
   case OP_TXLQ: code[1] = 0xb0000000; break;
   case OP_TXD: code[1] = 0xe0000000; break;
   default:
      assert(!"invalid texture op");
      }
   if (i->op == OP_TXF) {
      if (!i->tex.levelZero)
      } else
   if (i->tex.levelZero) {
                  if (i->op != OP_TXD && i->tex.derivAll)
            defId(i->def(0), 14);
                                       code[1] |= i->tex.r;
   code[1] |= i->tex.s << 8;
   if (i->tex.rIndirectSrc >= 0 || i->tex.sIndirectSrc >= 0)
            // texture target:
   code[1] |= (i->tex.target.getDim() - 1) << 20;
   if (i->tex.target.isCube())
         if (i->tex.target.isArray())
         if (i->tex.target.isShadow())
                     if (i->srcExists(src1) && i->src(src1).getFile() == FILE_IMMEDIATE) {
      // lzero
   if (i->op == OP_TXL)
         else
   if (i->op == OP_TXF)
      }
   if (i->tex.target == TEX_TARGET_2D_MS ||
      i->tex.target == TEX_TARGET_2D_MS_ARRAY)
         if (i->tex.useOffsets == 1)
         if (i->tex.useOffsets == 4)
               }
      void
   CodeEmitterNVC0::emitTXQ(const TexInstruction *i)
   {
      code[0] = 0x00000086;
            switch (i->tex.query) {
   case TXQ_DIMS:            code[1] |= 0 << 22; break;
   case TXQ_TYPE:            code[1] |= 1 << 22; break;
   case TXQ_SAMPLE_POSITION: code[1] |= 2 << 22; break;
   case TXQ_FILTER:          code[1] |= 3 << 22; break;
   case TXQ_LOD:             code[1] |= 4 << 22; break;
   case TXQ_BORDER_COLOUR:   code[1] |= 5 << 22; break;
   default:
      assert(!"invalid texture query");
                        code[1] |= i->tex.r;
   code[1] |= i->tex.s << 8;
   if (i->tex.sIndirectSrc >= 0 || i->tex.rIndirectSrc >= 0)
                     defId(i->def(0), 14);
   srcId(i->src(0), 20);
               }
      void
   CodeEmitterNVC0::emitQUADOP(const Instruction *i, uint8_t qOp, uint8_t laneMask)
   {
      code[0] = 0x00000200 | (laneMask << 6); // dall
            defId(i->def(0), 14);
   srcId(i->src(0), 20);
               }
      void
   CodeEmitterNVC0::emitFlow(const Instruction *i)
   {
                                 switch (i->op) {
   case OP_BRA:
      code[1] = f->absolute ? 0x00000000 : 0x40000000;
   if (i->srcExists(0) && i->src(0).getFile() == FILE_MEMORY_CONST)
         mask = 3;
      case OP_CALL:
      code[1] = f->absolute ? 0x10000000 : 0x50000000;
   if (f->indirect)
         mask = 2;
         case OP_EXIT:    code[1] = 0x80000000; mask = 1; break;
   case OP_RET:     code[1] = 0x90000000; mask = 1; break;
   case OP_DISCARD: code[1] = 0x98000000; mask = 1; break;
   case OP_BREAK:   code[1] = 0xa8000000; mask = 1; break;
            case OP_JOINAT:   code[1] = 0x60000000; mask = 2; break;
   case OP_PREBREAK: code[1] = 0x68000000; mask = 2; break;
   case OP_PRECONT:  code[1] = 0x70000000; mask = 2; break;
            case OP_QUADON:  code[1] = 0xc0000000; mask = 0; break;
   case OP_QUADPOP: code[1] = 0xc8000000; mask = 0; break;
   case OP_BRKPT:   code[1] = 0xd0000000; mask = 0; break;
   default:
      assert(!"invalid flow operation");
               if (mask & 1) {
      emitPredicate(i);
   if (i->flagsSrc < 0)
               if (!f)
            if (f->allWarp)
         if (f->limit)
            if (f->indirect) {
      if (code[0] & 0x4000) {
      assert(i->srcExists(0) && i->src(0).getFile() == FILE_MEMORY_CONST);
   setAddress16(i->src(0));
   code[1] |= i->getSrc(0)->reg.fileIndex << 10;
   if (f->op == OP_BRA)
      } else {
                     if (f->op == OP_CALL) {
      if (f->indirect) {
         } else
   if (f->builtin) {
      assert(f->absolute);
   uint32_t pcAbs = targNVC0->getBuiltinOffset(f->target.builtin);
   addReloc(RelocEntry::TYPE_BUILTIN, 0, pcAbs, 0xfc000000, 26);
      } else {
      assert(!f->absolute);
   int32_t pcRel = f->target.fn->binPos - (codeSize + 8);
   code[0] |= (pcRel & 0x3f) << 26;
         } else
   if (mask & 2) {
      int32_t pcRel = f->target.bb->binPos - (codeSize + 8);
   if (writeIssueDelays && !(f->target.bb->binPos & 0x3f))
         // currently we don't want absolute branches
   assert(!f->absolute);
   code[0] |= (pcRel & 0x3f) << 26;
         }
      void
   CodeEmitterNVC0::emitBAR(const Instruction *i)
   {
               switch (i->subOp) {
   case NV50_IR_SUBOP_BAR_ARRIVE:   code[0] = 0x84; break;
   case NV50_IR_SUBOP_BAR_RED_AND:  code[0] = 0x24; break;
   case NV50_IR_SUBOP_BAR_RED_OR:   code[0] = 0x44; break;
   case NV50_IR_SUBOP_BAR_RED_POPC: code[0] = 0x04; break;
   default:
      code[0] = 0x04;
   assert(i->subOp == NV50_IR_SUBOP_BAR_SYNC);
      }
            code[0] |= 63 << 14;
                     // barrier id
   if (i->src(0).getFile() == FILE_GPR) {
         } else {
      ImmediateValue *imm = i->getSrc(0)->asImm();
   assert(imm);
   code[0] |= imm->reg.data.u32 << 20;
               // thread count
   if (i->src(1).getFile() == FILE_GPR) {
         } else {
      ImmediateValue *imm = i->getSrc(1)->asImm();
   assert(imm);
   assert(imm->reg.data.u32 <= 0xfff);
   code[0] |= imm->reg.data.u32 << 26;
   code[1] |= imm->reg.data.u32 >> 6;
               if (i->srcExists(2) && (i->predSrc != 2)) {
      srcId(i->src(2), 32 + 17);
   if (i->src(2).mod == Modifier(NV50_IR_MOD_NOT))
      } else {
                  if (i->defExists(0)) {
      if (i->def(0).getFile() == FILE_GPR)
         else
            if (i->defExists(1)) {
      if (i->def(1).getFile() == FILE_GPR)
         else
         }
   if (rDef) {
      code[0] &= ~(63 << 14);
      }
   if (pDef) {
      code[1] &= ~(7 << 21);
         }
      void
   CodeEmitterNVC0::emitAFETCH(const Instruction *i)
   {
      code[0] = 0x00000006;
            if (i->getSrc(0)->reg.file == FILE_SHADER_OUTPUT)
                     defId(i->def(0), 14);
      }
      void
   CodeEmitterNVC0::emitPFETCH(const Instruction *i)
   {
               code[0] = 0x00000006 | ((prim & 0x3f) << 26);
                              defId(i->def(0), 14);
      }
      void
   CodeEmitterNVC0::emitVFETCH(const Instruction *i)
   {
      code[0] = 0x00000006;
            if (i->perPatch)
         if (i->getSrc(0)->reg.file == FILE_SHADER_OUTPUT)
                              defId(i->def(0), 14);
   srcId(i->src(0).getIndirect(0), 20);
      }
      void
   CodeEmitterNVC0::emitEXPORT(const Instruction *i)
   {
               code[0] = 0x00000006 | ((size / 4 - 1) << 5);
                     if (i->perPatch)
                              srcId(i->src(0).getIndirect(0), 20);
   srcId(i->src(0).getIndirect(1), 32 + 17); // vertex base address
      }
      void
   CodeEmitterNVC0::emitOUT(const Instruction *i)
   {
      code[0] = 0x00000006;
                     defId(i->def(0), 14); // new secret address
                     if (i->op == OP_EMIT)
         if (i->op == OP_RESTART || i->subOp == NV50_IR_SUBOP_EMIT_RESTART)
            // vertex stream
   if (i->src(1).getFile() == FILE_IMMEDIATE) {
      unsigned int stream = SDATA(i->src(1)).u32;
   assert(stream < 4);
   if (stream) {
      code[1] |= 0xc000;
      } else {
            } else {
            }
      void
   CodeEmitterNVC0::emitInterpMode(const Instruction *i)
   {
      if (i->encSize == 8) {
         } else {
      if (i->getInterpMode() == NV50_IR_INTERP_SC)
               }
      void
   nvc0_interpApply(const FixupEntry *entry, uint32_t *code, const FixupData& data)
   {
      int ipa = entry->ipa;
   int reg = entry->reg;
            if (data.flatshade &&
      (ipa & NV50_IR_INTERP_MODE_MASK) == NV50_IR_INTERP_SC) {
   ipa = NV50_IR_INTERP_FLAT;
      } else if (data.force_persample_interp &&
            (ipa & NV50_IR_INTERP_SAMPLE_MASK) == NV50_IR_INTERP_DEFAULT &&
      }
   code[loc + 0] &= ~(0xf << 6);
   code[loc + 0] |= ipa << 6;
   code[loc + 0] &= ~(0x3f << 26);
      }
      void
   CodeEmitterNVC0::emitINTERP(const Instruction *i)
   {
               if (i->encSize == 8) {
      code[0] = 0x00000000;
            if (i->saturate)
            if (i->op == OP_PINTERP) {
      srcId(i->src(1), 26);
      } else {
      code[0] |= 0x3f << 26;
                  } else {
      assert(i->op == OP_PINTERP);
   code[0] = 0x00000009 | ((base & 0xc) << 6) | ((base >> 4) << 26);
      }
            emitPredicate(i);
            if (i->getSampleMode() == NV50_IR_INTERP_OFFSET)
         else
      }
      void
   CodeEmitterNVC0::emitLoadStoreType(DataType ty)
   {
               switch (ty) {
   case TYPE_U8:
      val = 0x00;
      case TYPE_S8:
      val = 0x20;
      case TYPE_F16:
   case TYPE_U16:
      val = 0x40;
      case TYPE_S16:
      val = 0x60;
      case TYPE_F32:
   case TYPE_U32:
   case TYPE_S32:
      val = 0x80;
      case TYPE_F64:
   case TYPE_U64:
   case TYPE_S64:
      val = 0xa0;
      case TYPE_B128:
      val = 0xc0;
      default:
      val = 0x80;
   assert(!"invalid type");
      }
      }
      void
   CodeEmitterNVC0::emitCachingMode(CacheMode c)
   {
               switch (c) {
      // case CACHE_WB:
         val = 0x000;
      case CACHE_CG:
      val = 0x100;
      case CACHE_CS:
      val = 0x200;
         // case CACHE_WT:
         val = 0x300;
      default:
      val = 0;
   assert(!"invalid caching mode");
      }
      }
      static inline bool
   uses64bitAddress(const Instruction *ldst)
   {
      return ldst->src(0).getFile() == FILE_MEMORY_GLOBAL &&
      ldst->src(0).isIndirect(0) &&
   }
      void
   CodeEmitterNVC0::emitSTORE(const Instruction *i)
   {
               switch (i->src(0).getFile()) {
   case FILE_MEMORY_GLOBAL: opc = 0x90000000; break;
   case FILE_MEMORY_LOCAL:  opc = 0xc8000000; break;
   case FILE_MEMORY_SHARED:
      if (i->subOp == NV50_IR_SUBOP_STORE_UNLOCKED) {
      if (targ->getChipset() >= NVISA_GK104_CHIPSET)
         else
      } else {
         }
      default:
      assert(!"invalid memory file");
   opc = 0;
      }
   code[0] = 0x00000005;
            if (targ->getChipset() >= NVISA_GK104_CHIPSET) {
      // Unlocked store on shared memory can fail.
   if (i->src(0).getFile() == FILE_MEMORY_SHARED &&
      i->subOp == NV50_IR_SUBOP_STORE_UNLOCKED) {
   assert(i->defExists(0));
                  setAddressByFile(i->src(0));
   srcId(i->src(1), 14);
   srcId(i->src(0).getIndirect(0), 20);
   if (uses64bitAddress(i))
                     emitLoadStoreType(i->dType);
      }
      void
   CodeEmitterNVC0::emitLOAD(const Instruction *i)
   {
                        switch (i->src(0).getFile()) {
   case FILE_MEMORY_GLOBAL: opc = 0x80000000; break;
   case FILE_MEMORY_LOCAL:  opc = 0xc0000000; break;
   case FILE_MEMORY_SHARED:
      if (i->subOp == NV50_IR_SUBOP_LOAD_LOCKED) {
      if (targ->getChipset() >= NVISA_GK104_CHIPSET)
         else
      } else {
         }
      case FILE_MEMORY_CONST:
      if (!i->src(0).isIndirect(0) && typeSizeof(i->dType) == 4) {
      emitMOV(i); // not sure if this is any better
      }
   opc = 0x14000000 | (i->src(0).get()->reg.fileIndex << 10);
   code[0] = 0x00000006 | (i->subOp << 8);
      default:
      assert(!"invalid memory file");
   opc = 0;
      }
            int r = 0, p = -1;
   if (i->src(0).getFile() == FILE_MEMORY_SHARED) {
      if (i->subOp == NV50_IR_SUBOP_LOAD_LOCKED) {
      if (i->def(0).getFile() == FILE_PREDICATE) { // p, #
      r = -1;
      } else if (i->defExists(1)) { // r, p
         } else {
                        if (r >= 0)
         else
            if (p >= 0) {
      if (targ->getChipset() >= NVISA_GK104_CHIPSET)
         else
               setAddressByFile(i->src(0));
   srcId(i->src(0).getIndirect(0), 20);
   if (uses64bitAddress(i))
                     emitLoadStoreType(i->dType);
      }
      uint8_t
   CodeEmitterNVC0::getSRegEncoding(const ValueRef& ref)
   {
      switch (SDATA(ref).sv.sv) {
   case SV_LANEID:        return 0x00;
   case SV_PHYSID:        return 0x03;
   case SV_VERTEX_COUNT:  return 0x10;
   case SV_INVOCATION_ID: return 0x11;
   case SV_YDIR:          return 0x12;
   case SV_THREAD_KILL:   return 0x13;
   case SV_COMBINED_TID:  return 0x20;
   case SV_TID:           return 0x21 + SDATA(ref).sv.index;
   case SV_CTAID:         return 0x25 + SDATA(ref).sv.index;
   case SV_NTID:          return 0x29 + SDATA(ref).sv.index;
   case SV_GRIDID:        return 0x2c;
   case SV_NCTAID:        return 0x2d + SDATA(ref).sv.index;
   case SV_LBASE:         return 0x34;
   case SV_SBASE:         return 0x30;
   case SV_LANEMASK_EQ:   return 0x38;
   case SV_LANEMASK_LT:   return 0x39;
   case SV_LANEMASK_LE:   return 0x3a;
   case SV_LANEMASK_GT:   return 0x3b;
   case SV_LANEMASK_GE:   return 0x3c;
   case SV_CLOCK:         return 0x50 + SDATA(ref).sv.index;
   default:
      assert(!"no sreg for system value");
         }
      void
   CodeEmitterNVC0::emitMOV(const Instruction *i)
   {
      assert(!i->saturate);
   if (i->def(0).getFile() == FILE_PREDICATE) {
      if (i->src(0).getFile() == FILE_GPR) {
      code[0] = 0xfc01c003;
   code[1] = 0x1a8e0000;
      } else {
      code[0] = 0x0001c004;
   code[1] = 0x0c0e0000;
   if (i->src(0).getFile() == FILE_IMMEDIATE) {
      code[0] |= 7 << 20;
   if (!i->getSrc(0)->reg.data.u32)
      } else {
            }
   defId(i->def(0), 17);
      } else
   if (i->src(0).getFile() == FILE_SYSTEM_VALUE) {
               if (i->encSize == 8) {
      code[0] = 0x00000004 | (sr << 26);
      } else {
         }
               } else
   if (i->encSize == 8) {
               if (i->src(0).getFile() == FILE_IMMEDIATE)
         else
   if (i->src(0).getFile() == FILE_PREDICATE)
         else
            if (i->src(0).getFile() != FILE_PREDICATE)
                     // Explicitly emit the predicate source as emitForm_B skips it.
   if (i->src(0).getFile() == FILE_PREDICATE)
      } else {
               if (i->src(0).getFile() == FILE_IMMEDIATE) {
      imm = SDATA(i->src(0)).u32;
   if (imm & 0xfff00000) {
      assert(!(imm & 0x000fffff));
      } else {
      assert(imm < 0x800 && ((int32_t)imm >= -0x800));
         } else {
      code[0] = 0x0028;
      }
                  }
      void
   CodeEmitterNVC0::emitATOM(const Instruction *i)
   {
      const bool hasDst = i->defExists(0);
   const bool casOrExch =
      i->subOp == NV50_IR_SUBOP_ATOM_EXCH ||
         if (i->dType == TYPE_U64) {
      switch (i->subOp) {
   case NV50_IR_SUBOP_ATOM_ADD:
      code[0] = 0x205;
   if (hasDst)
         else
            case NV50_IR_SUBOP_ATOM_EXCH:
      code[0] = 0x305;
   code[1] = 0x507e0000;
      case NV50_IR_SUBOP_ATOM_CAS:
      code[0] = 0x325;
   code[1] = 0x50000000;
      default:
      assert(!"invalid u64 red op");
         } else
   if (i->dType == TYPE_U32) {
      switch (i->subOp) {
   case NV50_IR_SUBOP_ATOM_EXCH:
      code[0] = 0x105;
   code[1] = 0x507e0000;
      case NV50_IR_SUBOP_ATOM_CAS:
      code[0] = 0x125;
   code[1] = 0x50000000;
      default:
      code[0] = 0x5 | (i->subOp << 5);
   if (hasDst)
         else
               } else
   if (i->dType == TYPE_S32) {
      assert(i->subOp <= 2);
   code[0] = 0x205 | (i->subOp << 5);
   if (hasDst)
         else
      } else
   if (i->dType == TYPE_F32) {
      assert(i->subOp == NV50_IR_SUBOP_ATOM_ADD);
   code[0] = 0x205;
   if (hasDst)
         else
                                 if (hasDst)
         else
   if (casOrExch)
            if (hasDst || casOrExch) {
      const int32_t offset = SDATA(i->src(0)).offset;
   assert(offset < 0x80000 && offset >= -0x80000);
   code[0] |= offset << 26;
   code[1] |= (offset & 0x1ffc0) >> 6;
      } else {
         }
   if (i->getIndirect(0, 0)) {
      srcId(i->getIndirect(0, 0), 20);
   if (i->getIndirect(0, 0)->reg.size == 8)
      } else {
                  if (i->subOp == NV50_IR_SUBOP_ATOM_CAS) {
      assert(i->src(1).getSize() == 2 * typeSizeof(i->sType));
         }
      void
   CodeEmitterNVC0::emitMEMBAR(const Instruction *i)
   {
      switch (NV50_IR_SUBOP_MEMBAR_SCOPE(i->subOp)) {
   case NV50_IR_SUBOP_MEMBAR_CTA: code[0] = 0x05; break;
   case NV50_IR_SUBOP_MEMBAR_GL:  code[0] = 0x25; break;
   default:
      code[0] = 0x45;
   assert(NV50_IR_SUBOP_MEMBAR_SCOPE(i->subOp) == NV50_IR_SUBOP_MEMBAR_SYS);
      }
               }
      void
   CodeEmitterNVC0::emitCCTL(const Instruction *i)
   {
               if (i->src(0).getFile() == FILE_MEMORY_GLOBAL) {
      code[1] = 0x98000000;
      } else {
      code[1] = 0xd0000000;
      }
   if (uses64bitAddress(i))
                              }
      void
   CodeEmitterNVC0::emitSUCLAMPMode(uint16_t subOp)
   {
      uint8_t m;
   switch (subOp & ~NV50_IR_SUBOP_SUCLAMP_2D) {
   case NV50_IR_SUBOP_SUCLAMP_SD(0, 1): m = 0; break;
   case NV50_IR_SUBOP_SUCLAMP_SD(1, 1): m = 1; break;
   case NV50_IR_SUBOP_SUCLAMP_SD(2, 1): m = 2; break;
   case NV50_IR_SUBOP_SUCLAMP_SD(3, 1): m = 3; break;
   case NV50_IR_SUBOP_SUCLAMP_SD(4, 1): m = 4; break;
   case NV50_IR_SUBOP_SUCLAMP_PL(0, 1): m = 5; break;
   case NV50_IR_SUBOP_SUCLAMP_PL(1, 1): m = 6; break;
   case NV50_IR_SUBOP_SUCLAMP_PL(2, 1): m = 7; break;
   case NV50_IR_SUBOP_SUCLAMP_PL(3, 1): m = 8; break;
   case NV50_IR_SUBOP_SUCLAMP_PL(4, 1): m = 9; break;
   case NV50_IR_SUBOP_SUCLAMP_BL(0, 1): m = 10; break;
   case NV50_IR_SUBOP_SUCLAMP_BL(1, 1): m = 11; break;
   case NV50_IR_SUBOP_SUCLAMP_BL(2, 1): m = 12; break;
   case NV50_IR_SUBOP_SUCLAMP_BL(3, 1): m = 13; break;
   case NV50_IR_SUBOP_SUCLAMP_BL(4, 1): m = 14; break;
   default:
         }
   code[0] |= m << 5;
   if (subOp & NV50_IR_SUBOP_SUCLAMP_2D)
      }
      void
   CodeEmitterNVC0::emitSUCalc(Instruction *i)
   {
      ImmediateValue *imm = NULL;
            if (i->srcExists(2)) {
      imm = i->getSrc(2)->asImm();
   if (imm)
               switch (i->op) {
   case OP_SUCLAMP: opc = HEX64(58000000, 00000004); break;
   case OP_SUBFM: opc = HEX64(5c000000, 00000004); break;
   case OP_SUEAU: opc = HEX64(60000000, 00000004); break;
   default:
      assert(0);
      }
            if (i->op == OP_SUCLAMP) {
      if (i->dType == TYPE_S32)
                     if (i->op == OP_SUBFM && i->subOp == NV50_IR_SUBOP_SUBFM_3D)
            if (i->op != OP_SUEAU) {
      if (i->def(0).getFile() == FILE_PREDICATE) { // p, #
      code[0] |= 63 << 14;
      } else
   if (i->defExists(1)) { // r, p
      assert(i->def(1).getFile() == FILE_PREDICATE);
      } else { // r, #
            }
   if (imm) {
      assert(i->op == OP_SUCLAMP);
   i->setSrc(2, imm);
         }
      void
   CodeEmitterNVC0::emitSUGType(DataType ty)
   {
      switch (ty) {
   case TYPE_S32: code[1] |= 1 << 13; break;
   case TYPE_U8:  code[1] |= 2 << 13; break;
   case TYPE_S8:  code[1] |= 3 << 13; break;
   default:
      assert(ty == TYPE_U32);
         }
      void
   CodeEmitterNVC0::setSUConst16(const Instruction *i, const int s)
   {
               assert(i->src(s).getFile() == FILE_MEMORY_CONST);
            code[1] |= 1 << 21;
   code[0] |= offset << 24;
   code[1] |= offset >> 8;
      }
      void
   CodeEmitterNVC0::setSUPred(const Instruction *i, const int s)
   {
      if (!i->srcExists(s) || (i->predSrc == s)) {
         } else {
      if (i->src(s).mod == Modifier(NV50_IR_MOD_NOT))
               }
      void
   CodeEmitterNVC0::emitSULDGB(const TexInstruction *i)
   {
      code[0] = 0x5;
            emitLoadStoreType(i->dType);
   emitSUGType(i->sType);
            emitPredicate(i);
   defId(i->def(0), 14); // destination
   srcId(i->src(0), 20); // address
   // format
   if (i->src(1).getFile() == FILE_GPR)
         else
            }
      void
   CodeEmitterNVC0::emitSUSTGx(const TexInstruction *i)
   {
      code[0] = 0x5;
            if (i->op == OP_SUSTP)
         else
         emitSUGType(i->sType);
            emitPredicate(i);
   srcId(i->src(0), 20); // address
   // format
   if (i->src(1).getFile() == FILE_GPR)
         else
         srcId(i->src(3), 14); // values
      }
      void
   CodeEmitterNVC0::emitSUAddr(const TexInstruction *i)
   {
               if (i->tex.rIndirectSrc < 0) {
      code[1] |= 0x00004000;
      } else {
            }
      void
   CodeEmitterNVC0::emitSUDim(const TexInstruction *i)
   {
               code[1] |= (i->tex.target.getDim() - 1) << 12;
   if (i->tex.target.isArray() || i->tex.target.isCube() ||
      i->tex.target.getDim() == 3) {
   // use e2d mode for 3-dim images, arrays and cubes.
                  }
      void
   CodeEmitterNVC0::emitSULEA(const TexInstruction *i)
   {
               code[0] = 0x5;
            emitPredicate(i);
                     if (i->defExists(1)) {
         } else {
                  emitSUAddr(i);
      }
      void
   CodeEmitterNVC0::emitSULDB(const TexInstruction *i)
   {
               code[0] = 0x5;
            emitPredicate(i);
                     emitCachingMode(i->cache);
   emitSUAddr(i);
      }
      void
   CodeEmitterNVC0::emitSUSTx(const TexInstruction *i)
   {
               code[0] = 0x5;
            if (i->op == OP_SUSTP)
         else
                              emitCachingMode(i->cache);
   emitSUAddr(i);
      }
      void
   CodeEmitterNVC0::emitVectorSubOp(const Instruction *i)
   {
      switch (NV50_IR_SUBOP_Vn(i->subOp)) {
   case 0:
      code[1] |= (i->subOp & 0x000f) << 12; // vsrc1
   code[1] |= (i->subOp & 0x00e0) >> 5;  // vsrc2
   code[1] |= (i->subOp & 0x0100) << 7;  // vsrc2
   code[1] |= (i->subOp & 0x3c00) << 13; // vdst
      case 1:
      code[1] |= (i->subOp & 0x000f) << 8;  // v2src1
   code[1] |= (i->subOp & 0x0010) << 11; // v2src1
   code[1] |= (i->subOp & 0x01e0) >> 1;  // v2src2
   code[1] |= (i->subOp & 0x0200) << 6;  // v2src2
   code[1] |= (i->subOp & 0x3c00) << 2;  // v4dst
   code[1] |= (i->mask & 0x3) << 2;
      case 2:
      code[1] |= (i->subOp & 0x000f) << 8; // v4src1
   code[1] |= (i->subOp & 0x01e0) >> 1; // v4src2
   code[1] |= (i->subOp & 0x3c00) << 2; // v4dst
   code[1] |= (i->mask & 0x3) << 2;
   code[1] |= (i->mask & 0xc) << 21;
      default:
      assert(0);
         }
      void
   CodeEmitterNVC0::emitVSHL(const Instruction *i)
   {
               switch (NV50_IR_SUBOP_Vn(i->subOp)) {
   case 0: opc |= 0xe8ULL << 56; break;
   case 1: opc |= 0xb4ULL << 56; break;
   case 2: opc |= 0x94ULL << 56; break;
   default:
      assert(0);
      }
   if (NV50_IR_SUBOP_Vn(i->subOp) == 1) {
      if (isSignedType(i->dType)) opc |= 1ULL << 0x2a;
      } else {
      if (isSignedType(i->dType)) opc |= 1ULL << 0x39;
      }
   emitForm_A(i, opc);
            if (i->saturate)
         if (i->flagsDef >= 0)
      }
      void
   CodeEmitterNVC0::emitPIXLD(const Instruction *i)
   {
      assert(i->encSize == 8);
   emitForm_A(i, HEX64(10000000, 00000006));
   code[0] |= i->subOp << 5;
      }
      void
   CodeEmitterNVC0::emitSHFL(const Instruction *i)
   {
                        code[0] = 0x00000005;
                     defId(i->def(0), 14);
            switch (i->src(1).getFile()) {
   case FILE_GPR:
      srcId(i->src(1), 26);
      case FILE_IMMEDIATE:
      imm = i->getSrc(1)->asImm();
   assert(imm && imm->reg.data.u32 < 0x20);
   code[0] |= imm->reg.data.u32 << 26;
   code[0] |= 1 << 5;
      default:
      assert(!"invalid src1 file");
               switch (i->src(2).getFile()) {
   case FILE_GPR:
      srcId(i->src(2), 49);
      case FILE_IMMEDIATE:
      imm = i->getSrc(2)->asImm();
   assert(imm && imm->reg.data.u32 < 0x2000);
   code[1] |= imm->reg.data.u32 << 10;
   code[0] |= 1 << 6;
      default:
      assert(!"invalid src2 file");
                  }
      void
   CodeEmitterNVC0::emitVOTE(const Instruction *i)
   {
      const ImmediateValue *imm;
            code[0] = 0x00000004 | (i->subOp << 5);
                     unsigned rp = 0;
   for (int d = 0; i->defExists(d); d++) {
      if (i->def(d).getFile() == FILE_PREDICATE) {
      assert(!(rp & 2));
   rp |= 2;
      } else if (i->def(d).getFile() == FILE_GPR) {
      assert(!(rp & 1));
   rp |= 1;
      } else {
            }
   if (!(rp & 1))
         if (!(rp & 2))
            switch (i->src(0).getFile()) {
   case FILE_PREDICATE:
      if (i->src(0).mod == Modifier(NV50_IR_MOD_NOT))
         srcId(i->src(0), 20);
      case FILE_IMMEDIATE:
      imm = i->getSrc(0)->asImm();
   assert(imm);
   u32 = imm->reg.data.u32;
   assert(u32 == 0 || u32 == 1);
   code[0] |= (u32 == 1 ? 0x7 : 0xf) << 20;
      default:
      assert(!"Unhandled src");
         }
      bool
   CodeEmitterNVC0::emitInstruction(Instruction *insn)
   {
               if (writeIssueDelays && !(codeSize & 0x3f))
            if (!insn->encSize) {
      ERROR("skipping unencodable instruction: "); insn->print();
      } else
   if (codeSize + size > codeSizeLimit) {
      ERROR("code emitter output buffer too small\n");
               if (writeIssueDelays) {
      if (!(codeSize & 0x3f)) {
      code[0] = 0x00000007; // cf issue delay "instruction"
   code[1] = 0x20000000;
   code += 2;
      }
   const unsigned int id = (codeSize & 0x3f) / 8 - 1;
   uint32_t *data = code - (id * 2 + 2);
   if (id <= 2) {
         } else
   if (id == 3) {
      data[0] |= insn->sched << 28;
      } else {
                     // assert that instructions with multiple defs don't corrupt registers
   for (int d = 0; insn->defExists(d); ++d)
            switch (insn->op) {
   case OP_MOV:
   case OP_RDSV:
      emitMOV(insn);
      case OP_NOP:
         case OP_LOAD:
      emitLOAD(insn);
      case OP_STORE:
      emitSTORE(insn);
      case OP_LINTERP:
   case OP_PINTERP:
      emitINTERP(insn);
      case OP_VFETCH:
      emitVFETCH(insn);
      case OP_EXPORT:
      emitEXPORT(insn);
      case OP_PFETCH:
      emitPFETCH(insn);
      case OP_AFETCH:
      emitAFETCH(insn);
      case OP_EMIT:
   case OP_RESTART:
      emitOUT(insn);
      case OP_ADD:
   case OP_SUB:
      if (insn->dType == TYPE_F64)
         else if (isFloatType(insn->dType))
         else
            case OP_MUL:
      if (insn->dType == TYPE_F64)
         else if (isFloatType(insn->dType))
         else
            case OP_MAD:
   case OP_FMA:
      if (insn->dType == TYPE_F64)
         else if (isFloatType(insn->dType))
         else
            case OP_SAD:
      emitISAD(insn);
      case OP_SHLADD:
      emitSHLADD(insn);
      case OP_NOT:
      emitNOT(insn);
      case OP_AND:
      emitLogicOp(insn, 0);
      case OP_OR:
      emitLogicOp(insn, 1);
      case OP_XOR:
      emitLogicOp(insn, 2);
      case OP_SHL:
   case OP_SHR:
      emitShift(insn);
      case OP_SET:
   case OP_SET_AND:
   case OP_SET_OR:
   case OP_SET_XOR:
      emitSET(insn->asCmp());
      case OP_SELP:
      emitSELP(insn);
      case OP_SLCT:
      emitSLCT(insn->asCmp());
      case OP_MIN:
   case OP_MAX:
      emitMINMAX(insn);
      case OP_ABS:
   case OP_NEG:
   case OP_CEIL:
   case OP_FLOOR:
   case OP_TRUNC:
   case OP_SAT:
      emitCVT(insn);
      case OP_CVT:
      if (insn->def(0).getFile() == FILE_PREDICATE ||
      insn->src(0).getFile() == FILE_PREDICATE)
      else
            case OP_RSQ:
      emitSFnOp(insn, 5 + 2 * insn->subOp);
      case OP_RCP:
      emitSFnOp(insn, 4 + 2 * insn->subOp);
      case OP_LG2:
      emitSFnOp(insn, 3);
      case OP_EX2:
      emitSFnOp(insn, 2);
      case OP_SIN:
      emitSFnOp(insn, 1);
      case OP_COS:
      emitSFnOp(insn, 0);
      case OP_PRESIN:
   case OP_PREEX2:
      emitPreOp(insn);
      case OP_TEX:
   case OP_TXB:
   case OP_TXL:
   case OP_TXD:
   case OP_TXF:
   case OP_TXG:
   case OP_TXLQ:
      emitTEX(insn->asTex());
      case OP_TXQ:
      emitTXQ(insn->asTex());
      case OP_TEXBAR:
      emitTEXBAR(insn);
      case OP_SUBFM:
   case OP_SUCLAMP:
   case OP_SUEAU:
      emitSUCalc(insn);
      case OP_MADSP:
      emitMADSP(insn);
      case OP_SULDB:
      if (targ->getChipset() >= NVISA_GK104_CHIPSET)
         else
            case OP_SUSTB:
   case OP_SUSTP:
      if (targ->getChipset() >= NVISA_GK104_CHIPSET)
         else
            case OP_SULEA:
      emitSULEA(insn->asTex());
      case OP_ATOM:
      emitATOM(insn);
      case OP_BRA:
   case OP_CALL:
   case OP_PRERET:
   case OP_RET:
   case OP_DISCARD:
   case OP_EXIT:
   case OP_PRECONT:
   case OP_CONT:
   case OP_PREBREAK:
   case OP_BREAK:
   case OP_JOINAT:
   case OP_BRKPT:
   case OP_QUADON:
   case OP_QUADPOP:
      emitFlow(insn);
      case OP_QUADOP:
      emitQUADOP(insn, insn->subOp, insn->lanes);
      case OP_DFDX:
      emitQUADOP(insn, insn->src(0).mod.neg() ? 0x66 : 0x99, 0x4);
      case OP_DFDY:
      emitQUADOP(insn, insn->src(0).mod.neg() ? 0x5a : 0xa5, 0x5);
      case OP_POPCNT:
      emitPOPC(insn);
      case OP_INSBF:
      emitINSBF(insn);
      case OP_EXTBF:
      emitEXTBF(insn);
      case OP_BFIND:
      emitBFIND(insn);
      case OP_PERMT:
      emitPERMT(insn);
      case OP_JOIN:
      emitNOP(insn);
   insn->join = 1;
      case OP_BAR:
      emitBAR(insn);
      case OP_MEMBAR:
      emitMEMBAR(insn);
      case OP_CCTL:
      emitCCTL(insn);
      case OP_VSHL:
      emitVSHL(insn);
      case OP_PIXLD:
      emitPIXLD(insn);
      case OP_SHFL:
      emitSHFL(insn);
      case OP_VOTE:
      emitVOTE(insn);
      case OP_PHI:
   case OP_UNION:
      ERROR("operation should have been eliminated");
      case OP_SQRT:
      ERROR("operation should have been lowered\n");
      default:
      ERROR("unknown op: %u\n", insn->op);
               if (insn->join) {
      code[0] |= 0x10;
               code += insn->encSize / 4;
   codeSize += insn->encSize;
      }
      uint32_t
   CodeEmitterNVC0::getMinEncodingSize(const Instruction *i) const
   {
               if (writeIssueDelays || info.minEncSize == 8 || true)
            if (i->ftz || i->saturate || i->join)
         if (i->rnd != ROUND_N)
         if (i->predSrc >= 0 && i->op == OP_MAD)
            if (i->op == OP_PINTERP) {
      if (i->getSampleMode() || true) // XXX: grr, short op doesn't work
      } else
   if (i->op == OP_MOV && i->lanes != 0xf) {
                  for (int s = 0; i->srcExists(s); ++s) {
      if (i->src(s).isIndirect(0))
            if (i->src(s).getFile() == FILE_MEMORY_CONST) {
      if (SDATA(i->src(s)).offset >= 0x100)
         if (i->getSrc(s)->reg.fileIndex > 1 &&
      i->getSrc(s)->reg.fileIndex != 16)
   } else
   if (i->src(s).getFile() == FILE_IMMEDIATE) {
      if (i->dType == TYPE_F32) {
      if (SDATA(i->src(s)).u32 >= 0x100)
      } else {
      if (SDATA(i->src(s)).u32 > 0xff)
                  if (i->op == OP_CVT)
         if (i->src(s).mod != Modifier(0)) {
      if (i->src(s).mod == Modifier(NV50_IR_MOD_ABS))
      if (i->op != OP_RSQ)
      if (i->src(s).mod == Modifier(NV50_IR_MOD_NEG))
      if (i->op != OP_ADD || s != 0)
                  }
      // Simplified, erring on safe side.
   class SchedDataCalculator : public Pass
   {
   public:
      SchedDataCalculator(const Target *targ) : score(NULL), prevData(0),
         private:
      struct RegScores
   {
      struct Resource {
      int st[DATA_FILE_COUNT]; // LD to LD delay 3
   int ld[DATA_FILE_COUNT]; // ST to ST delay 3
   int tex; // TEX to non-TEX delay 17 (0x11)
   int sfu; // SFU to SFU delay 3 (except PRE-ops)
      } res;
   struct ScoreData {
      int r[256];
   int p[8];
      } rd, wr;
   int base;
            void rebase(const int base)
   {
      const int delta = this->base - base;
   if (!delta)
                  for (int i = 0; i < regs; ++i) {
      rd.r[i] += delta;
      }
   for (int i = 0; i < 8; ++i) {
      rd.p[i] += delta;
      }
                  for (unsigned int f = 0; f < DATA_FILE_COUNT; ++f) {
      res.ld[f] += delta;
      }
   res.sfu += delta;
   res.imul += delta;
      }
   void wipe(int regs)
   {
      memset(&rd, 0, sizeof(rd));
   memset(&wr, 0, sizeof(wr));
   memset(&res, 0, sizeof(res));
      }
   int getLatest(const ScoreData& d) const
   {
      int max = 0;
   for (int i = 0; i < regs; ++i)
      if (d.r[i] > max)
      for (int i = 0; i < 8; ++i)
      if (d.p[i] > max)
      if (d.c > max)
            }
   inline int getLatestRd() const
   {
         }
   inline int getLatestWr() const
   {
         }
   inline int getLatest() const
   {
                     int max = MAX2(a, b);
   for (unsigned int f = 0; f < DATA_FILE_COUNT; ++f) {
      max = MAX2(res.ld[f], max);
      }
   max = MAX2(res.sfu, max);
   max = MAX2(res.imul, max);
   max = MAX2(res.tex, max);
      }
   void setMax(const RegScores *that)
   {
      for (int i = 0; i < regs; ++i) {
      rd.r[i] = MAX2(rd.r[i], that->rd.r[i]);
      }
   for (int i = 0; i < 8; ++i) {
      rd.p[i] = MAX2(rd.p[i], that->rd.p[i]);
      }
                  for (unsigned int f = 0; f < DATA_FILE_COUNT; ++f) {
      res.ld[f] = MAX2(res.ld[f], that->res.ld[f]);
      }
   res.sfu = MAX2(res.sfu, that->res.sfu);
   res.imul = MAX2(res.imul, that->res.imul);
      }
   void print(int cycle)
   {
      for (int i = 0; i < regs; ++i) {
      if (rd.r[i] > cycle)
         if (wr.r[i] > cycle)
      }
   for (int i = 0; i < 8; ++i) {
      if (rd.p[i] > cycle)
         if (wr.p[i] > cycle)
      }
   if (rd.c > cycle)
         if (wr.c > cycle)
         if (res.sfu > cycle)
         if (res.imul > cycle)
         if (res.tex > cycle)
                  RegScores *score; // for current BB
   std::vector<RegScores> scoreBoards;
   int prevData;
                     bool visit(Function *);
            void commitInsn(const Instruction *, int cycle);
   int calcDelay(const Instruction *, int cycle) const;
            void recordRd(const Value *, const int ready);
   void recordWr(const Value *, const int ready);
   void checkRd(const Value *, int cycle, int& delay) const;
               };
      void
   SchedDataCalculator::setDelay(Instruction *insn, int delay, Instruction *next)
   {
      if (insn->op == OP_EXIT || insn->op == OP_RET)
            if (insn->op == OP_TEXBAR) {
      // TODO: except if results not used before EXIT
      } else
   if (insn->op == OP_JOIN || insn->join) {
         } else
   if (delay >= 0 || prevData == 0x04 ||
      !next || !targ->canDualIssue(insn, next)) {
   insn->sched = static_cast<uint8_t>(MAX2(delay, 0));
   if (prevOp == OP_EXPORT)
         else
      } else {
                  if (prevData != 0x04 || prevOp != OP_EXPORT)
      if (insn->sched != 0x04 || insn->op == OP_EXPORT)
            }
      int
   SchedDataCalculator::getCycles(const Instruction *insn, int origDelay) const
   {
      if (insn->sched & 0x80) {
      int c = (insn->sched & 0x0f) * 2 + 1;
   if (insn->op == OP_TEXBAR && origDelay > 0)
            }
   if (insn->sched & 0x60)
            }
      bool
   SchedDataCalculator::visit(Function *func)
   {
      int regs = targ->getFileSize(FILE_GPR) + 1;
   scoreBoards.resize(func->cfg.getSize());
   for (size_t i = 0; i < scoreBoards.size(); ++i)
            }
      bool
   SchedDataCalculator::visit(BasicBlock *bb)
   {
      Instruction *insn;
                     prevData = 0x00;
   prevOp = OP_NOP;
            for (Graph::EdgeIterator ei = bb->cfg.incident(); !ei.end(); ei.next()) {
      // back branches will wait until all target dependencies are satisfied
   if (ei.getType() == Graph::Edge::BACK) // sched would be uninitialized
         BasicBlock *in = BasicBlock::get(ei.getNode());
   if (in->getExit()) {
      if (prevData != 0x04)
            }
      }
   if (bb->cfg.incidentCount() > 1)
         #ifdef NVC0_DEBUG_SCHED_DATA
      INFO("=== BB:%i initial scores\n", bb->getId());
      #endif
         for (insn = bb->getEntry(); insn && insn->next; insn = insn->next) {
               commitInsn(insn, cycle);
   int delay = calcDelay(next, cycle);
   setDelay(insn, delay, next);
      #ifdef NVC0_DEBUG_SCHED_DATA
         INFO("cycle %i, sched %02x\n", cycle, insn->sched);
   insn->print();
   #endif
      }
   if (!insn)
                           for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
               if (ei.getType() != Graph::Edge::BACK) {
      // only test the first instruction of the outgoing block
   next = out->getEntry();
   if (next)
      } else {
      // wait until all dependencies are satisfied
   const int regsFree = score->getLatest();
   next = out->getFirst();
   for (int c = cycle; next && c < regsFree; next = next->next) {
      bbDelay = MAX2(bbDelay, calcDelay(next, c));
      }
         }
   if (bb->cfg.outgoingCount() != 1)
         setDelay(insn, bbDelay, next);
            score->rebase(cycle); // common base for initializing out blocks' scores
      }
      #define NVE4_MAX_ISSUE_DELAY 0x1f
   int
   SchedDataCalculator::calcDelay(const Instruction *insn, int cycle) const
   {
               for (int s = 0; insn->srcExists(s); ++s)
         // WAR & WAW don't seem to matter
   // for (int s = 0; insn->srcExists(s); ++s)
            switch (Target::getOpClass(insn->op)) {
   case OPCLASS_SFU:
      ready = score->res.sfu;
      case OPCLASS_ARITH:
      if (insn->op == OP_MUL && !isFloatType(insn->dType))
            case OPCLASS_TEXTURE:
      ready = score->res.tex;
      case OPCLASS_LOAD:
      ready = score->res.ld[insn->src(0).getFile()];
      case OPCLASS_STORE:
      ready = score->res.st[insn->src(0).getFile()];
      default:
         }
   if (Target::getOpClass(insn->op) != OPCLASS_TEXTURE)
                     // if can issue next cycle, delay is 0, not 1
      }
      void
   SchedDataCalculator::commitInsn(const Instruction *insn, int cycle)
   {
               for (int d = 0; insn->defExists(d); ++d)
         // WAR & WAW don't seem to matter
   // for (int s = 0; insn->srcExists(s); ++s)
            switch (Target::getOpClass(insn->op)) {
   case OPCLASS_SFU:
      score->res.sfu = cycle + 4;
      case OPCLASS_ARITH:
      if (insn->op == OP_MUL && !isFloatType(insn->dType))
            case OPCLASS_TEXTURE:
      score->res.tex = cycle + 18;
      case OPCLASS_LOAD:
      if (insn->src(0).getFile() == FILE_MEMORY_CONST)
         score->res.ld[insn->src(0).getFile()] = cycle + 4;
   score->res.st[insn->src(0).getFile()] = ready;
      case OPCLASS_STORE:
      score->res.st[insn->src(0).getFile()] = cycle + 4;
   score->res.ld[insn->src(0).getFile()] = ready;
      case OPCLASS_OTHER:
      if (insn->op == OP_TEXBAR)
            default:
               #ifdef NVC0_DEBUG_SCHED_DATA
         #endif
   }
      void
   SchedDataCalculator::checkRd(const Value *v, int cycle, int& delay) const
   {
      int ready = cycle;
            switch (v->reg.file) {
   case FILE_GPR:
      a = v->reg.data.id;
   b = a + v->reg.size / 4;
   for (int r = a; r < b; ++r)
            case FILE_PREDICATE:
      ready = MAX2(ready, score->rd.p[v->reg.data.id]);
      case FILE_FLAGS:
      ready = MAX2(ready, score->rd.c);
      case FILE_SHADER_INPUT:
   case FILE_SHADER_OUTPUT: // yes, TCPs can read outputs
   case FILE_MEMORY_LOCAL:
   case FILE_MEMORY_CONST:
   case FILE_MEMORY_SHARED:
   case FILE_MEMORY_GLOBAL:
   case FILE_SYSTEM_VALUE:
      // TODO: any restrictions here ?
      case FILE_IMMEDIATE:
         default:
      assert(0);
      }
   if (cycle < ready)
      }
      void
   SchedDataCalculator::checkWr(const Value *v, int cycle, int& delay) const
   {
      int ready = cycle;
            switch (v->reg.file) {
   case FILE_GPR:
      a = v->reg.data.id;
   b = a + v->reg.size / 4;
   for (int r = a; r < b; ++r)
            case FILE_PREDICATE:
      ready = MAX2(ready, score->wr.p[v->reg.data.id]);
      default:
      assert(v->reg.file == FILE_FLAGS);
   ready = MAX2(ready, score->wr.c);
      }
   if (cycle < ready)
      }
      void
   SchedDataCalculator::recordWr(const Value *v, const int ready)
   {
               if (v->reg.file == FILE_GPR) {
      int b = a + v->reg.size / 4;
   for (int r = a; r < b; ++r)
      } else
   // $c, $pX: shorter issue-to-read delay (at least as exec pred and carry)
   if (v->reg.file == FILE_PREDICATE) {
         } else {
      assert(v->reg.file == FILE_FLAGS);
         }
      void
   SchedDataCalculator::recordRd(const Value *v, const int ready)
   {
               if (v->reg.file == FILE_GPR) {
      int b = a + v->reg.size / 4;
   for (int r = a; r < b; ++r)
      } else
   if (v->reg.file == FILE_PREDICATE) {
         } else
   if (v->reg.file == FILE_FLAGS) {
            }
      bool
   calculateSchedDataNVC0(const Target *targ, Function *func)
   {
      SchedDataCalculator sched(targ);
      }
      void
   CodeEmitterNVC0::prepareEmission(Function *func)
   {
               if (targ->hasSWSched)
      }
      CodeEmitterNVC0::CodeEmitterNVC0(const TargetNVC0 *target, Program::Type type)
      : CodeEmitter(target),
   targNVC0(target),
   progType(type),
      {
      code = NULL;
   codeSize = codeSizeLimit = 0;
      }
      CodeEmitter *
   TargetNVC0::createCodeEmitterNVC0(Program::Type type)
   {
      CodeEmitterNVC0 *emit = new CodeEmitterNVC0(this, type);
      }
      CodeEmitter *
   TargetNVC0::getCodeEmitter(Program::Type type)
   {
      if (chipset >= NVISA_GK20A_CHIPSET)
            }
      } // namespace nv50_ir
