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
      #include "nv50_ir.h"
   #include "nv50_ir_build_util.h"
      #include "nv50_ir_target_nv50.h"
      #define NV50_SU_INFO_SIZE_X   0x00
   #define NV50_SU_INFO_SIZE_Y   0x04
   #define NV50_SU_INFO_SIZE_Z   0x08
   #define NV50_SU_INFO_BSIZE    0x0c
   #define NV50_SU_INFO_STRIDE_Y 0x10
   #define NV50_SU_INFO_MS_X     0x18
   #define NV50_SU_INFO_MS_Y     0x1c
   #define NV50_SU_INFO_TILE_SHIFT_X 0x20
   #define NV50_SU_INFO_TILE_SHIFT_Y 0x24
   #define NV50_SU_INFO_TILE_SHIFT_Z 0x28
   #define NV50_SU_INFO_OFFSET_Z 0x2c
      #define NV50_SU_INFO__STRIDE 0x30
      #define NV50_SU_INFO_SIZE(i) (0x00 + (i) * 4)
   #define NV50_SU_INFO_MS(i)   (0x18 + (i) * 4)
   #define NV50_SU_INFO_TILE_SHIFT(i) (0x20 + (i) * 4)
      namespace nv50_ir {
      // nv50 doesn't support 32 bit integer multiplication
   //
   //       ah al * bh bl = LO32: (al * bh + ah * bl) << 16 + (al * bl)
   // -------------------
   //    al*bh 00           HI32: (al * bh + ah * bl) >> 16 + (ah * bh) +
   // ah*bh 00 00                 (           carry1) << 16 + ( carry2)
   //       al*bl
   //    ah*bl 00
   //
   // fffe0001 + fffe0001
   //
   // Note that this sort of splitting doesn't work for signed values, so we
   // compute the sign on those manually and then perform an unsigned multiply.
   static bool
   expandIntegerMUL(BuildUtil *bld, Instruction *mul)
   {
      const bool highResult = mul->subOp == NV50_IR_SUBOP_MUL_HIGH;
   ImmediateValue src1;
            DataType fTy; // full type
   switch (mul->sType) {
   case TYPE_S32: fTy = TYPE_U32; break;
   case TYPE_S64: fTy = TYPE_U64; break;
   default: fTy = mul->sType; break;
            DataType hTy; // half type
   switch (fTy) {
   case TYPE_U32: hTy = TYPE_U16; break;
   case TYPE_U64: hTy = TYPE_U32; break;
   default:
         }
   unsigned int fullSize = typeSizeof(fTy);
                              Value *s[2];
   Value *a[2], *b[2];
   Value *t[4];
   for (int j = 0; j < 4; ++j)
            if (isSignedType(mul->sType) && highResult) {
      s[0] = bld->getSSA(fullSize);
   s[1] = bld->getSSA(fullSize);
   bld->mkOp1(OP_ABS, mul->sType, s[0], mul->getSrc(0));
   bld->mkOp1(OP_ABS, mul->sType, s[1], mul->getSrc(1));
      } else {
      s[0] = mul->getSrc(0);
               // split sources into halves
   i[0] = bld->mkSplit(a, halfSize, s[0]);
            if (src1imm && (src1.reg.data.u32 & 0xffff0000) == 0) {
      i[2] = i[3] = bld->mkOp2(OP_MUL, fTy, t[1], a[1],
      } else {
      i[2] = bld->mkOp2(OP_MUL, fTy, t[0], a[0],
         if (src1imm && (src1.reg.data.u32 & 0x0000ffff) == 0) {
      i[3] = i[2];
      } else {
            }
   i[7] = bld->mkOp2(OP_SHL, fTy, t[2], t[1], bld->mkImm(halfSize * 8));
   if (src1imm && (src1.reg.data.u32 & 0x0000ffff) == 0) {
      i[4] = i[3];
      } else {
                  if (highResult) {
      Value *c[2];
   Value *r[5];
   Value *imm = bld->loadImm(NULL, 1 << (halfSize * 8));
   c[0] = bld->getSSA(1, FILE_FLAGS);
   c[1] = bld->getSSA(1, FILE_FLAGS);
   for (int j = 0; j < 5; ++j)
            i[8] = bld->mkOp2(OP_SHR, fTy, r[0], t[1], bld->mkImm(halfSize * 8));
   i[6] = bld->mkOp2(OP_ADD, fTy, r[1], r[0], imm);
   bld->mkMov(r[3], r[0])->setPredicate(CC_NC, c[0]);
   bld->mkOp2(OP_UNION, TYPE_U32, r[2], r[1], r[3]);
            // set carry defs / sources
   i[3]->setFlagsDef(1, c[0]);
   // actual result required in negative case, but ignored for
   // unsigned. for some reason the compiler ends up dropping the whole
   // instruction if the destination is unused but the flags are.
   if (isSignedType(mul->sType))
         else
         i[6]->setPredicate(CC_C, c[0]);
            if (isSignedType(mul->sType)) {
      Value *cc[2];
   Value *rr[7];
   Value *one = bld->getSSA(fullSize);
   bld->loadImm(one, 1);
                  // NOTE: this logic uses predicates because splitting basic blocks is
                  // Set the sign of the result based on the inputs
                  // 1s complement of 64-bit value
   bld->mkOp1(OP_NOT, fTy, rr[0], r[4])
                        // add to low 32-bits, keep track of the carry
   Instruction *n = bld->mkOp2(OP_ADD, fTy, NULL, rr[1], one);
                  // If there was a carry, add 1 to the upper 32 bits
   // XXX: These get executed even if they shouldn't be
   bld->mkOp2(OP_ADD, fTy, rr[2], rr[0], one)
         bld->mkMov(rr[3], rr[0])
                  // Merge the results from the negative and non-negative paths
   bld->mkMov(rr[5], rr[4])
         bld->mkMov(rr[6], r[4])
            } else {
            } else {
         }
            for (int j = 2; j <= (highResult ? 5 : 4); ++j)
      if (i[j])
            }
      #define QOP_ADD  0
   #define QOP_SUBR 1
   #define QOP_SUB  2
   #define QOP_MOV2 3
      //             UL UR LL LR
   #define QUADOP(q, r, s, t)            \
      ((QOP_##q << 6) | (QOP_##r << 4) | \
         class NV50LegalizePostRA : public Pass
   {
   public:
            private:
      virtual bool visit(Function *);
            void handlePRERET(FlowInstruction *);
                        };
      bool
   NV50LegalizePostRA::visit(Function *fn)
   {
               r63 = new_LValue(fn, FILE_GPR);
   // GPR units on nv50 are in half-regs
   if (prog->maxGPR < 126)
         else
            // this is actually per-program, but we can do it all on visiting main()
   std::list<Instruction *> *outWrites =
            if (outWrites) {
      for (std::list<Instruction *>::iterator it = outWrites->begin();
      it != outWrites->end(); ++it)
      // instructions will be deleted on exit
                  }
      void
   NV50LegalizePostRA::replaceZero(Instruction *i)
   {
      for (int s = 0; i->srcExists(s); ++s) {
      ImmediateValue *imm = i->getSrc(s)->asImm();
   if (imm && imm->reg.data.u64 == 0)
         }
      // Emulate PRERET: jump to the target and call to the origin from there
   //
   // WARNING: atm only works if BBs are affected by at most a single PRERET
   //
   // BB:0
   // preret BB:3
   // (...)
   // BB:3
   // (...)
   //             --->
   // BB:0
   // bra BB:3 + n0 (directly to the call; move to beginning of BB and fixate)
   // (...)
   // BB:3
   // bra BB:3 + n1 (skip the call)
   // call BB:0 + n2 (skip bra at beginning of BB:0)
   // (...)
   void
   NV50LegalizePostRA::handlePRERET(FlowInstruction *pre)
   {
      BasicBlock *bbE = pre->bb;
            pre->subOp = NV50_IR_SUBOP_EMU_PRERET + 0;
   bbE->remove(pre);
            Instruction *skip = new_FlowInstruction(func, OP_PRERET, bbT);
            bbT->insertHead(call);
                     skip->subOp = NV50_IR_SUBOP_EMU_PRERET + 1;
      }
      bool
   NV50LegalizePostRA::visit(BasicBlock *bb)
   {
               // remove pseudo operations and non-fixed no-ops, split 64 bit operations
   for (i = bb->getFirst(); i; i = next) {
      next = i->next;
   if (i->isNop()) {
         } else
   if (i->op == OP_PRERET && prog->getTarget()->getChipset() < 0xa0) {
         } else {
      // TODO: We will want to do this before register allocation,
   // since have to use a $c register for the carry flag.
   if (typeSizeof(i->dType) == 8) {
      Instruction *hi = BuildUtil::split64BitOpPostRA(func, i, r63, NULL);
   if (hi)
               if (i->op != OP_PFETCH && i->op != OP_BAR &&
      (!i->defExists(0) || i->def(0).getFile() != FILE_ADDRESS))
      }
   if (!bb->getEntry())
               }
      class NV50LegalizeSSA : public Pass
   {
   public:
                     private:
      void propagateWriteToOutput(Instruction *);
   void handleDIV(Instruction *);
   void handleMOD(Instruction *);
   void handleMUL(Instruction *);
                                 };
      NV50LegalizeSSA::NV50LegalizeSSA(Program *prog)
   {
               if (prog->optLevel >= 2 &&
      (prog->getType() == Program::TYPE_GEOMETRY ||
   prog->getType() == Program::TYPE_VERTEX))
   outWrites =
      else
      }
      void
   NV50LegalizeSSA::propagateWriteToOutput(Instruction *st)
   {
      if (st->src(0).isIndirect(0) || st->getSrc(1)->refCount() != 1)
            // check def instruction can store
            // TODO: move exports (if beneficial) in common opt pass
   if (di->isPseudo() || isTextureOp(di->op) || di->defCount(0xff, true) > 1)
            for (int s = 0; di->srcExists(s); ++s)
      if (di->src(s).getFile() == FILE_IMMEDIATE ||
               if (prog->getType() == Program::TYPE_GEOMETRY) {
      // Only propagate output writes in geometry shaders when we can be sure
   // that we are propagating to the same output vertex.
   if (di->bb != st->bb)
         Instruction *i;
   for (i = di; i != st; i = i->next) {
      if (i->op == OP_EMIT || i->op == OP_RESTART)
      }
               // We cannot set defs to non-lvalues before register allocation, so
   // save & remove (to save registers) the exports and replace later.
   outWrites->push_back(st);
      }
      bool
   NV50LegalizeSSA::isARL(const Instruction *i) const
   {
               if (i->op != OP_SHL || i->src(0).getFile() != FILE_GPR)
         if (!i->src(1).getImmediate(imm))
            }
      void
   NV50LegalizeSSA::handleAddrDef(Instruction *i)
   {
                        // PFETCH can always write to $a
   if (i->op == OP_PFETCH)
         // only ADDR <- SHL(GPR, IMM) and ADDR <- ADD(ADDR, IMM) are valid
   if (i->srcExists(1) && i->src(1).getFile() == FILE_IMMEDIATE) {
      if (i->op == OP_SHL && i->src(0).getFile() == FILE_GPR)
         if (i->op == OP_ADD && i->src(0).getFile() == FILE_ADDRESS)
               // turn $a sources into $r sources (can't operate on $a)
   for (int s = 0; i->srcExists(s); ++s) {
      Value *a = i->getSrc(s);
   Value *r;
   if (a->reg.file == FILE_ADDRESS) {
      if (a->getInsn() && isARL(a->getInsn())) {
         } else {
      bld.setPosition(i, false);
   r = bld.getSSA();
   bld.mkMov(r, a);
            }
   if (i->op == OP_SHL && i->src(1).getFile() == FILE_IMMEDIATE)
            // turn result back into $a
   bld.setPosition(i, true);
   arl = bld.mkOp2(OP_SHL, TYPE_U32, i->getDef(0), bld.getSSA(), bld.mkImm(0));
      }
      void
   NV50LegalizeSSA::handleMUL(Instruction *mul)
   {
      if (isFloatType(mul->sType) || typeSizeof(mul->sType) <= 2)
         Value *def = mul->getDef(0);
   Value *pred = mul->getPredicate();
   CondCode cc = mul->cc;
   if (pred)
            if (mul->op == OP_MAD) {
      Instruction *add = mul;
   bld.setPosition(add, false);
   Value *res = cloneShallow(func, mul->getDef(0));
   mul = bld.mkOp2(OP_MUL, add->sType, res, add->getSrc(0), add->getSrc(1));
   add->op = OP_ADD;
   add->setSrc(0, mul->getDef(0));
   add->setSrc(1, add->getSrc(2));
   for (int s = 2; add->srcExists(s); ++s)
         mul->subOp = add->subOp;
      }
   expandIntegerMUL(&bld, mul);
   if (pred)
      }
      // Use f32 division: first compute an approximate result, use it to reduce
   // the dividend, which should then be representable as f32, divide the reduced
   // dividend, and add the quotients.
   void
   NV50LegalizeSSA::handleDIV(Instruction *div)
   {
               if (ty != TYPE_U32 && ty != TYPE_S32)
                              Value *a, *af = bld.getSSA();
            bld.mkCvt(OP_CVT, TYPE_F32, af, ty, div->getSrc(0));
            if (isSignedType(ty)) {
      af->getInsn()->src(0).mod = Modifier(NV50_IR_MOD_ABS);
   bf->getInsn()->src(0).mod = Modifier(NV50_IR_MOD_ABS);
   a = bld.getSSA();
   b = bld.getSSA();
   bld.mkOp1(OP_ABS, ty, a, div->getSrc(0));
      } else {
      a = div->getSrc(0);
               bf = bld.mkOp1v(OP_RCP, TYPE_F32, bld.getSSA(), bf);
            bld.mkOp2(OP_MUL, TYPE_F32, (qf = bld.getSSA()), af, bf)->rnd = ROUND_Z;
            // get error of 1st result
   expandIntegerMUL(&bld,
                           bld.mkOp2(OP_MUL, TYPE_F32, (qRf = bld.getSSA()), aR, bf)->rnd = ROUND_Z;
   bld.mkCvt(OP_CVT, TYPE_U32, (qR = bld.getSSA()), TYPE_F32, qRf)
                  // correction: if modulus >= divisor, add 1
   expandIntegerMUL(&bld,
         bld.mkOp2(OP_SUB, TYPE_U32, (m = bld.getSSA()), a, t);
   bld.mkCmp(OP_SET, CC_GE, TYPE_U32, (s = bld.getSSA()), TYPE_U32, m, b);
   if (!isSignedType(ty)) {
      div->op = OP_SUB;
   div->setSrc(0, q);
      } else {
      t = q;
   bld.mkOp2(OP_SUB, TYPE_U32, (q = bld.getSSA()), t, s);
   s = bld.getSSA();
   t = bld.getSSA();
   // fix the sign
   bld.mkOp2(OP_XOR, TYPE_U32, NULL, div->getSrc(0), div->getSrc(1))
         bld.mkOp1(OP_NEG, ty, s, q)->setPredicate(CC_S, cond);
            div->op = OP_UNION;
   div->setSrc(0, s);
         }
      void
   NV50LegalizeSSA::handleMOD(Instruction *mod)
   {
      if (mod->dType != TYPE_U32 && mod->dType != TYPE_S32)
                  Value *q = bld.getSSA();
            bld.mkOp2(OP_DIV, mod->dType, q, mod->getSrc(0), mod->getSrc(1));
            bld.setPosition(mod, false);
            mod->op = OP_SUB;
      }
      bool
   NV50LegalizeSSA::visit(BasicBlock *bb)
   {
      Instruction *insn, *next;
   // skipping PHIs (don't pass them to handleAddrDef) !
   for (insn = bb->getEntry(); insn; insn = next) {
               if (insn->defExists(0) && insn->getDef(0)->reg.file == FILE_ADDRESS)
            switch (insn->op) {
   case OP_EXPORT:
      if (outWrites)
            case OP_DIV:
      handleDIV(insn);
      case OP_MOD:
      handleMOD(insn);
      case OP_MAD:
   case OP_MUL:
      handleMUL(insn);
      default:
            }
      }
      class NV50LoweringPreSSA : public Pass
   {
   public:
            private:
      virtual bool visit(Instruction *);
                     bool handlePFETCH(Instruction *);
   bool handleEXPORT(Instruction *);
   bool handleLOAD(Instruction *);
   bool handleLDST(Instruction *);
   bool handleMEMBAR(Instruction *);
   bool handleSharedATOM(Instruction *);
   bool handleSULDP(TexInstruction *);
   bool handleSUREDP(TexInstruction *);
   bool handleSUSTP(TexInstruction *);
            bool handleDIV(Instruction *);
            bool handleSET(Instruction *);
   bool handleSLCT(CmpInstruction *);
            bool handleTEX(TexInstruction *);
   bool handleTXB(TexInstruction *); // I really
   bool handleTXL(TexInstruction *); // hate
   bool handleTXD(TexInstruction *); // these 3
   bool handleTXLQ(TexInstruction *);
   bool handleTXQ(TexInstruction *);
   bool handleSUQ(TexInstruction *);
            bool handleCALL(Instruction *);
   bool handlePRECONT(Instruction *);
            void checkPredicate(Instruction *);
   void loadTexMsInfo(uint32_t off, Value **ms, Value **ms_x, Value **ms_y);
   void loadMsInfo(Value *ms, Value *s, Value **dx, Value **dy);
   Value *loadSuInfo(int slot, uint32_t off);
         private:
                           };
      NV50LoweringPreSSA::NV50LoweringPreSSA(Program *prog) :
         {
         }
      bool
   NV50LoweringPreSSA::visit(Function *f)
   {
               if (prog->getType() == Program::TYPE_COMPUTE) {
      // Add implicit "thread id" argument in $r0 to the function
   Value *arg = new_LValue(func, FILE_GPR);
   arg->reg.data.id = 0;
            bld.setPosition(root, false);
                  }
      void NV50LoweringPreSSA::loadTexMsInfo(uint32_t off, Value **ms,
            // This loads the texture-indexed ms setting from the constant buffer
   Value *tmp = new_LValue(func, FILE_GPR);
   uint8_t b = prog->driver->io.auxCBSlot;
   off += prog->driver->io.suInfoBase;
   if (prog->getType() > Program::TYPE_VERTEX)
         if (prog->getType() > Program::TYPE_GEOMETRY)
         if (prog->getType() > Program::TYPE_FRAGMENT)
         *ms_x = bld.mkLoadv(TYPE_U32, bld.mkSymbol(
         *ms_y = bld.mkLoadv(TYPE_U32, bld.mkSymbol(
            }
      void NV50LoweringPreSSA::loadMsInfo(Value *ms, Value *s, Value **dx, Value **dy) {
      // Given a MS level, and a sample id, compute the delta x/y
   uint8_t b = prog->driver->io.msInfoCBSlot;
            // The required information is at mslevel * 16 * 4 + sample * 8
   // = (mslevel * 8 + sample) * 8
   bld.mkOp2(OP_SHL,
            TYPE_U32,
   off,
   bld.mkOp2v(OP_ADD, TYPE_U32, t,
            *dx = bld.mkLoadv(TYPE_U32, bld.mkSymbol(
               *dy = bld.mkLoadv(TYPE_U32, bld.mkSymbol(
            }
      Value *
   NV50LoweringPreSSA::loadSuInfo(int slot, uint32_t off)
   {
      uint8_t b = prog->driver->io.auxCBSlot;
   off += prog->driver->io.bufInfoBase + slot * NV50_SU_INFO__STRIDE;
   return bld.mkLoadv(TYPE_U32, bld.mkSymbol(
      }
      Value *
   NV50LoweringPreSSA::loadSuInfo16(int slot, uint32_t off)
   {
      uint8_t b = prog->driver->io.auxCBSlot;
   off += prog->driver->io.bufInfoBase + slot * NV50_SU_INFO__STRIDE;
   return bld.mkLoadv(TYPE_U16, bld.mkSymbol(
      }
      bool
   NV50LoweringPreSSA::handleTEX(TexInstruction *i)
   {
      const int arg = i->tex.target.getArgCount();
   const int dref = arg;
            /* Only normalize in the non-explicit derivatives case.
   */
   if (i->tex.target.isCube() && i->op != OP_TXD) {
      Value *src[3], *val;
   int c;
   for (c = 0; c < 3; ++c)
         val = bld.getScratch();
   bld.mkOp2(OP_MAX, TYPE_F32, val, src[0], src[1]);
   bld.mkOp2(OP_MAX, TYPE_F32, val, src[2], val);
   bld.mkOp1(OP_RCP, TYPE_F32, val, val);
   for (c = 0; c < 3; ++c) {
      i->setSrc(c, bld.mkOp2v(OP_MUL, TYPE_F32, bld.getSSA(),
                  // handle MS, which means looking up the MS params for this texture, and
   // adjusting the input coordinates to point at the right sample.
   if (i->tex.target.isMS()) {
      Value *x = i->getSrc(0);
   Value *y = i->getSrc(1);
   Value *s = i->getSrc(arg - 1);
   Value *tx = new_LValue(func, FILE_GPR), *ty = new_LValue(func, FILE_GPR),
                     loadTexMsInfo(i->tex.r * 4 * 2, &ms, &ms_x, &ms_y);
            bld.mkOp2(OP_SHL, TYPE_U32, tx, x, ms_x);
   bld.mkOp2(OP_SHL, TYPE_U32, ty, y, ms_y);
   bld.mkOp2(OP_ADD, TYPE_U32, tx, tx, dx);
   bld.mkOp2(OP_ADD, TYPE_U32, ty, ty, dy);
   i->setSrc(0, tx);
   i->setSrc(1, ty);
               // dref comes before bias/lod
   if (i->tex.target.isShadow())
      if (i->op == OP_TXB || i->op == OP_TXL)
         if (i->tex.target.isArray()) {
      if (i->op != OP_TXF) {
      // array index must be converted to u32, but it's already an integer
   // for TXF
   Value *layer = i->getSrc(arg - 1);
   LValue *src = new_LValue(func, FILE_GPR);
   bld.mkCvt(OP_CVT, TYPE_U32, src, TYPE_F32, layer);
   bld.mkOp2(OP_MIN, TYPE_U32, src, src, bld.loadImm(NULL, 511));
      }
   if (i->tex.target.isCube() && i->srcCount() > 4) {
                     acube.resize(4);
   for (c = 0; c < 4; ++c)
         a2d.resize(4);
   for (c = 0; c < 3; ++c)
                                 for (c = 0; c < 3; ++c)
         for (; i->srcExists(c + 1); ++c)
                        i->tex.target = i->tex.target.isShadow() ?
                  // texel offsets are 3 immediate fields in the instruction,
   // nv50 cannot do textureGatherOffsets
   assert(i->tex.useOffsets <= 1);
   if (i->tex.useOffsets) {
      for (int c = 0; c < 3; ++c) {
      ImmediateValue val;
   if (!i->offset[0][c].getImmediate(val))
         i->tex.offset[c] = val.reg.data.u32;
                     }
      // Bias must be equal for all threads of a quad or lod calculation will fail.
   //
   // The lanes of a quad are grouped by the bit in the condition register they
   // have set, which is selected by differing bias values.
   // Move the input values for TEX into a new register set for each group and
   // execute TEX only for a specific group.
   // We always need to use 4 new registers for the inputs/outputs because the
   // implicitly calculated derivatives must be correct.
   //
   // TODO: move to SSA phase so we can easily determine whether bias is constant
   bool
   NV50LoweringPreSSA::handleTXB(TexInstruction *i)
   {
      const CondCode cc[4] = { CC_EQU, CC_S, CC_C, CC_O };
            // We can't actually apply bias *and* do a compare for a cube
   // texture. Since the compare has to be done before the filtering, just
   // drop the bias on the floor.
   if (i->tex.target == TEX_TARGET_CUBE_SHADOW) {
      i->op = OP_TEX;
   i->setSrc(3, i->getSrc(4));
   i->setSrc(4, NULL);
               handleTEX(i);
   Value *bias = i->getSrc(i->tex.target.getArgCount());
   if (bias->isUniform())
            Instruction *cond = bld.mkOp1(OP_UNION, TYPE_U32, bld.getScratch(),
                  for (l = 1; l < 4; ++l) {
      const uint8_t qop = QUADOP(SUBR, SUBR, SUBR, SUBR);
   Value *bit = bld.getSSA();
   Value *pred = bld.getScratch(1, FILE_FLAGS);
   Value *imm = bld.loadImm(NULL, (1 << l));
   bld.mkQuadop(qop, pred, l, bias, bias)->flagsDef = 0;
   bld.mkMov(bit, imm)->setPredicate(CC_EQ, pred);
      }
   Value *flags = bld.getScratch(1, FILE_FLAGS);
   bld.setPosition(cond, true);
            Instruction *tex[4];
   for (l = 0; l < 4; ++l) {
      (tex[l] = cloneForward(func, i))->setPredicate(cc[l], flags);
               Value *res[4][4];
   for (d = 0; i->defExists(d); ++d)
         for (l = 1; l < 4; ++l) {
      for (d = 0; tex[l]->defExists(d); ++d) {
      res[l][d] = cloneShallow(func, res[0][d]);
                  for (d = 0; i->defExists(d); ++d) {
      Instruction *dst = bld.mkOp(OP_UNION, TYPE_U32, i->getDef(d));
   for (l = 0; l < 4; ++l)
      }
   delete_Instruction(prog, i);
      }
      // LOD must be equal for all threads of a quad.
   // Unlike with TXB, here we can just diverge since there's no LOD calculation
   // that would require all 4 threads' sources to be set up properly.
   bool
   NV50LoweringPreSSA::handleTXL(TexInstruction *i)
   {
      handleTEX(i);
   Value *lod = i->getSrc(i->tex.target.getArgCount());
   if (lod->isUniform())
            BasicBlock *currBB = i->bb;
   BasicBlock *texiBB = i->bb->splitBefore(i, false);
            bld.setPosition(currBB, true);
   assert(!currBB->joinAt);
            for (int l = 0; l <= 3; ++l) {
      const uint8_t qop = QUADOP(SUBR, SUBR, SUBR, SUBR);
   Value *pred = bld.getScratch(1, FILE_FLAGS);
   bld.setPosition(currBB, true);
   bld.mkQuadop(qop, pred, l, lod, lod)->flagsDef = 0;
   bld.mkFlow(OP_BRA, texiBB, CC_EQ, pred)->fixed = 1;
   currBB->cfg.attach(&texiBB->cfg, Graph::Edge::FORWARD);
   if (l <= 2) {
      BasicBlock *laneBB = new BasicBlock(func);
   currBB->cfg.attach(&laneBB->cfg, Graph::Edge::TREE);
         }
   bld.setPosition(joinBB, false);
   bld.mkFlow(OP_JOIN, NULL, CC_ALWAYS, NULL)->fixed = 1;
      }
      bool
   NV50LoweringPreSSA::handleTXD(TexInstruction *i)
   {
      static const uint8_t qOps[4][2] =
   {
      { QUADOP(MOV2, ADD,  MOV2, ADD),  QUADOP(MOV2, MOV2, ADD,  ADD) }, // l0
   { QUADOP(SUBR, MOV2, SUBR, MOV2), QUADOP(MOV2, MOV2, ADD,  ADD) }, // l1
   { QUADOP(MOV2, ADD,  MOV2, ADD),  QUADOP(SUBR, SUBR, MOV2, MOV2) }, // l2
      };
   Value *def[4][4];
   Value *crd[3];
   Instruction *tex;
   Value *zero = bld.loadImm(bld.getSSA(), 0);
   int l, c;
            handleTEX(i);
   i->op = OP_TEX; // no need to clone dPdx/dPdy later
            for (c = 0; c < dim; ++c)
            bld.mkOp(OP_QUADON, TYPE_NONE, NULL);
   for (l = 0; l < 4; ++l) {
      Value *src[3], *val;
   // mov coordinates from lane l to all lanes
   for (c = 0; c < dim; ++c)
         // add dPdx from lane l to lanes dx
   for (c = 0; c < dim; ++c)
         // add dPdy from lane l to lanes dy
   for (c = 0; c < dim; ++c)
         // normalize cube coordinates if necessary
   if (i->tex.target.isCube()) {
      for (c = 0; c < 3; ++c)
         val = bld.getScratch();
   bld.mkOp2(OP_MAX, TYPE_F32, val, src[0], src[1]);
   bld.mkOp2(OP_MAX, TYPE_F32, val, src[2], val);
   bld.mkOp1(OP_RCP, TYPE_F32, val, val);
   for (c = 0; c < 3; ++c)
      } else {
      for (c = 0; c < dim; ++c)
      }
   // texture
   bld.insert(tex = cloneForward(func, i));
   for (c = 0; c < dim; ++c)
         // save results
   for (c = 0; i->defExists(c); ++c) {
      Instruction *mov;
   def[c][l] = bld.getSSA();
   mov = bld.mkMov(def[c][l], tex->getDef(c));
   mov->fixed = 1;
         }
            for (c = 0; i->defExists(c); ++c) {
      Instruction *u = bld.mkOp(OP_UNION, TYPE_U32, i->getDef(c));
   for (l = 0; l < 4; ++l)
               i->bb->remove(i);
      }
      bool
   NV50LoweringPreSSA::handleTXLQ(TexInstruction *i)
   {
      handleTEX(i);
            /* The returned values are not quite what we want:
   * (a) convert from s32 to f32
   * (b) multiply by 1/256
   */
   for (int def = 0; def < 2; ++def) {
      if (!i->defExists(def))
         bld.mkCvt(OP_CVT, TYPE_F32, i->getDef(def), TYPE_S32, i->getDef(def));
   bld.mkOp2(OP_MUL, TYPE_F32, i->getDef(def),
      }
      }
      bool
   NV50LoweringPreSSA::handleTXQ(TexInstruction *i)
   {
      Value *ms, *ms_x, *ms_y;
   if (i->tex.query == TXQ_DIMS) {
      if (i->tex.target.isMS()) {
      bld.setPosition(i, true);
   loadTexMsInfo(i->tex.r * 4 * 2, &ms, &ms_x, &ms_y);
   int d = 0;
   if (i->tex.mask & 1) {
      bld.mkOp2(OP_SHR, TYPE_U32, i->getDef(d), i->getDef(d), ms_x);
      }
   if (i->tex.mask & 2) {
      bld.mkOp2(OP_SHR, TYPE_U32, i->getDef(d), i->getDef(d), ms_y);
         }
      }
   assert(i->tex.query == TXQ_TYPE);
            loadTexMsInfo(i->tex.r * 4 * 2, &ms, &ms_x, &ms_y);
   bld.mkOp2(OP_SHL, TYPE_U32, i->getDef(0), bld.loadImm(NULL, 1), ms);
               }
      bool
   NV50LoweringPreSSA::handleSUQ(TexInstruction *suq)
   {
      const int dim = suq->tex.target.getDim();
   const int arg = dim + (suq->tex.target.isArray() || suq->tex.target.isCube());
   int mask = suq->tex.mask;
   int slot = suq->tex.r;
            for (c = 0, d = 0; c < 3; ++c, mask >>= 1) {
      if (c >= arg || !(mask & 1))
                     if (c == 1 && suq->tex.target == TEX_TARGET_1D_ARRAY) {
         } else {
         }
   bld.mkMov(suq->getDef(d++), loadSuInfo(slot, offset));
   if (c == 2 && suq->tex.target.isCube())
      bld.mkOp2(OP_DIV, TYPE_U32, suq->getDef(d - 1), suq->getDef(d - 1),
            if (mask & 1) {
      if (suq->tex.target.isMS()) {
      Value *ms_x = loadSuInfo(slot, NV50_SU_INFO_MS(0));
   Value *ms_y = loadSuInfo(slot, NV50_SU_INFO_MS(1));
   Value *ms = bld.mkOp2v(OP_ADD, TYPE_U32, bld.getScratch(), ms_x, ms_y);
      } else {
                     bld.remove(suq);
      }
      bool
   NV50LoweringPreSSA::handleBUFQ(Instruction *bufq)
   {
      bufq->op = OP_MOV;
   bufq->setSrc(0, loadSuInfo(bufq->getSrc(0)->reg.fileIndex, NV50_SU_INFO_SIZE_X));
   bufq->setIndirect(0, 0, NULL);
   bufq->setIndirect(0, 1, NULL);
      }
      bool
   NV50LoweringPreSSA::handleSET(Instruction *i)
   {
      if (i->dType == TYPE_F32) {
      bld.setPosition(i, true);
   i->dType = TYPE_U32;
   bld.mkOp1(OP_ABS, TYPE_S32, i->getDef(0), i->getDef(0));
      }
      }
      bool
   NV50LoweringPreSSA::handleSLCT(CmpInstruction *i)
   {
      Value *src0 = bld.getSSA();
   Value *src1 = bld.getSSA();
            Value *v0 = i->getSrc(0);
   Value *v1 = i->getSrc(1);
   // XXX: these probably shouldn't be immediates in the first place ...
   if (v0->asImm())
         if (v1->asImm())
            bld.setPosition(i, true);
   bld.mkMov(src0, v0)->setPredicate(CC_NE, pred);
   bld.mkMov(src1, v1)->setPredicate(CC_EQ, pred);
            bld.setPosition(i, false);
   i->op = OP_SET;
   i->setFlagsDef(0, pred);
   i->dType = TYPE_U8;
   i->setSrc(0, i->getSrc(2));
   i->setSrc(2, NULL);
               }
      bool
   NV50LoweringPreSSA::handleSELP(Instruction *i)
   {
      Value *src0 = bld.getSSA();
            Value *v0 = i->getSrc(0);
   Value *v1 = i->getSrc(1);
   if (v0->asImm())
         if (v1->asImm())
            bld.mkMov(src0, v0)->setPredicate(CC_NE, i->getSrc(2));
   bld.mkMov(src1, v1)->setPredicate(CC_EQ, i->getSrc(2));
   bld.mkOp2(OP_UNION, i->dType, i->getDef(0), src0, src1);
   delete_Instruction(prog, i);
      }
      bool
   NV50LoweringPreSSA::handleCALL(Instruction *i)
   {
      if (prog->getType() == Program::TYPE_COMPUTE) {
      // Add implicit "thread id" argument in $r0 to the function
      }
      }
      bool
   NV50LoweringPreSSA::handlePRECONT(Instruction *i)
   {
      delete_Instruction(prog, i);
      }
      bool
   NV50LoweringPreSSA::handleCONT(Instruction *i)
   {
      i->op = OP_BRA;
      }
      bool
   NV50LoweringPreSSA::handleRDSV(Instruction *i)
   {
      Symbol *sym = i->getSrc(0)->asSym();
   uint32_t addr = targ->getSVAddress(FILE_SHADER_INPUT, sym);
   Value *def = i->getDef(0);
   SVSemantic sv = sym->reg.data.sv.sv;
            if (addr >= 0x400) // mov $sreg
            switch (sv) {
   case SV_POSITION:
      assert(prog->getType() == Program::TYPE_FRAGMENT);
   bld.mkInterp(NV50_IR_INTERP_LINEAR, i->getDef(0), addr, NULL);
      case SV_FACE:
      bld.mkInterp(NV50_IR_INTERP_FLAT, def, addr, NULL);
   if (i->dType == TYPE_F32) {
      bld.mkOp2(OP_OR, TYPE_U32, def, def, bld.mkImm(0x00000001));
   bld.mkOp1(OP_NEG, TYPE_S32, def, def);
      }
      case SV_NCTAID:
   case SV_CTAID:
   case SV_NTID: {
      Value *x = bld.getSSA(2);
   bld.mkOp1(OP_LOAD, TYPE_U16, x,
         bld.mkCvt(OP_CVT, TYPE_U32, def, TYPE_U16, x);
      }
   case SV_TID:
      if (idx == 0) {
         } else if (idx == 1) {
      bld.mkOp2(OP_AND, TYPE_U32, def, tid, bld.mkImm(0x03ff0000));
      } else if (idx == 2) {
         } else {
         }
      case SV_COMBINED_TID:
      bld.mkMov(def, tid);
      case SV_SAMPLE_POS: {
      Value *off = new_LValue(func, FILE_ADDRESS);
   bld.mkOp1(OP_RDSV, TYPE_U32, def, bld.mkSysVal(SV_SAMPLE_INDEX, 0));
   bld.mkOp2(OP_SHL, TYPE_U32, off, def, bld.mkImm(3));
   bld.mkLoad(TYPE_F32,
            def,
   bld.mkSymbol(
               }
   case SV_THREAD_KILL:
      // Not actually supported. But it's implementation-dependent, so we can
   // always just say it's not a helper.
   bld.mkMov(def, bld.loadImm(NULL, 0));
      default:
      bld.mkFetch(i->getDef(0), i->dType,
            }
   bld.getBB()->remove(i);
      }
      bool
   NV50LoweringPreSSA::handleDIV(Instruction *i)
   {
      if (!isFloatType(i->dType))
         bld.setPosition(i, false);
   Instruction *rcp = bld.mkOp1(OP_RCP, i->dType, bld.getSSA(), i->getSrc(1));
   i->op = OP_MUL;
   i->setSrc(1, rcp->getDef(0));
      }
      bool
   NV50LoweringPreSSA::handleSQRT(Instruction *i)
   {
      bld.setPosition(i, true);
   i->op = OP_RSQ;
               }
      bool
   NV50LoweringPreSSA::handleEXPORT(Instruction *i)
   {
      if (prog->getType() == Program::TYPE_FRAGMENT) {
      if (i->getIndirect(0, 0)) {
      // TODO: redirect to l[] here, load to GPRs at exit
      } else {
               i->op = OP_MOV;
   i->subOp = NV50_IR_SUBOP_MOV_FINAL;
   i->src(0).set(i->src(1));
   i->setSrc(1, NULL);
                        }
      }
      // Handle indirect addressing in geometry shaders:
   //
   // ld $r0 a[$a1][$a2+k] ->
   // ld $r0 a[($a1 + $a2 * $vstride) + k], where k *= $vstride is implicit
   //
   bool
   NV50LoweringPreSSA::handleLOAD(Instruction *i)
   {
      ValueRef src = i->src(0);
            if (prog->getType() == Program::TYPE_COMPUTE) {
      if (sym->inFile(FILE_MEMORY_SHARED) ||
      sym->inFile(FILE_MEMORY_BUFFER) ||
   sym->inFile(FILE_MEMORY_GLOBAL)) {
                  if (src.isIndirect(1)) {
      assert(prog->getType() == Program::TYPE_GEOMETRY);
            if (src.isIndirect(0)) {
      // base address is in an address register, so move to a GPR
                  Symbol *sv = bld.mkSysVal(SV_VERTEX_STRIDE, 0);
   Value *vstride = bld.mkOp1v(OP_RDSV, TYPE_U32, bld.getSSA(), sv);
                  // Calculate final address: addr = base + attr*vstride; use 16-bit
   // multiplication since 32-bit would be lowered to multiple
   // instructions, and we only need the low 16 bits of the result
   Value *a[2], *b[2];
   bld.mkSplit(a, 2, attrib);
   bld.mkSplit(b, 2, vstride);
                  // move address from GPR into an address register
   addr = bld.getSSA(2, FILE_ADDRESS);
               i->setIndirect(0, 1, NULL);
                  }
      bool
   NV50LoweringPreSSA::handleSharedATOM(Instruction *atom)
   {
               BasicBlock *currBB = atom->bb;
   BasicBlock *tryLockBB = atom->bb->splitBefore(atom, false);
   BasicBlock *joinBB = atom->bb->splitAfter(atom);
   BasicBlock *setAndUnlockBB = new BasicBlock(func);
            bld.setPosition(currBB, true);
   assert(!currBB->joinAt);
            bld.mkFlow(OP_BRA, tryLockBB, CC_ALWAYS, NULL);
                     Instruction *ld =
      bld.mkLoad(TYPE_U32, atom->getDef(0), atom->getSrc(0)->asSym(),
      Value *locked = bld.getSSA(1, FILE_FLAGS);
   if (prog->getTarget()->getChipset() >= 0xa0) {
      ld->setFlagsDef(1, locked);
      } else {
      bld.mkMov(locked, bld.loadImm(NULL, 2))
               bld.mkFlow(OP_BRA, setAndUnlockBB, CC_LT, locked);
   bld.mkFlow(OP_BRA, failLockBB, CC_ALWAYS, NULL);
   tryLockBB->cfg.attach(&failLockBB->cfg, Graph::Edge::CROSS);
            tryLockBB->cfg.detach(&joinBB->cfg);
            bld.setPosition(setAndUnlockBB, true);
   Value *stVal;
   if (atom->subOp == NV50_IR_SUBOP_ATOM_EXCH) {
      // Read the old value, and write the new one.
      } else if (atom->subOp == NV50_IR_SUBOP_ATOM_CAS) {
      CmpInstruction *set =
                  Instruction *selp =
      bld.mkOp3(OP_SELP, TYPE_U32, bld.getSSA(), atom->getSrc(2),
                  } else {
               switch (atom->subOp) {
   case NV50_IR_SUBOP_ATOM_ADD:
      op = OP_ADD;
      case NV50_IR_SUBOP_ATOM_AND:
      op = OP_AND;
      case NV50_IR_SUBOP_ATOM_OR:
      op = OP_OR;
      case NV50_IR_SUBOP_ATOM_XOR:
      op = OP_XOR;
      case NV50_IR_SUBOP_ATOM_MIN:
      op = OP_MIN;
      case NV50_IR_SUBOP_ATOM_MAX:
      op = OP_MAX;
      default:
      assert(0);
               Instruction *i =
                              Instruction *store = bld.mkStore(OP_STORE, TYPE_U32, atom->getSrc(0)->asSym(),
         if (prog->getTarget()->getChipset() >= 0xa0) {
                  bld.mkFlow(OP_BRA, failLockBB, CC_ALWAYS, NULL);
            // Loop until the lock is acquired.
   bld.setPosition(failLockBB, true);
   bld.mkFlow(OP_BRA, tryLockBB, CC_GEU, locked);
   bld.mkFlow(OP_BRA, joinBB, CC_ALWAYS, NULL);
   failLockBB->cfg.attach(&tryLockBB->cfg, Graph::Edge::BACK);
            bld.setPosition(joinBB, false);
               }
      bool
   NV50LoweringPreSSA::handleLDST(Instruction *i)
   {
      ValueRef src = i->src(0);
            if (prog->getType() != Program::TYPE_COMPUTE) {
                  // Buffers just map directly to the different global memory spaces
   if (sym->inFile(FILE_MEMORY_BUFFER)) {
                              if (src.isIndirect(0)) {
               if (!addr->inFile(FILE_ADDRESS)) {
      // Move address from GPR into an address register
                                 if (i->op == OP_ATOM)
      } else if (sym->inFile(FILE_MEMORY_GLOBAL)) {
      // All global access must be indirect. There are no instruction forms
   // with direct access.
            Value *offset = bld.loadImm(bld.getSSA(), sym->reg.data.offset);
   Value *sum;
   if (addr != NULL)
      sum = bld.mkOp2v(OP_ADD, TYPE_U32, bld.getSSA(), addr,
      else
            i->setIndirect(0, 0, sum);
                  }
      bool
   NV50LoweringPreSSA::handleMEMBAR(Instruction *i)
   {
      // For global memory, apparently doing a bunch of reads at different
   // addresses forces things to get sufficiently flushed.
   if (i->subOp & NV50_IR_SUBOP_MEMBAR_GL) {
      uint8_t b = prog->driver->io.auxCBSlot;
   Value *base =
      bld.mkLoadv(TYPE_U32, bld.mkSymbol(FILE_MEMORY_CONST, b, TYPE_U32,
      Value *physid = bld.mkOp1v(OP_RDSV, TYPE_U32, bld.getSSA(), bld.mkSysVal(SV_PHYSID, 0));
   Value *off = bld.mkOp2v(OP_SHL, TYPE_U32, bld.getSSA(),
                     base = bld.mkOp2v(OP_ADD, TYPE_U32, bld.getSSA(), base, off);
   Symbol *gmemMembar = bld.mkSymbol(FILE_MEMORY_GLOBAL, prog->driver->io.gmemMembar, TYPE_U32, 0);
   for (int i = 0; i < 8; i++) {
      if (i != 0) {
         }
   bld.mkLoad(TYPE_U32, bld.getSSA(), gmemMembar, base)
                  // Both global and shared memory barriers also need a regular control bar
   // TODO: double-check this is the case
   i->op = OP_BAR;
   i->subOp = NV50_IR_SUBOP_BAR_SYNC;
   i->setSrc(0, bld.mkImm(0u));
               }
      // The type that bests represents how each component can be stored when packed.
   static DataType
   getPackedType(const TexInstruction::ImgFormatDesc *t, int c)
   {
      switch (t->type) {
   case FLOAT: return t->bits[c] == 16 ? TYPE_F16 : TYPE_F32;
   case UNORM: return t->bits[c] == 8 ? TYPE_U8 : TYPE_U16;
   case SNORM: return t->bits[c] == 8 ? TYPE_S8 : TYPE_S16;
   case UINT:
      return (t->bits[c] == 8 ? TYPE_U8 :
      case SINT:
      return (t->bits[c] == 8 ? TYPE_S8 :
      }
      }
      // The type that the rest of the shader expects to process this image type in.
   static DataType
   getShaderType(const ImgType type) {
      switch (type) {
   case FLOAT:
   case UNORM:
   case SNORM:
         case UINT:
         case SINT:
         default:
      assert(!"Impossible type");
         }
      // Reads the raw coordinates out of the input instruction, and returns a
   // single-value coordinate which is what the hardware expects to receive in a
   // ld/st op.
   Value *
   NV50LoweringPreSSA::processSurfaceCoords(TexInstruction *su)
   {
      const int slot = su->tex.r;
   const int dim = su->tex.target.getDim();
            const TexInstruction::ImgFormatDesc *format = su->tex.format;
   const uint16_t bytes = (format->bits[0] + format->bits[1] +
                  // Buffer sizes don't necessarily fit in 16-bit values
   if (su->tex.target == TEX_TARGET_BUFFER) {
      return bld.mkOp2v(OP_SHL, TYPE_U32, bld.getSSA(),
               // For buffers, we just need the byte offset. And for 2d buffers we want
   // the x coordinate in bytes as well.
   Value *coords[3] = {};
   for (int i = 0; i < arg; i++) {
      Value *src[2];
   bld.mkSplit(src, 2, su->getSrc(i));
   coords[i] = src[0];
   // For 1d-images, we want the y coord to be 0, which it will be here.
   if (i == 0)
               coords[0] = bld.mkOp2v(OP_SHL, TYPE_U16, bld.getSSA(2),
            if (su->tex.target.isMS()) {
      Value *ms_x = loadSuInfo16(slot, NV50_SU_INFO_MS(0));
   Value *ms_y = loadSuInfo16(slot, NV50_SU_INFO_MS(1));
   coords[0] = bld.mkOp2v(OP_SHL, TYPE_U16, bld.getSSA(2), coords[0], ms_x);
               // If there are more dimensions, we just want the y-offset. But that needs
   // to be adjusted up by the y-stride for array images.
   if (su->tex.target.isArray() || su->tex.target.isCube()) {
      Value *index = coords[dim];
   Value *height = loadSuInfo16(slot, NV50_SU_INFO_STRIDE_Y);
   Instruction *mul = bld.mkOp2(OP_MUL, TYPE_U32, bld.getSSA(4), index, height);
   mul->sType = TYPE_U16;
   Value *muls[2];
   bld.mkSplit(muls, 2, mul->getDef(0));
   if (dim > 1)
         else
               // 3d is special-cased. Note that a single "slice" of a 3d image may
   // also be attached as 2d, so we have to do the same 3d processing for
   // 2d as well, just in case. In order to remap a 3d image onto a 2d
   // image, we have to retile it "by hand".
   if (su->tex.target == TEX_TARGET_3D || su->tex.target == TEX_TARGET_2D) {
      Value *z = loadSuInfo16(slot, NV50_SU_INFO_OFFSET_Z);
   Value *y_size_aligned = loadSuInfo16(slot, NV50_SU_INFO_STRIDE_Y);
   // Add the z coordinate for actual 3d-images
   if (dim > 2)
         else
            // Compute the surface parameters from tile shifts
   Value *tile_shift[3];
   Value *tile_size[3];
   Value *tile_mask[3];
   // We only ever use one kind of X-tiling.
   tile_shift[0] = bld.loadImm(NULL, (uint16_t)6);
   tile_size[0] = bld.loadImm(NULL, (uint16_t)64);
   tile_mask[0] = bld.loadImm(NULL, (uint16_t)63);
   // Fetch the "real" tiling parameters of the underlying surface
   for (int i = 1; i < 3; i++) {
      tile_shift[i] = loadSuInfo16(slot, NV50_SU_INFO_TILE_SHIFT(i));
   tile_size[i] = bld.mkOp2v(OP_SHL, TYPE_U16, bld.getSSA(2), bld.loadImm(NULL, (uint16_t)1), tile_shift[i]);
               // Compute the location of given coordinate, both inside the tile as
   // well as which (linearly-laid out) tile it's in.
   Value *coord_in_tile[3];
   Value *tile[3];
   for (int i = 0; i < 3; i++) {
      coord_in_tile[i] = bld.mkOp2v(OP_AND, TYPE_U16, bld.getSSA(2), coords[i], tile_mask[i]);
               // Based on the "real" tiling parameters, compute x/y coordinates in the
   // larger surface with 2d tiling that was supplied to the hardware. This
   // was determined and verified with the help of the tiling pseudocode in
   // the envytools docs.
   //
   // adj_x = x_coord_in_tile + x_tile * x_tile_size * z_tile_size +
   //         z_coord_in_tile * x_tile_size
   // adj_y = y_coord_in_tile + y_tile * y_tile_size +
   //         z_tile * y_tile_size * y_tiles
   //
            coords[0] = bld.mkOp2v(
         OP_ADD, TYPE_U16, bld.getSSA(2),
   bld.mkOp2v(OP_ADD, TYPE_U16, bld.getSSA(2),
            coord_in_tile[0],
   bld.mkOp2v(OP_SHL, TYPE_U16, bld.getSSA(2),
                     Instruction *mul = bld.mkOp2(OP_MUL, TYPE_U32, bld.getSSA(4),
         mul->sType = TYPE_U16;
   Value *muls[2];
            coords[1] = bld.mkOp2v(
         OP_ADD, TYPE_U16, bld.getSSA(2),
   muls[0],
   bld.mkOp2v(OP_ADD, TYPE_U16, bld.getSSA(2),
                        }
      // This is largely a copy of NVC0LoweringPass::convertSurfaceFormat, but
   // adjusted to make use of 16-bit math where possible.
   bool
   NV50LoweringPreSSA::handleSULDP(TexInstruction *su)
   {
      const int slot = su->tex.r;
                     const TexInstruction::ImgFormatDesc *format = su->tex.format;
   const int bytes = (su->tex.format->bits[0] +
                                       Value *untypedDst[4] = {};
   Value *typedDst[4] = {};
   int i;
   for (i = 0; i < bytes / 4; i++)
         if (bytes < 4)
            for (i = 0; i < 4; i++)
            Instruction *load = bld.mkLoad(ty, NULL, bld.mkSymbol(FILE_MEMORY_GLOBAL, slot, ty, 0), coord);
   for (i = 0; i < 4 && untypedDst[i]; i++)
            // Unpack each component into the typed dsts
   int bits = 0;
   for (int i = 0; i < 4; bits += format->bits[i], i++) {
      if (!typedDst[i])
            if (i >= format->components) {
      if (format->type == FLOAT ||
      format->type == UNORM ||
   format->type == SNORM)
      else
                     // Get just that component's data into the relevant place
   if (format->bits[i] == 32)
         else if (format->bits[i] == 16) {
      // We can always convert directly from the appropriate half of the
   // loaded value into the typed result.
   Value *src[2];
   bld.mkSplit(src, 2, untypedDst[i / 2]);
   bld.mkCvt(OP_CVT, getShaderType(format->type), typedDst[i],
      }
   else if (format->bits[i] == 8) {
      // Same approach as for 16 bits, but we have to massage the value a
   // bit more, since we have to get the appropriate 8 bits from the
   // half-register. In all cases, we can CVT from a 8-bit source, so we
   // only have to shift when we want the upper 8 bits.
   Value *src[2], *shifted;
   bld.mkSplit(src, 2, untypedDst[0]);
   DataType packedType = getPackedType(format, i);
   if (i & 1)
                        bld.mkCvt(OP_CVT, getShaderType(format->type), typedDst[i],
      }
   else {
      // The options are 10, 11, and 2. Get it into a 32-bit reg, then
   // shift/mask. That's where it'll have to end up anyways. For signed,
   // we have to make sure to get sign-extension, so we actually have to
   // shift *up* first, and then shift down. There's no advantage to
   // AND'ing, so we don't.
   DataType ty = TYPE_U32;
   if (format->type == SNORM || format->type == SINT) {
                  // Poor man's EXTBF
   bld.mkOp2(
                        // If the stored data is already in the appropriate type, we don't
   // have to do anything. Convert to float for the *NORM formats.
   if (format->type == UNORM || format->type == SNORM)
               // Normalize / convert as necessary
   if (format->type == UNORM)
         else if (format->type == SNORM)
         else if (format->type == FLOAT && format->bits[i] < 16) {
      // We expect the value to be in the low bits of the register, so we
   // have to shift back up.
   bld.mkOp2(OP_SHL, TYPE_U32, typedDst[i], typedDst[i], bld.loadImm(NULL, 15 - format->bits[i]));
   Value *src[2];
   bld.mkSplit(src, 2, typedDst[i]);
                  if (format->bgra) {
                  bld.getBB()->remove(su);
      }
      bool
   NV50LoweringPreSSA::handleSUREDP(TexInstruction *su)
   {
      const int slot = su->tex.r;
   const int dim = su->tex.target.getDim();
   const int arg = dim + (su->tex.target.isArray() || su->tex.target.isCube());
                              // This is guaranteed to be a 32-bit format. So there's nothing to
   // pack/unpack.
   Instruction *atom = bld.mkOp2(
         OP_ATOM, su->dType, su->getDef(0),
   if (su->subOp == NV50_IR_SUBOP_ATOM_CAS)
         atom->setIndirect(0, 0, coord);
            bld.getBB()->remove(su);
      }
      bool
   NV50LoweringPreSSA::handleSUSTP(TexInstruction *su)
   {
      const int slot = su->tex.r;
   const int dim = su->tex.target.getDim();
   const int arg = dim + (su->tex.target.isArray() || su->tex.target.isCube());
                     const TexInstruction::ImgFormatDesc *format = su->tex.format;
   const int bytes = (su->tex.format->bits[0] +
                                       // The packed values we will eventually store into memory
   Value *untypedDst[4] = {};
   // Each component's packed representation, in 16-bit registers (only used
   // where appropriate)
   Value *untypedDst16[4] = {};
   // The original values that are being packed
   Value *typedDst[4] = {};
            for (i = 0; i < bytes / 4; i++)
         for (i = 0; i < format->components; i++)
         // Make sure we get at least one of each value allocated for the
   // super-narrow formats.
   if (bytes < 4)
         if (bytes < 2)
            for (i = 0; i < 4; i++) {
      typedDst[i] = bld.getSSA();
               if (format->bgra) {
                  // Pack each component into the untyped dsts.
   int bits = 0;
   for (int i = 0; i < format->components; bits += format->bits[i], i++) {
      // Un-normalize / convert as necessary
   if (format->type == UNORM)
         else if (format->type == SNORM)
            // There is nothing to convert/pack for 32-bit values
   if (format->bits[i] == 32) {
      bld.mkMov(untypedDst[i], typedDst[i]);
               // The remainder of the cases will naturally want to deal in 16-bit
   // registers. We will put these into untypedDst16 and then merge them
   // together later.
   if (format->type == FLOAT && format->bits[i] < 16) {
                     // For odd bit sizes, it's easier to pack it into the final
   // destination directly.
   Value *tmp = bld.getSSA();
   bld.mkCvt(OP_CVT, TYPE_U32, tmp, TYPE_U16, untypedDst16[i]);
   if (i == 0) {
         } else {
      bld.mkOp2(OP_SHL, TYPE_U32, tmp, tmp, bld.loadImm(NULL, bits));
         } else if (format->bits[i] == 16) {
      // We can always convert the shader value into the packed value
   // directly here
   bld.mkCvt(OP_CVT, getPackedType(format, i), untypedDst16[i],
      } else if (format->bits[i] < 16) {
      DataType packedType = getPackedType(format, i);
   DataType shaderType = getShaderType(format->type);
   // We can't convert F32 to U8/S8 directly, so go to U16/S16 first.
   if (shaderType == TYPE_F32 && typeSizeof(packedType) == 1) {
         }
   bld.mkCvt(OP_CVT, packedType, untypedDst16[i], shaderType, typedDst[i]);
   // TODO: clamp for 10- and 2-bit sizes. Also, due to the oddness of
   // the size, it's easier to dump them into a 32-bit value and OR
   // everything later.
   if (format->bits[i] != 8) {
      // Restrict value to the appropriate bits (although maybe supposed
   // to clamp instead?)
   bld.mkOp2(OP_AND, TYPE_U16, untypedDst16[i], untypedDst16[i], bld.loadImm(NULL, (uint16_t)((1 << format->bits[i]) - 1)));
   // And merge into final packed value
   Value *tmp = bld.getSSA();
   bld.mkCvt(OP_CVT, TYPE_U32, tmp, TYPE_U16, untypedDst16[i]);
   if (i == 0) {
         } else {
      bld.mkOp2(OP_SHL, TYPE_U32, tmp, tmp, bld.loadImm(NULL, bits));
         } else if (i & 1) {
      // Shift the 8-bit value up (so that it can be OR'd later)
      } else if (packedType != TYPE_U8) {
      // S8 (or the *16 if converted from float) will all have high bits
   // set, so AND them out.
                     // OR pairs of 8-bit values together (into the even value)
   if (format->bits[0] == 8) {
      for (i = 0; i < 2 && untypedDst16[2 * i] && untypedDst16[2 * i + 1]; i++)
               // We'll always want to have at least a 32-bit source register for the store
   Instruction *merge = bld.mkOp(OP_MERGE, bytes < 4 ? TYPE_U32 : ty, bld.getSSA(bytes < 4 ? 4 : bytes));
   if (format->bits[0] == 32) {
      for (i = 0; i < 4 && untypedDst[i]; i++)
      } else if (format->bits[0] == 16) {
      for (i = 0; i < 4 && untypedDst16[i]; i++)
         if (i == 1)
      } else if (format->bits[0] == 8) {
      for (i = 0; i < 2 && untypedDst16[2 * i]; i++)
         if (i == 1)
      } else {
                           bld.getBB()->remove(su);
      }
      bool
   NV50LoweringPreSSA::handlePFETCH(Instruction *i)
   {
               // NOTE: cannot use getImmediate here, not in SSA form yet, move to
            ImmediateValue *imm = i->getSrc(0)->asImm();
                     if (i->srcExists(1)) {
               LValue *val = bld.getScratch();
   Value *ptr = bld.getSSA(2, FILE_ADDRESS);
   bld.mkOp2v(OP_SHL, TYPE_U32, ptr, i->getSrc(1), bld.mkImm(2));
            // NOTE: PFETCH directly to an $aX only works with direct addressing
   i->op = OP_SHL;
   i->setSrc(0, val);
                  }
      // Set flags according to predicate and make the instruction read $cX.
   void
   NV50LoweringPreSSA::checkPredicate(Instruction *insn)
   {
      Value *pred = insn->getPredicate();
            // FILE_PREDICATE will simply be changed to FLAGS on conversion to SSA
   if (!pred ||
      pred->reg.file == FILE_FLAGS || pred->reg.file == FILE_PREDICATE)
                              }
      //
   // - add quadop dance for texturing
   // - put FP outputs in GPRs
   // - convert instruction sequences
   //
   bool
   NV50LoweringPreSSA::visit(Instruction *i)
   {
               if (i->cc != CC_ALWAYS)
            switch (i->op) {
   case OP_TEX:
   case OP_TXF:
   case OP_TXG:
         case OP_TXB:
         case OP_TXL:
         case OP_TXD:
         case OP_TXLQ:
         case OP_TXQ:
         case OP_EX2:
      bld.mkOp1(OP_PREEX2, TYPE_F32, i->getDef(0), i->getSrc(0));
   i->setSrc(0, i->getDef(0));
      case OP_SET:
         case OP_SLCT:
         case OP_SELP:
         case OP_DIV:
         case OP_SQRT:
         case OP_EXPORT:
         case OP_LOAD:
         case OP_MEMBAR:
         case OP_ATOM:
   case OP_STORE:
         case OP_SULDP:
         case OP_SUSTP:
         case OP_SUREDP:
         case OP_SUQ:
         case OP_BUFQ:
         case OP_RDSV:
         case OP_CALL:
         case OP_PRECONT:
         case OP_CONT:
         case OP_PFETCH:
         default:
         }
      }
      bool
   TargetNV50::runLegalizePass(Program *prog, CGStage stage) const
   {
               if (stage == CG_STAGE_PRE_SSA) {
      NV50LoweringPreSSA pass(prog);
      } else
   if (stage == CG_STAGE_SSA) {
      if (!prog->targetPriv)
         NV50LegalizeSSA pass(prog);
      } else
   if (stage == CG_STAGE_POST_RA) {
      NV50LegalizePostRA pass;
   ret = pass.run(prog, false, true);
   if (prog->targetPriv)
      }
      }
      } // namespace nv50_ir
