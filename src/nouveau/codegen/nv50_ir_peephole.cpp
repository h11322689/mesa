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
   #include "nv50_ir_target.h"
   #include "nv50_ir_build_util.h"
      extern "C" {
   #include "util/u_math.h"
   }
      namespace nv50_ir {
      bool
   Instruction::isNop() const
   {
      if (op == OP_PHI || op == OP_SPLIT || op == OP_MERGE)
         if (terminator || join) // XXX: should terminator imply flow ?
         if (op == OP_ATOM)
         if (!fixed && op == OP_NOP)
            if (defExists(0) && def(0).rep()->reg.data.id < 0) {
      for (int d = 1; defExists(d); ++d)
      if (def(d).rep()->reg.data.id >= 0)
                  if (op == OP_MOV || op == OP_UNION) {
      if (!getDef(0)->equals(getSrc(0)))
         if (op == OP_UNION)
      if (!getDef(0)->equals(getSrc(1)))
                     }
      bool Instruction::isDead() const
   {
      if (op == OP_STORE ||
      op == OP_EXPORT ||
   op == OP_ATOM ||
   op == OP_SUSTB || op == OP_SUSTP || op == OP_SUREDP || op == OP_SUREDB)
         for (int d = 0; defExists(d); ++d)
      if (getDef(d)->refCount() || getDef(d)->reg.data.id >= 0)
         if (terminator || asFlow())
         if (fixed)
               };
      // =============================================================================
      class CopyPropagation : public Pass
   {
   private:
         };
      // Propagate all MOVs forward to make subsequent optimization easier, except if
   // the sources stem from a phi, in which case we don't want to mess up potential
   // swaps $rX <-> $rY, i.e. do not create live range overlaps of phi src and def.
   bool
   CopyPropagation::visit(BasicBlock *bb)
   {
               for (mov = bb->getEntry(); mov; mov = next) {
      next = mov->next;
   if (mov->op != OP_MOV || mov->fixed || !mov->getSrc(0)->asLValue())
         if (mov->getPredicate())
         if (mov->def(0).getFile() != mov->src(0).getFile())
         si = mov->getSrc(0)->getInsn();
   if (mov->getDef(0)->reg.data.id < 0 && si && si->op != OP_PHI) {
      // propagate
   mov->def(0).replace(mov->getSrc(0), false);
         }
      }
      // =============================================================================
      class MergeSplits : public Pass
   {
   private:
         };
      // For SPLIT / MERGE pairs that operate on the same registers, replace the
   // post-merge def with the SPLIT's source.
   bool
   MergeSplits::visit(BasicBlock *bb)
   {
               for (i = bb->getEntry(); i; i = next) {
      next = i->next;
   if (i->op != OP_MERGE || typeSizeof(i->dType) != 8)
         si = i->getSrc(0)->getInsn();
   if (si->op != OP_SPLIT || si != i->getSrc(1)->getInsn())
         i->def(0).replace(si->getSrc(0), false);
                  }
      // =============================================================================
      class LoadPropagation : public Pass
   {
   private:
                        bool isCSpaceLoad(Instruction *);
   bool isImmdLoad(Instruction *);
      };
      bool
   LoadPropagation::isCSpaceLoad(Instruction *ld)
   {
         }
      bool
   LoadPropagation::isImmdLoad(Instruction *ld)
   {
      if (!ld || (ld->op != OP_MOV) ||
      ((typeSizeof(ld->dType) != 4) && (typeSizeof(ld->dType) != 8)))
         // A 0 can be replaced with a register, so it doesn't count as an immediate.
   ImmediateValue val;
      }
      bool
   LoadPropagation::isAttribOrSharedLoad(Instruction *ld)
   {
      return ld &&
      (ld->op == OP_VFETCH ||
   (ld->op == OP_LOAD &&
   (ld->src(0).getFile() == FILE_SHADER_INPUT ||
   }
      void
   LoadPropagation::checkSwapSrc01(Instruction *insn)
   {
      const Target *targ = prog->getTarget();
   if (!targ->getOpInfo(insn).commutative) {
      if (insn->op != OP_SET && insn->op != OP_SLCT &&
      insn->op != OP_SUB && insn->op != OP_XMAD)
      // XMAD is only commutative if both the CBCC and MRG flags are not set.
   if (insn->op == OP_XMAD &&
      (insn->subOp & NV50_IR_SUBOP_XMAD_CMODE_MASK) == NV50_IR_SUBOP_XMAD_CBCC)
      if (insn->op == OP_XMAD && (insn->subOp & NV50_IR_SUBOP_XMAD_MRG))
      }
   if (insn->src(1).getFile() != FILE_GPR)
         // This is the special OP_SET used for alphatesting, we can't reverse its
   // arguments as that will confuse the fixup code.
   if (insn->op == OP_SET && insn->subOp)
            Instruction *i0 = insn->getSrc(0)->getInsn();
            // Swap sources to inline the less frequently used source. That way,
   // optimistically, it will eventually be able to remove the instruction.
   int i0refs = insn->getSrc(0)->refCount();
            if ((isCSpaceLoad(i0) || isImmdLoad(i0)) && targ->insnCanLoad(insn, 1, i0)) {
      if ((!isImmdLoad(i1) && !isCSpaceLoad(i1)) ||
      !targ->insnCanLoad(insn, 1, i1) ||
   i0refs < i1refs)
      else
      } else
   if (isAttribOrSharedLoad(i1)) {
      if (!isAttribOrSharedLoad(i0))
         else
      } else {
                  if (insn->op == OP_SET || insn->op == OP_SET_AND ||
      insn->op == OP_SET_OR || insn->op == OP_SET_XOR)
      else
   if (insn->op == OP_SLCT)
         else
   if (insn->op == OP_SUB) {
      insn->src(0).mod = insn->src(0).mod ^ Modifier(NV50_IR_MOD_NEG);
      } else
   if (insn->op == OP_XMAD) {
      // swap h1 flags
   uint16_t h1 = (insn->subOp >> 1 & NV50_IR_SUBOP_XMAD_H1(0)) |
               }
      bool
   LoadPropagation::visit(BasicBlock *bb)
   {
      const Target *targ = prog->getTarget();
            for (Instruction *i = bb->getEntry(); i; i = next) {
               if (i->op == OP_CALL) // calls have args as sources, they must be in regs
            if (i->op == OP_PFETCH) // pfetch expects arg1 to be a reg
            if (i->srcExists(1))
            for (int s = 0; i->srcExists(s); ++s) {
               if (!ld || ld->fixed || (ld->op != OP_LOAD && ld->op != OP_MOV))
         if (ld->op == OP_LOAD && ld->subOp == NV50_IR_SUBOP_LOAD_LOCKED)
                        // propagate !
   i->setSrc(s, ld->getSrc(0));
                  if (ld->getDef(0)->refCount() == 0)
         }
      }
      // =============================================================================
      class IndirectPropagation : public Pass
   {
   private:
                  };
      bool
   IndirectPropagation::visit(BasicBlock *bb)
   {
      const Target *targ = prog->getTarget();
            for (Instruction *i = bb->getEntry(); i; i = next) {
                        for (int s = 0; i->srcExists(s); ++s) {
      Instruction *insn;
   ImmediateValue imm;
   if (!i->src(s).isIndirect(0))
         insn = i->getIndirect(s, 0)->getInsn();
   if (!insn)
         if (insn->op == OP_ADD && !isFloatType(insn->dType)) {
      if (insn->src(0).getFile() != targ->nativeFile(FILE_ADDRESS) ||
      !insn->src(1).getImmediate(imm) ||
   !targ->insnCanLoadOffset(i, s, imm.reg.data.s32))
      i->setIndirect(s, 0, insn->getSrc(0));
   i->setSrc(s, cloneShallow(func, i->getSrc(s)));
      } else if (insn->op == OP_SUB && !isFloatType(insn->dType)) {
      if (insn->src(0).getFile() != targ->nativeFile(FILE_ADDRESS) ||
      !insn->src(1).getImmediate(imm) ||
   !targ->insnCanLoadOffset(i, s, -imm.reg.data.s32))
      i->setIndirect(s, 0, insn->getSrc(0));
   i->setSrc(s, cloneShallow(func, i->getSrc(s)));
      } else if (insn->op == OP_MOV) {
      if (!insn->src(0).getImmediate(imm) ||
      !targ->insnCanLoadOffset(i, s, imm.reg.data.s32))
      i->setIndirect(s, 0, NULL);
   i->setSrc(s, cloneShallow(func, i->getSrc(s)));
      } else if (insn->op == OP_SHLADD) {
      if (!insn->src(2).getImmediate(imm) ||
      !targ->insnCanLoadOffset(i, s, imm.reg.data.s32))
      i->setIndirect(s, 0, bld.mkOp2v(
         i->setSrc(s, cloneShallow(func, i->getSrc(s)));
            }
      }
      // =============================================================================
      // Evaluate constant expressions.
   class ConstantFolding : public Pass
   {
   public:
      ConstantFolding() : foldCount(0) {}
         private:
               void expr(Instruction *, ImmediateValue&, ImmediateValue&);
   void expr(Instruction *, ImmediateValue&, ImmediateValue&, ImmediateValue&);
   /* true if i was deleted */
   bool opnd(Instruction *i, ImmediateValue&, int s);
                                                            };
      // TODO: remember generated immediates and only revisit these
   bool
   ConstantFolding::foldAll(Program *prog)
   {
      unsigned int iterCount = 0;
   do {
      foldCount = 0;
   if (!run(prog))
      } while (foldCount && ++iterCount < 2);
      }
      bool
   ConstantFolding::visit(BasicBlock *bb)
   {
               for (i = bb->getEntry(); i; i = next) {
      next = i->next;
   if (i->op == OP_MOV || i->op == OP_CALL)
                     if (i->srcExists(2) &&
      i->src(0).getImmediate(src0) &&
   i->src(1).getImmediate(src1) &&
   i->src(2).getImmediate(src2)) {
      } else
   if (i->srcExists(1) &&
      i->src(0).getImmediate(src0) && i->src(1).getImmediate(src1)) {
      } else
   if (i->srcExists(0) && i->src(0).getImmediate(src0)) {
      if (opnd(i, src0, 0))
      } else
   if (i->srcExists(1) && i->src(1).getImmediate(src1)) {
      if (opnd(i, src1, 1))
      }
   if (i->srcExists(2) && i->src(2).getImmediate(src2))
      }
      }
      CmpInstruction *
   ConstantFolding::findOriginForTestWithZero(Value *value)
   {
      if (!value)
         Instruction *insn = value->getInsn();
   if (!insn)
            if (insn->asCmp() && insn->op != OP_SLCT)
            /* Sometimes mov's will sneak in as a result of other folding. This gets
   * cleaned up later.
   */
   if (insn->op == OP_MOV)
            /* Deal with AND 1.0 here since nv50 can't fold into boolean float */
   if (insn->op == OP_AND) {
      int s = 0;
   ImmediateValue imm;
   if (!insn->src(s).getImmediate(imm)) {
      s = 1;
   if (!insn->src(s).getImmediate(imm))
      }
   if (imm.reg.data.f32 != 1.0f)
         /* TODO: Come up with a way to handle the condition being inverted */
   if (insn->src(!s).mod != Modifier(0))
                        }
      void
   Modifier::applyTo(ImmediateValue& imm) const
   {
      if (!bits) // avoid failure if imm.reg.type is unhandled (e.g. b128)
         switch (imm.reg.type) {
   case TYPE_F32:
      if (bits & NV50_IR_MOD_ABS)
         if (bits & NV50_IR_MOD_NEG)
         if (bits & NV50_IR_MOD_SAT) {
      if (imm.reg.data.f32 < 0.0f)
         else
   if (imm.reg.data.f32 > 1.0f)
      }
   assert(!(bits & NV50_IR_MOD_NOT));
         case TYPE_S8: // NOTE: will be extended
   case TYPE_S16:
   case TYPE_S32:
   case TYPE_U8: // NOTE: treated as signed
   case TYPE_U16:
   case TYPE_U32:
      if (bits & NV50_IR_MOD_ABS)
      imm.reg.data.s32 = (imm.reg.data.s32 >= 0) ?
      if (bits & NV50_IR_MOD_NEG)
         if (bits & NV50_IR_MOD_NOT)
               case TYPE_F64:
      if (bits & NV50_IR_MOD_ABS)
         if (bits & NV50_IR_MOD_NEG)
         if (bits & NV50_IR_MOD_SAT) {
      if (imm.reg.data.f64 < 0.0)
         else
   if (imm.reg.data.f64 > 1.0)
      }
   assert(!(bits & NV50_IR_MOD_NOT));
         default:
      assert(!"invalid/unhandled type");
   imm.reg.data.u64 = 0;
         }
      operation
   Modifier::getOp() const
   {
      switch (bits) {
   case NV50_IR_MOD_ABS: return OP_ABS;
   case NV50_IR_MOD_NEG: return OP_NEG;
   case NV50_IR_MOD_SAT: return OP_SAT;
   case NV50_IR_MOD_NOT: return OP_NOT;
   case 0:
         default:
            }
      void
   ConstantFolding::expr(Instruction *i,
         {
      struct Storage *const a = &imm0.reg, *const b = &imm1.reg;
   struct Storage res;
                     switch (i->op) {
   case OP_SGXT: {
      int bits = b->data.u32;
   if (bits) {
      uint32_t data = a->data.u32 & (0xffffffff >> (32 - bits));
   if (bits < 32 && (data & (1 << (bits - 1))))
            }
      }
   case OP_BMSK:
      res.data.u32 = ((1 << b->data.u32) - 1) << a->data.u32;
      case OP_MAD:
   case OP_FMA:
   case OP_MUL:
      if (i->dnz && i->dType == TYPE_F32) {
      if (!isfinite(a->data.f32))
         if (!isfinite(b->data.f32))
      }
   switch (i->dType) {
   case TYPE_F32:
      res.data.f32 = a->data.f32 * b->data.f32 * exp2f(i->postFactor);
      case TYPE_F64: res.data.f64 = a->data.f64 * b->data.f64; break;
   case TYPE_S32:
      if (i->subOp == NV50_IR_SUBOP_MUL_HIGH) {
      res.data.s32 = ((int64_t)a->data.s32 * b->data.s32) >> 32;
      }
      case TYPE_U32:
      if (i->subOp == NV50_IR_SUBOP_MUL_HIGH) {
      res.data.u32 = ((uint64_t)a->data.u32 * b->data.u32) >> 32;
      }
      default:
         }
      case OP_DIV:
      if (b->data.u32 == 0)
         switch (i->dType) {
   case TYPE_F32: res.data.f32 = a->data.f32 / b->data.f32; break;
   case TYPE_F64: res.data.f64 = a->data.f64 / b->data.f64; break;
   case TYPE_S32: res.data.s32 = a->data.s32 / b->data.s32; break;
   case TYPE_U32: res.data.u32 = a->data.u32 / b->data.u32; break;
   default:
         }
      case OP_ADD:
      switch (i->dType) {
   case TYPE_F32: res.data.f32 = a->data.f32 + b->data.f32; break;
   case TYPE_F64: res.data.f64 = a->data.f64 + b->data.f64; break;
   case TYPE_S32:
   case TYPE_U32: res.data.u32 = a->data.u32 + b->data.u32; break;
   default:
         }
      case OP_SUB:
      switch (i->dType) {
   case TYPE_F32: res.data.f32 = a->data.f32 - b->data.f32; break;
   case TYPE_F64: res.data.f64 = a->data.f64 - b->data.f64; break;
   case TYPE_S32:
   case TYPE_U32: res.data.u32 = a->data.u32 - b->data.u32; break;
   default:
         }
      case OP_MAX:
      switch (i->dType) {
   case TYPE_F32: res.data.f32 = MAX2(a->data.f32, b->data.f32); break;
   case TYPE_F64: res.data.f64 = MAX2(a->data.f64, b->data.f64); break;
   case TYPE_S32: res.data.s32 = MAX2(a->data.s32, b->data.s32); break;
   case TYPE_U32: res.data.u32 = MAX2(a->data.u32, b->data.u32); break;
   default:
         }
      case OP_MIN:
      switch (i->dType) {
   case TYPE_F32: res.data.f32 = MIN2(a->data.f32, b->data.f32); break;
   case TYPE_F64: res.data.f64 = MIN2(a->data.f64, b->data.f64); break;
   case TYPE_S32: res.data.s32 = MIN2(a->data.s32, b->data.s32); break;
   case TYPE_U32: res.data.u32 = MIN2(a->data.u32, b->data.u32); break;
   default:
         }
      case OP_AND:
      res.data.u64 = a->data.u64 & b->data.u64;
      case OP_OR:
      res.data.u64 = a->data.u64 | b->data.u64;
      case OP_XOR:
      res.data.u64 = a->data.u64 ^ b->data.u64;
      case OP_SHL:
      res.data.u32 = a->data.u32 << b->data.u32;
      case OP_SHR:
      switch (i->dType) {
   case TYPE_S32: res.data.s32 = a->data.s32 >> b->data.u32; break;
   case TYPE_U32: res.data.u32 = a->data.u32 >> b->data.u32; break;
   default:
         }
      case OP_SLCT:
      if (a->data.u32 != b->data.u32)
         res.data.u32 = a->data.u32;
      case OP_EXTBF: {
      int offset = b->data.u32 & 0xff;
   int width = (b->data.u32 >> 8) & 0xff;
   int rshift = offset;
   int lshift = 0;
   if (width == 0) {
      res.data.u32 = 0;
      }
   if (width + offset < 32) {
      rshift = 32 - width;
      }
   if (i->subOp == NV50_IR_SUBOP_EXTBF_REV)
         else
         switch (i->dType) {
   case TYPE_S32: res.data.s32 = (res.data.s32 << lshift) >> rshift; break;
   case TYPE_U32: res.data.u32 = (res.data.u32 << lshift) >> rshift; break;
   default:
         }
      }
   case OP_POPCNT:
      res.data.u32 = util_bitcount(a->data.u32 & b->data.u32);
      case OP_PFETCH:
      // The two arguments to pfetch are logically added together. Normally
   // the second argument will not be constant, but that can happen.
   res.data.u32 = a->data.u32 + b->data.u32;
   type = TYPE_U32;
      case OP_MERGE:
      switch (i->dType) {
   case TYPE_U64:
   case TYPE_S64:
   case TYPE_F64:
      res.data.u64 = (((uint64_t)b->data.u32) << 32) | a->data.u32;
      default:
         }
      default:
         }
            i->src(0).mod = Modifier(0);
   i->src(1).mod = Modifier(0);
            i->setSrc(0, new_ImmediateValue(i->bb->getProgram(), res.data.u32));
            i->getSrc(0)->reg.data = res.data;
   i->getSrc(0)->reg.type = type;
            switch (i->op) {
   case OP_MAD:
   case OP_FMA: {
      ImmediateValue src0, src1;
            // Move the immediate into position 1, where we know it might be
   // emittable. However it might not be anyways, as there may be other
   // restrictions, so move it into a separate LValue.
   bld.setPosition(i, false);
   i->op = OP_ADD;
   i->dnz = 0;
   i->setSrc(1, bld.mkMov(bld.getSSA(type), i->getSrc(0), type)->getDef(0));
   i->setSrc(0, i->getSrc(2));
   i->src(0).mod = i->src(2).mod;
            if (i->src(0).getImmediate(src0))
         else
            }
   case OP_PFETCH:
      // Leave PFETCH alone... we just folded its 2 args into 1.
      default:
      i->op = i->saturate ? OP_SAT : OP_MOV;
   if (i->saturate)
            }
      }
      void
   ConstantFolding::expr(Instruction *i,
                     {
      struct Storage *const a = &imm0.reg, *const b = &imm1.reg, *const c = &imm2.reg;
                     switch (i->op) {
   case OP_LOP3_LUT:
      for (int n = 0; n < 32; n++) {
      uint8_t lut = ((a->data.u32 >> n) & 1) << 2 |
                  }
      case OP_PERMT:
      if (!i->subOp) {
      uint64_t input = (uint64_t)c->data.u32 << 32 | a->data.u32;
   uint16_t permt = b->data.u32;
   for (int n = 0 ; n < 4; n++, permt >>= 4)
      } else
            case OP_INSBF: {
      int offset = b->data.u32 & 0xff;
   int width = (b->data.u32 >> 8) & 0xff;
   unsigned bitmask = ((1 << width) - 1) << offset;
   res.data.u32 = ((a->data.u32 << offset) & bitmask) | (c->data.u32 & ~bitmask);
      }
   case OP_MAD:
   case OP_FMA: {
      switch (i->dType) {
   case TYPE_F32:
      res.data.f32 = a->data.f32 * b->data.f32 * exp2f(i->postFactor) +
            case TYPE_F64:
      res.data.f64 = a->data.f64 * b->data.f64 + c->data.f64;
      case TYPE_S32:
      if (i->subOp == NV50_IR_SUBOP_MUL_HIGH) {
      res.data.s32 = ((int64_t)a->data.s32 * b->data.s32 >> 32) + c->data.s32;
      }
      case TYPE_U32:
      if (i->subOp == NV50_IR_SUBOP_MUL_HIGH) {
      res.data.u32 = ((uint64_t)a->data.u32 * b->data.u32 >> 32) + c->data.u32;
      }
   res.data.u32 = a->data.u32 * b->data.u32 + c->data.u32;
      default:
         }
      }
   case OP_SHLADD:
      res.data.u32 = (a->data.u32 << b->data.u32) + c->data.u32;
      default:
                  ++foldCount;
   i->src(0).mod = Modifier(0);
   i->src(1).mod = Modifier(0);
            i->setSrc(0, new_ImmediateValue(i->bb->getProgram(), res.data.u32));
   i->setSrc(1, NULL);
            i->getSrc(0)->reg.data = res.data;
   i->getSrc(0)->reg.type = i->dType;
               }
      void
   ConstantFolding::unary(Instruction *i, const ImmediateValue &imm)
   {
               if (i->dType != TYPE_F32)
         switch (i->op) {
   case OP_NEG: res.data.f32 = -imm.reg.data.f32; break;
   case OP_ABS: res.data.f32 = fabsf(imm.reg.data.f32); break;
   case OP_SAT: res.data.f32 = SATURATE(imm.reg.data.f32); break;
   case OP_RCP: res.data.f32 = 1.0f / imm.reg.data.f32; break;
   case OP_RSQ: res.data.f32 = 1.0f / sqrtf(imm.reg.data.f32); break;
   case OP_LG2: res.data.f32 = log2f(imm.reg.data.f32); break;
   case OP_EX2: res.data.f32 = exp2f(imm.reg.data.f32); break;
   case OP_SIN: res.data.f32 = sinf(imm.reg.data.f32); break;
   case OP_COS: res.data.f32 = cosf(imm.reg.data.f32); break;
   case OP_SQRT: res.data.f32 = sqrtf(imm.reg.data.f32); break;
   case OP_PRESIN:
   case OP_PREEX2:
      // these should be handled in subsequent OP_SIN/COS/EX2
   res.data.f32 = imm.reg.data.f32;
      default:
         }
   i->op = OP_MOV;
   i->setSrc(0, new_ImmediateValue(i->bb->getProgram(), res.data.f32));
      }
      void
   ConstantFolding::tryCollapseChainedMULs(Instruction *mul2,
         {
      const int t = s ? 0 : 1;
   Instruction *insn;
   Instruction *mul1 = NULL; // mul1 before mul2
   int e = 0;
   float f = imm2.reg.data.f32 * exp2f(mul2->postFactor);
                     if (mul2->getSrc(t)->refCount() == 1) {
      insn = mul2->getSrc(t)->getInsn();
   if (!mul2->src(t).mod && insn->op == OP_MUL && insn->dType == TYPE_F32)
         if (mul1 && !mul1->saturate) {
               if (mul1->src(s1 = 0).getImmediate(imm1) ||
      mul1->src(s1 = 1).getImmediate(imm1)) {
   bld.setPosition(mul1, false);
   // a = mul r, imm1
   // d = mul a, imm2 -> d = mul r, (imm1 * imm2)
   mul1->setSrc(s1, bld.loadImm(NULL, f * imm1.reg.data.f32));
   mul1->src(s1).mod = Modifier(0);
   mul2->def(0).replace(mul1->getDef(0), false);
      } else
   if (prog->getTarget()->isPostMultiplySupported(OP_MUL, f, e)) {
      // c = mul a, b
   // d = mul c, imm   -> d = mul_x_imm a, b
   mul1->postFactor = e;
   mul2->def(0).replace(mul1->getDef(0), false);
   if (f < 0)
            }
         }
   if (mul2->getDef(0)->refCount() == 1 && !mul2->saturate) {
      // b = mul a, imm
   // d = mul b, c   -> d = mul_x_imm a, c
   int s2, t2;
   insn = (*mul2->getDef(0)->uses.begin())->getInsn();
   if (!insn)
         mul1 = mul2;
   mul2 = NULL;
   s2 = insn->getSrc(0) == mul1->getDef(0) ? 0 : 1;
   t2 = s2 ? 0 : 1;
   if (insn->op == OP_MUL && insn->dType == TYPE_F32)
      if (!insn->src(s2).mod && !insn->src(t2).getImmediate(imm1))
      if (mul2 && prog->getTarget()->isPostMultiplySupported(OP_MUL, f, e)) {
      mul2->postFactor = e;
   mul2->setSrc(s2, mul1->src(t));
   if (f < 0)
            }
      void
   ConstantFolding::opnd3(Instruction *i, ImmediateValue &imm2)
   {
      switch (i->op) {
   case OP_MAD:
   case OP_FMA:
      if (imm2.isInteger(0)) {
      i->op = OP_MUL;
   i->setSrc(2, NULL);
   foldCount++;
      }
      case OP_SHLADD:
      if (imm2.isInteger(0)) {
      i->op = OP_SHL;
   i->setSrc(2, NULL);
   foldCount++;
      }
      default:
            }
      bool
   ConstantFolding::createMul(DataType ty, Value *def, Value *a, int64_t b, Value *c)
   {
      const Target *target = prog->getTarget();
            //a * (2^shl) -> a << shl
   if (b >= 0 && util_is_power_of_two_or_zero64(b)) {
               Value *res = c ? bld.getSSA(typeSizeof(ty)) : def;
   bld.mkOp2(OP_SHL, ty, res, a, bld.mkImm(shl));
   if (c)
                        //a * (2^shl + 1) -> a << shl + a
   //a * -(2^shl + 1) -> -a << shl + a
   //a * (2^shl - 1) -> a << shl - a
   //a * -(2^shl - 1) -> -a << shl - a
   if (typeSizeof(ty) == 4 &&
      (util_is_power_of_two_or_zero64(absB - 1) ||
   util_is_power_of_two_or_zero64(absB + 1)) &&
   target->isOpSupported(OP_SHLADD, TYPE_U32)) {
   bool subA = util_is_power_of_two_or_zero64(absB + 1);
            Value *res = c ? bld.getSSA() : def;
   Instruction *insn = bld.mkOp3(OP_SHLADD, TYPE_U32, res, a, bld.mkImm(shl), a);
   if (b < 0)
         if (subA)
            if (c)
                        if (typeSizeof(ty) == 4 && b >= 0 && b <= 0xffff &&
      target->isOpSupported(OP_XMAD, TYPE_U32)) {
   Value *tmp = bld.mkOp3v(OP_XMAD, TYPE_U32, bld.getSSA(),
         bld.mkOp3(OP_XMAD, TYPE_U32, def, a, bld.mkImm((uint32_t)b), tmp)->subOp =
                           }
      bool
   ConstantFolding::opnd(Instruction *i, ImmediateValue &imm0, int s)
   {
      const int t = !s;
   const operation op = i->op;
   Instruction *newi = i;
            switch (i->op) {
   case OP_SPLIT: {
               uint8_t size = i->getDef(0)->reg.size;
   uint8_t bitsize = size * 8;
   uint32_t mask = (1ULL << bitsize) - 1;
            uint64_t val = imm0.reg.data.u64;
   for (int8_t d = 0; i->defExists(d); ++d) {
                     newi = bld.mkMov(def, bld.mkImm((uint32_t)(val & mask)),
            }
   delete_Instruction(prog, i);
   deleted = true;
      }
   case OP_MUL:
      if (i->dType == TYPE_F32 && !i->precise)
            if (i->subOp == NV50_IR_SUBOP_MUL_HIGH) {
      assert(!isFloatType(i->sType));
   if (imm0.isInteger(1) && i->dType == TYPE_S32) {
      bld.setPosition(i, false);
   // Need to set to the sign value, which is a compare.
   newi = bld.mkCmp(OP_SET, CC_LT, TYPE_S32, i->getDef(0),
         delete_Instruction(prog, i);
      } else if (imm0.isInteger(0) || imm0.isInteger(1)) {
      // The high bits can't be set in this case (either mul by 0 or
   // unsigned by 1)
   i->op = OP_MOV;
   i->subOp = 0;
   i->setSrc(0, new_ImmediateValue(prog, 0u));
   i->src(0).mod = Modifier(0);
      } else if (!imm0.isNegative() && imm0.isPow2()) {
      // Translate into a shift
   imm0.applyLog2();
   i->op = OP_SHR;
   i->subOp = 0;
   imm0.reg.data.u32 = 32 - imm0.reg.data.u32;
   i->setSrc(0, i->getSrc(t));
   i->src(0).mod = i->src(t).mod;
   i->setSrc(1, new_ImmediateValue(prog, imm0.reg.data.u32));
         } else
   if (imm0.isInteger(0)) {
      i->dnz = 0;
   i->op = OP_MOV;
   i->setSrc(0, new_ImmediateValue(prog, 0u));
   i->src(0).mod = Modifier(0);
   i->postFactor = 0;
      } else
   if (!i->postFactor && (imm0.isInteger(1) || imm0.isInteger(-1))) {
      if (imm0.isNegative())
         i->dnz = 0;
   i->op = i->src(t).mod.getOp();
   if (s == 0) {
      i->setSrc(0, i->getSrc(1));
   i->src(0).mod = i->src(1).mod;
      }
   if (i->op != OP_CVT)
            } else
   if (!i->postFactor && (imm0.isInteger(2) || imm0.isInteger(-2))) {
      if (imm0.isNegative())
         i->op = OP_ADD;
   i->dnz = 0;
   i->setSrc(s, i->getSrc(t));
      } else
   if (!isFloatType(i->dType) && !i->src(t).mod) {
      bld.setPosition(i, false);
   int64_t b = typeSizeof(i->dType) == 8 ? imm0.reg.data.s64 : imm0.reg.data.s32;
   if (createMul(i->dType, i->getDef(0), i->getSrc(t), b, NULL)) {
      delete_Instruction(prog, i);
         } else
   if (i->postFactor && i->sType == TYPE_F32) {
      /* Can't emit a postfactor with an immediate, have to fold it in */
   i->setSrc(s, new_ImmediateValue(
            }
      case OP_FMA:
   case OP_MAD:
      if (imm0.isInteger(0)) {
      i->setSrc(0, i->getSrc(2));
   i->src(0).mod = i->src(2).mod;
   i->setSrc(1, NULL);
   i->setSrc(2, NULL);
   i->dnz = 0;
   i->op = i->src(0).mod.getOp();
   if (i->op != OP_CVT)
      } else
   if (i->subOp != NV50_IR_SUBOP_MUL_HIGH &&
      (imm0.isInteger(1) || imm0.isInteger(-1))) {
   if (imm0.isNegative())
         if (s == 0) {
      i->setSrc(0, i->getSrc(1));
      }
   i->setSrc(1, i->getSrc(2));
   i->src(1).mod = i->src(2).mod;
   i->setSrc(2, NULL);
   i->dnz = 0;
      } else
   if (!isFloatType(i->dType) && !i->subOp && !i->src(t).mod && !i->src(2).mod) {
      bld.setPosition(i, false);
   int64_t b = typeSizeof(i->dType) == 8 ? imm0.reg.data.s64 : imm0.reg.data.s32;
   if (createMul(i->dType, i->getDef(0), i->getSrc(t), b, i->getSrc(2))) {
      delete_Instruction(prog, i);
         }
      case OP_SUB:
      if (imm0.isInteger(0) && s == 0 && typeSizeof(i->dType) == 8 &&
      !isFloatType(i->dType))
         case OP_ADD:
      if (i->usesFlags())
         if (imm0.isInteger(0)) {
      if (s == 0) {
      i->setSrc(0, i->getSrc(1));
   i->src(0).mod = i->src(1).mod;
   if (i->op == OP_SUB)
      }
   i->setSrc(1, NULL);
   i->op = i->src(0).mod.getOp();
   if (i->op != OP_CVT)
      }
         case OP_DIV:
      if (s != 1 || (i->dType != TYPE_S32 && i->dType != TYPE_U32))
         bld.setPosition(i, false);
   if (imm0.reg.data.u32 == 0) {
         } else
   if (imm0.reg.data.u32 == 1) {
      i->op = OP_MOV;
      } else
   if (i->dType == TYPE_U32 && imm0.isPow2()) {
      i->op = OP_SHR;
      } else
   if (i->dType == TYPE_U32) {
      Instruction *mul;
   Value *tA, *tB;
   const uint32_t d = imm0.reg.data.u32;
   uint32_t m;
   int r, s;
   uint32_t l = util_logbase2(d);
   if (((uint32_t)1 << l) < d)
         m = (((uint64_t)1 << 32) * (((uint64_t)1 << l) - d)) / d + 1;
                  tA = bld.getSSA();
   tB = bld.getSSA();
   mul = bld.mkOp2(OP_MUL, TYPE_U32, tA, i->getSrc(0),
         mul->subOp = NV50_IR_SUBOP_MUL_HIGH;
   bld.mkOp2(OP_SUB, TYPE_U32, tB, i->getSrc(0), tA);
   tA = bld.getSSA();
   if (r)
         else
         tB = s ? bld.getSSA() : i->getDef(0);
   newi = bld.mkOp2(OP_ADD, TYPE_U32, tB, mul->getDef(0), tA);
                  delete_Instruction(prog, i);
      } else
   if (imm0.reg.data.s32 == -1) {
      i->op = OP_NEG;
      } else {
      LValue *tA, *tB;
   LValue *tD;
   const int32_t d = imm0.reg.data.s32;
   int32_t m;
   int32_t l = util_logbase2(static_cast<unsigned>(abs(d)));
   if ((1 << l) < abs(d))
         if (!l)
                  tA = bld.getSSA();
   tB = bld.getSSA();
   bld.mkOp3(OP_MAD, TYPE_S32, tA, i->getSrc(0), bld.loadImm(NULL, m),
         if (l > 1)
         else
         tA = bld.getSSA();
   bld.mkCmp(OP_SET, CC_LT, TYPE_S32, tA, TYPE_S32, i->getSrc(0), bld.mkImm(0));
   tD = (d < 0) ? bld.getSSA() : i->getDef(0)->asLValue();
   newi = bld.mkOp2(OP_SUB, TYPE_U32, tD, tB, tA);
                  delete_Instruction(prog, i);
      }
         case OP_MOD:
      if (s == 1 && imm0.isPow2()) {
      bld.setPosition(i, false);
   if (i->sType == TYPE_U32) {
      i->op = OP_AND;
      } else if (i->sType == TYPE_S32) {
      // Do it on the absolute value of the input, and then restore the
   // sign. The only odd case is MIN_INT, but that should work out
   // as well, since MIN_INT mod any power of 2 is 0.
   //
   // Technically we don't have to do any of this since MOD is
   // undefined with negative arguments in GLSL, but this seems like
   // the nice thing to do.
   Value *abs = bld.mkOp1v(OP_ABS, TYPE_S32, bld.getSSA(), i->getSrc(0));
   Value *neg, *v1, *v2;
   bld.mkCmp(OP_SET, CC_LT, TYPE_S32,
               Value *mod = bld.mkOp2v(OP_AND, TYPE_U32, bld.getSSA(), abs,
         bld.mkOp1(OP_NEG, TYPE_S32, (v1 = bld.getSSA()), mod)
         bld.mkOp1(OP_MOV, TYPE_S32, (v2 = bld.getSSA()), mod)
                  delete_Instruction(prog, i);
         } else if (s == 1) {
      // In this case, we still want the optimized lowering that we get
   // from having division by an immediate.
   //
   // a % b == a - (a/b) * b
   bld.setPosition(i, false);
   Value *div = bld.mkOp2v(OP_DIV, i->sType, bld.getSSA(),
         newi = bld.mkOp2(OP_ADD, i->sType, i->getDef(0), i->getSrc(0),
         // TODO: Check that target supports this. In this case, we know that
                  delete_Instruction(prog, i);
      }
         case OP_SET: // TODO: SET_AND,OR,XOR
   {
      /* This optimizes the case where the output of a set is being compared
   * to zero. Since the set can only produce 0/-1 (int) or 0/1 (float), we
   * can be a lot cleverer in our comparison.
   */
   CmpInstruction *si = findOriginForTestWithZero(i->getSrc(t));
   CondCode cc, ccZ;
   if (imm0.reg.data.u32 != 0 || !si)
         cc = si->setCond;
   ccZ = (CondCode)((unsigned int)i->asCmp()->setCond & ~CC_U);
   // We do everything assuming var (cmp) 0, reverse the condition if 0 is
   // first.
   if (s == 0)
         // If there is a negative modifier, we need to undo that, by flipping
   // the comparison to zero.
   if (i->src(t).mod.neg())
         // If this is a signed comparison, we expect the input to be a regular
   // boolean, i.e. 0/-1. However the rest of the logic assumes that true
   // is positive, so just flip the sign.
   if (i->sType == TYPE_S32) {
      assert(!isFloatType(si->dType));
      }
   switch (ccZ) {
   case CC_LT: cc = CC_FL; break; // bool < 0 -- this is never true
   case CC_GE: cc = CC_TR; break; // bool >= 0 -- this is always true
   case CC_EQ: cc = inverseCondCode(cc); break; // bool == 0 -- !bool
   case CC_LE: cc = inverseCondCode(cc); break; // bool <= 0 -- !bool
   case CC_GT: break; // bool > 0 -- bool
   case CC_NE: break; // bool != 0 -- bool
   default:
                  // Update the condition of this SET to be identical to the origin set,
   // but with the updated condition code. The original SET should get
   // DCE'd, ideally.
   i->op = si->op;
   i->asCmp()->setCond = cc;
   i->setSrc(0, si->src(0));
   i->setSrc(1, si->src(1));
   if (si->srcExists(2))
            }
            case OP_AND:
   {
      Instruction *src = i->getSrc(t)->getInsn();
   ImmediateValue imm1;
   if (imm0.reg.data.u32 == 0) {
      i->op = OP_MOV;
   i->setSrc(0, new_ImmediateValue(prog, 0u));
   i->src(0).mod = Modifier(0);
      } else if (imm0.reg.data.u32 == ~0U) {
      i->op = i->src(t).mod.getOp();
   if (t) {
      i->setSrc(0, i->getSrc(t));
      }
      } else if (src->asCmp()) {
      CmpInstruction *cmp = src->asCmp();
   if (!cmp || cmp->op == OP_SLCT || cmp->getDef(0)->refCount() > 1)
         if (!prog->getTarget()->isOpSupported(cmp->op, TYPE_F32))
         if (imm0.reg.data.f32 != 1.0)
                        cmp->dType = TYPE_F32;
   if (i->src(t).mod != Modifier(0)) {
      assert(i->src(t).mod == Modifier(NV50_IR_MOD_NOT));
   i->src(t).mod = Modifier(0);
      }
   i->op = OP_MOV;
   i->setSrc(s, NULL);
   if (t) {
      i->setSrc(0, i->getSrc(t));
         } else if (prog->getTarget()->isOpSupported(OP_EXTBF, TYPE_U32) &&
            src->op == OP_SHR &&
   src->src(1).getImmediate(imm1) &&
   i->src(t).mod == Modifier(0) &&
   // low byte = offset, high byte = width
   uint32_t ext = (util_last_bit(imm0.reg.data.u32) << 8) | imm1.reg.data.u32;
   i->op = OP_EXTBF;
   i->setSrc(0, src->getSrc(0));
      } else if (src->op == OP_SHL &&
            src->src(1).getImmediate(imm1) &&
   i->src(t).mod == Modifier(0) &&
   util_is_power_of_two_or_zero(~imm0.reg.data.u32 + 1) &&
   i->op = OP_MOV;
   i->setSrc(s, NULL);
   if (t) {
      i->setSrc(0, i->getSrc(t));
            }
            case OP_SHL:
   {
      if (s != 1 || i->src(0).mod != Modifier(0))
            if (imm0.reg.data.u32 == 0) {
      i->op = OP_MOV;
   i->setSrc(1, NULL);
      }
   // try to concatenate shifts
   Instruction *si = i->getSrc(0)->getInsn();
   if (!si)
         ImmediateValue imm1;
   switch (si->op) {
   case OP_SHL:
      if (si->src(1).getImmediate(imm1)) {
      bld.setPosition(i, false);
   i->setSrc(0, si->getSrc(0));
      }
      case OP_SHR:
      if (si->src(1).getImmediate(imm1) && imm0.reg.data.u32 == imm1.reg.data.u32) {
      bld.setPosition(i, false);
   i->op = OP_AND;
   i->setSrc(0, si->getSrc(0));
      }
      case OP_MUL:
      int muls;
   if (isFloatType(si->dType))
         if (si->subOp)
         if (si->src(1).getImmediate(imm1))
         else if (si->src(0).getImmediate(imm1))
                        bld.setPosition(i, false);
   i->op = OP_MUL;
   i->subOp = 0;
   i->dType = si->dType;
   i->sType = si->sType;
   i->setSrc(0, si->getSrc(!muls));
   i->setSrc(1, bld.loadImm(NULL, imm1.reg.data.u32 << imm0.reg.data.u32));
      case OP_SUB:
   case OP_ADD:
      int adds;
   if (isFloatType(si->dType))
         if (si->op != OP_SUB && si->src(0).getImmediate(imm1))
         else if (si->src(1).getImmediate(imm1))
         else
         if (si->src(!adds).mod != Modifier(0))
                  // This is more operations, but if one of x, y is an immediate, then
   // we can get a situation where (a) we can use ISCADD, or (b)
   // propagate the add bit into an indirect load.
   bld.setPosition(i, false);
   i->op = si->op;
   i->setSrc(adds, bld.loadImm(NULL, imm1.reg.data.u32 << imm0.reg.data.u32));
   i->setSrc(!adds, bld.mkOp2v(OP_SHL, i->dType,
                        default:
            }
            case OP_ABS:
   case OP_NEG:
   case OP_SAT:
   case OP_LG2:
   case OP_RCP:
   case OP_SQRT:
   case OP_RSQ:
   case OP_PRESIN:
   case OP_SIN:
   case OP_COS:
   case OP_PREEX2:
   case OP_EX2:
      unary(i, imm0);
      case OP_BFIND: {
      int32_t res;
   switch (i->dType) {
   case TYPE_S32: res = util_last_bit_signed(imm0.reg.data.s32) - 1; break;
   case TYPE_U32: res = util_last_bit(imm0.reg.data.u32) - 1; break;
   default:
         }
   if (i->subOp == NV50_IR_SUBOP_BFIND_SAMT && res >= 0)
         bld.setPosition(i, false); /* make sure bld is init'ed */
   i->setSrc(0, bld.mkImm(res));
   i->setSrc(1, NULL);
   i->op = OP_MOV;
   i->subOp = 0;
      }
   case OP_BREV: {
      uint32_t res = util_bitreverse(imm0.reg.data.u32);
   i->setSrc(0, new_ImmediateValue(i->bb->getProgram(), res));
   i->op = OP_MOV;
      }
   case OP_POPCNT: {
      // Only deal with 1-arg POPCNT here
   if (i->srcExists(1))
         uint32_t res = util_bitcount(imm0.reg.data.u32);
   i->setSrc(0, new_ImmediateValue(i->bb->getProgram(), res));
   i->setSrc(1, NULL);
   i->op = OP_MOV;
      }
   case OP_CVT: {
               // TODO: handle 64-bit values properly
   if (typeSizeof(i->dType) == 8 || typeSizeof(i->sType) == 8)
            // TODO: handle single byte/word extractions
   if (i->subOp)
               #define CASE(type, dst, fmin, fmax, imin, imax, umin, umax) \
      case type: \
      switch (i->sType) { \
   case TYPE_F64: \
      res.data.dst = util_iround(i->saturate ? \
                  case TYPE_F32: \
      res.data.dst = util_iround(i->saturate ? \
                  case TYPE_S32: \
      res.data.dst = i->saturate ? \
                  case TYPE_U32: \
      res.data.dst = i->saturate ? \
                  case TYPE_S16: \
      res.data.dst = i->saturate ? \
                  case TYPE_U16: \
      res.data.dst = i->saturate ? \
                  default: return false; \
   } \
   i->setSrc(0, bld.mkImm(res.data.dst)); \
            switch(i->dType) {
   CASE(TYPE_U16, u16, 0, UINT16_MAX, 0, UINT16_MAX, 0, UINT16_MAX);
   CASE(TYPE_S16, s16, INT16_MIN, INT16_MAX, INT16_MIN, INT16_MAX, 0, INT16_MAX);
   CASE(TYPE_U32, u32, 0, (float)UINT32_MAX, 0, INT32_MAX, 0, UINT32_MAX);
   CASE(TYPE_S32, s32, (float)INT32_MIN, (float)INT32_MAX, INT32_MIN, INT32_MAX, 0, INT32_MAX);
   case TYPE_F32:
      switch (i->sType) {
   case TYPE_F64:
      res.data.f32 = i->saturate ?
      SATURATE(imm0.reg.data.f64) :
         case TYPE_F32:
      res.data.f32 = i->saturate ?
      SATURATE(imm0.reg.data.f32) :
         case TYPE_U16: res.data.f32 = (float) imm0.reg.data.u16; break;
   case TYPE_U32: res.data.f32 = (float) imm0.reg.data.u32; break;
   case TYPE_S16: res.data.f32 = (float) imm0.reg.data.s16; break;
   case TYPE_S32: res.data.f32 = (float) imm0.reg.data.s32; break;
   default:
         }
   i->setSrc(0, bld.mkImm(res.data.f32));
      case TYPE_F64:
      switch (i->sType) {
   case TYPE_F64:
      res.data.f64 = i->saturate ?
      SATURATE(imm0.reg.data.f64) :
         case TYPE_F32:
      res.data.f64 = i->saturate ?
      SATURATE(imm0.reg.data.f32) :
         case TYPE_U16: res.data.f64 = (double) imm0.reg.data.u16; break;
   case TYPE_U32: res.data.f64 = (double) imm0.reg.data.u32; break;
   case TYPE_S16: res.data.f64 = (double) imm0.reg.data.s16; break;
   case TYPE_S32: res.data.f64 = (double) imm0.reg.data.s32; break;
   default:
         }
   i->setSrc(0, bld.mkImm(res.data.f64));
      default:
         #undef CASE
            i->setType(i->dType); /* Remove i->sType, which we don't need anymore */
   i->op = OP_MOV;
   i->saturate = 0;
   i->src(0).mod = Modifier(0); /* Clear the already applied modifier */
      }
   default:
                  // This can get left behind some of the optimizations which simplify
   // saturatable values.
   if (newi->op == OP_MOV && newi->saturate) {
      ImmediateValue tmp;
   newi->saturate = 0;
   newi->op = OP_SAT;
   if (newi->src(0).getImmediate(tmp))
               if (newi->op != op)
            }
      // =============================================================================
      // Merge modifier operations (ABS, NEG, NOT) into ValueRefs where allowed.
   class ModifierFolding : public Pass
   {
   private:
         };
      bool
   ModifierFolding::visit(BasicBlock *bb)
   {
               Instruction *i, *next, *mi;
            for (i = bb->getEntry(); i; i = next) {
               if (false && i->op == OP_SUB) {
      // turn "sub" into "add neg" (do we really want this ?)
   i->op = OP_ADD;
               for (int s = 0; s < 3 && i->srcExists(s); ++s) {
      mi = i->getSrc(s)->getInsn();
   if (!mi ||
      mi->predSrc >= 0 || mi->getDef(0)->refCount() > 8)
      if (i->sType == TYPE_U32 && mi->dType == TYPE_S32) {
      if ((i->op != OP_ADD &&
      i->op != OP_MUL) ||
   (mi->op != OP_ABS &&
   mi->op != OP_NEG))
   } else
   if (i->sType != mi->dType) {
         }
   if ((mod = Modifier(mi->op)) == Modifier(0))
                  if ((i->op == OP_ABS) || i->src(s).mod.abs()) {
      // abs neg [abs] = abs
      } else
   if ((i->op == OP_NEG) && mod.neg()) {
      assert(s == 0);
   // neg as both opcode and modifier on same insn is prohibited
   // neg neg abs = abs, neg neg = identity
   mod = mod & Modifier(~NV50_IR_MOD_NEG);
   i->op = mod.getOp();
   mod = mod & Modifier(~NV50_IR_MOD_ABS);
   if (mod == Modifier(0))
               if (target->isModSupported(i, s, mod)) {
      i->setSrc(s, mi->getSrc(0));
                  if (i->op == OP_SAT) {
      mi = i->getSrc(0)->getInsn();
   if (mi &&
      mi->getDef(0)->refCount() <= 1 && target->isSatSupported(mi)) {
   mi->saturate = 1;
   mi->setDef(0, i->getDef(0));
                        }
      // =============================================================================
      // MUL + ADD -> MAD/FMA
   // MIN/MAX(a, a) -> a, etc.
   // SLCT(a, b, const) -> cc(const) ? a : b
   // RCP(RCP(a)) -> a
   // MUL(MUL(a, b), const) -> MUL_Xconst(a, b)
   // EXTBF(RDSV(COMBINED_TID)) -> RDSV(TID)
   class AlgebraicOpt : public Pass
   {
   private:
               void handleABS(Instruction *);
   bool handleADD(Instruction *);
   bool tryADDToMADOrSAD(Instruction *, operation toOp);
   void handleMINMAX(Instruction *);
   void handleRCP(Instruction *);
   void handleSLCT(Instruction *);
   void handleLOGOP(Instruction *);
   void handleCVT_NEG(Instruction *);
   void handleCVT_CVT(Instruction *);
   void handleCVT_EXTBF(Instruction *);
   void handleSUCLAMP(Instruction *);
   void handleNEG(Instruction *);
               };
      void
   AlgebraicOpt::handleABS(Instruction *abs)
   {
      Instruction *sub = abs->getSrc(0)->getInsn();
   DataType ty;
   if (!sub ||
      !prog->getTarget()->isOpSupported(OP_SAD, abs->dType))
      // hidden conversion ?
   ty = intTypeToSigned(sub->dType);
   if (abs->dType != abs->sType || ty != abs->sType)
            if ((sub->op != OP_ADD && sub->op != OP_SUB) ||
      sub->src(0).getFile() != FILE_GPR || sub->src(0).mod ||
   sub->src(1).getFile() != FILE_GPR || sub->src(1).mod)
         Value *src0 = sub->getSrc(0);
            if (sub->op == OP_ADD) {
      Instruction *neg = sub->getSrc(1)->getInsn();
   if (neg && neg->op != OP_NEG) {
      neg = sub->getSrc(0)->getInsn();
      }
   if (!neg || neg->op != OP_NEG ||
      neg->dType != neg->sType || neg->sType != ty)
                  // found ABS(SUB))
   abs->moveSources(1, 2); // move sources >=1 up by 2
   abs->op = OP_SAD;
   abs->setType(sub->dType);
   abs->setSrc(0, src0);
   abs->setSrc(1, src1);
   bld.setPosition(abs, false);
      }
      bool
   AlgebraicOpt::handleADD(Instruction *add)
   {
      Value *src0 = add->getSrc(0);
            if (src0->reg.file != FILE_GPR || src1->reg.file != FILE_GPR)
            bool changed = false;
   // we can't optimize to MAD if the add is precise
   if (!add->precise && prog->getTarget()->isOpSupported(OP_MAD, add->dType))
         if (!changed && prog->getTarget()->isOpSupported(OP_SAD, add->dType))
            }
      // ADD(SAD(a,b,0), c) -> SAD(a,b,c)
   // ADD(MUL(a,b), c) -> MAD(a,b,c)
   bool
   AlgebraicOpt::tryADDToMADOrSAD(Instruction *add, operation toOp)
   {
      Value *src0 = add->getSrc(0);
   Value *src1 = add->getSrc(1);
   Value *src;
   int s;
   const operation srcOp = toOp == OP_SAD ? OP_SAD : OP_MUL;
   const Modifier modBad = Modifier(~((toOp == OP_MAD) ? NV50_IR_MOD_NEG : 0));
            if (src0->refCount() == 1 &&
      src0->getUniqueInsn() && src0->getUniqueInsn()->op == srcOp)
      else
   if (src1->refCount() == 1 &&
      src1->getUniqueInsn() && src1->getUniqueInsn()->op == srcOp)
      else
                     if (src->getUniqueInsn() && src->getUniqueInsn()->bb != add->bb)
            if (src->getInsn()->saturate || src->getInsn()->postFactor ||
      src->getInsn()->dnz || src->getInsn()->precise)
         if (toOp == OP_SAD) {
      ImmediateValue imm;
   if (!src->getInsn()->src(2).getImmediate(imm))
         if (!imm.isInteger(0))
               if (typeSizeof(add->dType) != typeSizeof(src->getInsn()->dType) ||
      isFloatType(add->dType) != isFloatType(src->getInsn()->dType))
         mod[0] = add->src(0).mod;
   mod[1] = add->src(1).mod;
   mod[2] = src->getUniqueInsn()->src(0).mod;
            if (((mod[0] | mod[1]) | (mod[2] | mod[3])) & modBad)
            add->op = toOp;
   add->subOp = src->getInsn()->subOp; // potentially mul-high
   add->dnz = src->getInsn()->dnz;
   add->dType = src->getInsn()->dType; // sign matters for imad hi
                     add->setSrc(0, src->getInsn()->getSrc(0));
   add->src(0).mod = mod[2] ^ mod[s];
   add->setSrc(1, src->getInsn()->getSrc(1));
               }
      void
   AlgebraicOpt::handleMINMAX(Instruction *minmax)
   {
      Value *src0 = minmax->getSrc(0);
            if (src0 != src1 || src0->reg.file != FILE_GPR)
         if (minmax->src(0).mod == minmax->src(1).mod) {
      if (minmax->def(0).mayReplace(minmax->src(0))) {
      minmax->def(0).replace(minmax->src(0), false);
      } else {
      minmax->op = OP_CVT;
         } else {
      // TODO:
   // min(x, -x) = -abs(x)
   // min(x, -abs(x)) = -abs(x)
   // min(x, abs(x)) = x
   // max(x, -abs(x)) = x
   // max(x, abs(x)) = abs(x)
         }
      // rcp(rcp(a)) = a
   // rcp(sqrt(a)) = rsq(a)
   void
   AlgebraicOpt::handleRCP(Instruction *rcp)
   {
               if (!si)
            if (si->op == OP_RCP) {
      Modifier mod = rcp->src(0).mod * si->src(0).mod;
   rcp->op = mod.getOp();
      } else if (si->op == OP_SQRT) {
      rcp->op = OP_RSQ;
   rcp->setSrc(0, si->getSrc(0));
         }
      void
   AlgebraicOpt::handleSLCT(Instruction *slct)
   {
      if (slct->getSrc(2)->reg.file == FILE_IMMEDIATE) {
      if (slct->getSrc(2)->asImm()->compare(slct->asCmp()->setCond, 0.0f))
      } else
   if (slct->getSrc(0) != slct->getSrc(1)) {
         }
   slct->op = OP_MOV;
   slct->setSrc(1, NULL);
      }
      void
   AlgebraicOpt::handleLOGOP(Instruction *logop)
   {
      Value *src0 = logop->getSrc(0);
            if (src0->reg.file != FILE_GPR || src1->reg.file != FILE_GPR)
            if (src0 == src1) {
      if ((logop->op == OP_AND || logop->op == OP_OR) &&
      logop->def(0).mayReplace(logop->src(0))) {
   logop->def(0).replace(logop->src(0), false);
         } else {
      // try AND(SET, SET) -> SET_AND(SET)
   Instruction *set0 = src0->getInsn();
            if (!set0 || set0->fixed || !set1 || set1->fixed)
         if (set1->op != OP_SET) {
      Instruction *xchg = set0;
   set0 = set1;
   set1 = xchg;
   if (set1->op != OP_SET)
      }
   operation redOp = (logop->op == OP_AND ? OP_SET_AND :
         if (!prog->getTarget()->isOpSupported(redOp, set1->sType))
         if (set0->op != OP_SET &&
      set0->op != OP_SET_AND &&
   set0->op != OP_SET_OR &&
   set0->op != OP_SET_XOR)
      if (set0->getDef(0)->refCount() > 1 &&
      set1->getDef(0)->refCount() > 1)
      if (set0->getPredicate() || set1->getPredicate())
         // check that they don't source each other
   for (int s = 0; s < 2; ++s)
      if (set0->getSrc(s) == set1->getDef(0) ||
               set0 = cloneForward(func, set0);
   set1 = cloneShallow(func, set1);
   logop->bb->insertAfter(logop, set1);
            set0->dType = TYPE_U8;
   set0->getDef(0)->reg.file = FILE_PREDICATE;
   set0->getDef(0)->reg.size = 1;
   set1->setSrc(2, set0->getDef(0));
   set1->op = redOp;
   set1->setDef(0, logop->getDef(0));
         }
      // F2I(NEG(SET with result 1.0f/0.0f)) -> SET with result -1/0
   // nv50:
   //  F2I(NEG(I2F(ABS(SET))))
   void
   AlgebraicOpt::handleCVT_NEG(Instruction *cvt)
   {
      Instruction *insn = cvt->getSrc(0)->getInsn();
   if (cvt->sType != TYPE_F32 ||
      cvt->dType != TYPE_S32 || cvt->src(0).mod != Modifier(0))
      if (!insn || insn->op != OP_NEG || insn->dType != TYPE_F32)
         if (insn->src(0).mod != Modifier(0))
                  // check for nv50 SET(-1,0) -> SET(1.0f/0.0f) chain and nvc0's f32 SET
   if (insn && insn->op == OP_CVT &&
      insn->dType == TYPE_F32 &&
   insn->sType == TYPE_S32) {
   insn = insn->getSrc(0)->getInsn();
   if (!insn || insn->op != OP_ABS || insn->sType != TYPE_S32 ||
      insn->src(0).mod)
      insn = insn->getSrc(0)->getInsn();
   if (!insn || insn->op != OP_SET || insn->dType != TYPE_U32)
      } else
   if (!insn || insn->op != OP_SET || insn->dType != TYPE_F32) {
                  Instruction *bset = cloneShallow(func, insn);
   bset->dType = TYPE_U32;
   bset->setDef(0, cvt->getDef(0));
   cvt->bb->insertAfter(cvt, bset);
      }
      // F2I(TRUNC()) and so on can be expressed as a single CVT. If the earlier CVT
   // does a type conversion, this becomes trickier as there might be range
   // changes/etc. We could handle those in theory as long as the range was being
   // reduced or kept the same.
   void
   AlgebraicOpt::handleCVT_CVT(Instruction *cvt)
   {
               if (!insn ||
      insn->saturate ||
   insn->subOp ||
   insn->dType != insn->sType ||
   insn->dType != cvt->sType)
         RoundMode rnd = insn->rnd;
   switch (insn->op) {
   case OP_CEIL:
      rnd = ROUND_PI;
      case OP_FLOOR:
      rnd = ROUND_MI;
      case OP_TRUNC:
      rnd = ROUND_ZI;
      case OP_CVT:
         default:
                  if (!isFloatType(cvt->dType) || !isFloatType(insn->sType))
            cvt->rnd = rnd;
   cvt->setSrc(0, insn->getSrc(0));
   cvt->src(0).mod *= insn->src(0).mod;
      }
      // Some shaders extract packed bytes out of words and convert them to
   // e.g. float. The Fermi+ CVT instruction can extract those directly, as can
   // nv50 for word sizes.
   //
   // CVT(EXTBF(x, byte/word))
   // CVT(AND(bytemask, x))
   // CVT(AND(bytemask, SHR(x, 8/16/24)))
   // CVT(SHR(x, 16/24))
   void
   AlgebraicOpt::handleCVT_EXTBF(Instruction *cvt)
   {
      Instruction *insn = cvt->getSrc(0)->getInsn();
   ImmediateValue imm;
   Value *arg = NULL;
   unsigned width, offset = 0;
   if ((cvt->sType != TYPE_U32 && cvt->sType != TYPE_S32) || !insn)
         if (insn->op == OP_EXTBF && insn->src(1).getImmediate(imm)) {
      width = (imm.reg.data.u32 >> 8) & 0xff;
   offset = imm.reg.data.u32 & 0xff;
            if (width != 8 && width != 16)
         if (width == 8 && offset & 0x7)
         if (width == 16 && offset & 0xf)
      } else if (insn->op == OP_AND) {
      int s;
   if (insn->src(0).getImmediate(imm))
         else if (insn->src(1).getImmediate(imm))
         else
            if (imm.reg.data.u32 == 0xff)
         else if (imm.reg.data.u32 == 0xffff)
         else
            arg = insn->getSrc(!s);
            if (shift && shift->op == OP_SHR &&
      shift->sType == cvt->sType &&
   shift->src(1).getImmediate(imm) &&
   ((width == 8 && (imm.reg.data.u32 & 0x7) == 0) ||
   (width == 16 && (imm.reg.data.u32 & 0xf) == 0))) {
   arg = shift->getSrc(0);
      }
   // We just AND'd the high bits away, which means this is effectively an
   // unsigned value.
      } else if (insn->op == OP_SHR &&
            insn->sType == cvt->sType &&
   arg = insn->getSrc(0);
   if (imm.reg.data.u32 == 24) {
      width = 8;
      } else if (imm.reg.data.u32 == 16) {
      width = 16;
      } else {
                     if (!arg)
            // Irrespective of what came earlier, we can undo a shift on the argument
   // by adjusting the offset.
   Instruction *shift = arg->getInsn();
   if (shift && shift->op == OP_SHL &&
      shift->src(1).getImmediate(imm) &&
   ((width == 8 && (imm.reg.data.u32 & 0x7) == 0) ||
   (width == 16 && (imm.reg.data.u32 & 0xf) == 0)) &&
   imm.reg.data.u32 <= offset) {
   arg = shift->getSrc(0);
               // The unpackSnorm lowering still leaves a few shifts behind, but it's too
            if (width == 8) {
         } else {
      assert(width == 16);
      }
   cvt->setSrc(0, arg);
      }
      // SUCLAMP dst, (ADD b imm), k, 0 -> SUCLAMP dst, b, k, imm (if imm fits s6)
   void
   AlgebraicOpt::handleSUCLAMP(Instruction *insn)
   {
      ImmediateValue imm;
   int32_t val = insn->getSrc(2)->asImm()->reg.data.s32;
   int s;
                     // look for ADD (TODO: only count references by non-SUCLAMP)
   if (insn->getSrc(0)->refCount() > 1)
         add = insn->getSrc(0)->getInsn();
   if (!add || add->op != OP_ADD ||
      (add->dType != TYPE_U32 &&
   add->dType != TYPE_S32))
         // look for immediate
   for (s = 0; s < 2; ++s)
      if (add->src(s).getImmediate(imm))
      if (s >= 2)
         s = s ? 0 : 1;
   // determine if immediate fits
   val += imm.reg.data.s32;
   if (val > 31 || val < -32)
         // determine if other addend fits
   if (add->src(s).getFile() != FILE_GPR || add->src(s).mod != Modifier(0))
            bld.setPosition(insn, false); // make sure bld is init'ed
   // replace sources
   insn->setSrc(2, bld.mkImm(val));
      }
      // NEG(AND(SET, 1)) -> SET
   void
   AlgebraicOpt::handleNEG(Instruction *i) {
      Instruction *src = i->getSrc(0)->getInsn();
   ImmediateValue imm;
            if (isFloatType(i->sType) || !src || src->op != OP_AND)
            if (src->src(0).getImmediate(imm))
         else if (src->src(1).getImmediate(imm))
         else
            if (!imm.isInteger(1))
            Instruction *set = src->getSrc(b)->getInsn();
   if ((set->op == OP_SET || set->op == OP_SET_AND ||
      set->op == OP_SET_OR || set->op == OP_SET_XOR) &&
   !isFloatType(set->dType)) {
         }
      // EXTBF(RDSV(COMBINED_TID)) -> RDSV(TID)
   void
   AlgebraicOpt::handleEXTBF_RDSV(Instruction *i)
   {
      Instruction *rdsv = i->getSrc(0)->getUniqueInsn();
   if (rdsv->op != OP_RDSV ||
      rdsv->getSrc(0)->asSym()->reg.data.sv.sv != SV_COMBINED_TID)
      // Avoid creating more RDSV instructions
   if (rdsv->getDef(0)->refCount() > 1)
            ImmediateValue imm;
   if (!i->src(1).getImmediate(imm))
            int index;
   if (imm.isInteger(0x1000))
         else
   if (imm.isInteger(0x0a10))
         else
   if (imm.isInteger(0x061a))
         else
                     i->op = OP_RDSV;
   i->setSrc(0, bld.mkSysVal(SV_TID, index));
      }
      bool
   AlgebraicOpt::visit(BasicBlock *bb)
   {
      Instruction *next;
   for (Instruction *i = bb->getEntry(); i; i = next) {
      next = i->next;
   switch (i->op) {
   case OP_ABS:
      handleABS(i);
      case OP_ADD:
      handleADD(i);
      case OP_RCP:
      handleRCP(i);
      case OP_MIN:
   case OP_MAX:
      handleMINMAX(i);
      case OP_SLCT:
      handleSLCT(i);
      case OP_AND:
   case OP_OR:
   case OP_XOR:
      handleLOGOP(i);
      case OP_CVT:
      handleCVT_NEG(i);
   handleCVT_CVT(i);
   if (prog->getTarget()->isOpSupported(OP_EXTBF, TYPE_U32))
            case OP_SUCLAMP:
      handleSUCLAMP(i);
      case OP_NEG:
      handleNEG(i);
      case OP_EXTBF:
      handleEXTBF_RDSV(i);
      default:
                        }
      // =============================================================================
      // ADD(SHL(a, b), c) -> SHLADD(a, b, c)
   // MUL(a, b) -> a few XMADs
   // MAD/FMA(a, b, c) -> a few XMADs
   class LateAlgebraicOpt : public Pass
   {
   private:
               void handleADD(Instruction *);
   void handleMULMAD(Instruction *);
               };
      void
   LateAlgebraicOpt::handleADD(Instruction *add)
   {
      Value *src0 = add->getSrc(0);
            if (src0->reg.file != FILE_GPR || src1->reg.file != FILE_GPR)
            if (prog->getTarget()->isOpSupported(OP_SHLADD, add->dType))
      }
      // ADD(SHL(a, b), c) -> SHLADD(a, b, c)
   bool
   LateAlgebraicOpt::tryADDToSHLADD(Instruction *add)
   {
      Value *src0 = add->getSrc(0);
   Value *src1 = add->getSrc(1);
   ImmediateValue imm;
   Instruction *shl;
   Value *src;
            if (add->saturate || add->usesFlags() || typeSizeof(add->dType) == 8
      || isFloatType(add->dType))
         if (src0->getUniqueInsn() && src0->getUniqueInsn()->op == OP_SHL)
         else
   if (src1->getUniqueInsn() && src1->getUniqueInsn()->op == OP_SHL)
         else
            src = add->getSrc(s);
            if (shl->bb != add->bb || shl->usesFlags() || shl->subOp || shl->src(0).mod)
            if (!shl->src(1).getImmediate(imm))
            add->op = OP_SHLADD;
   add->setSrc(2, add->src(!s));
   // SHL can't have any modifiers, but the ADD source may have had
   // one. Preserve it.
   add->setSrc(0, shl->getSrc(0));
   if (s == 1)
         add->setSrc(1, new_ImmediateValue(shl->bb->getProgram(), imm.reg.data.u32));
               }
      // MUL(a, b) -> a few XMADs
   // MAD/FMA(a, b, c) -> a few XMADs
   void
   LateAlgebraicOpt::handleMULMAD(Instruction *i)
   {
      // TODO: handle NV50_IR_SUBOP_MUL_HIGH
   if (!prog->getTarget()->isOpSupported(OP_XMAD, TYPE_U32))
         if (isFloatType(i->dType) || typeSizeof(i->dType) != 4)
         if (i->subOp || i->usesFlags() || i->flagsDef >= 0)
            assert(!i->src(0).mod);
   assert(!i->src(1).mod);
                     Value *a = i->getSrc(0);
   Value *b = i->getSrc(1);
            Value *tmp0 = bld.getSSA();
            Instruction *insn = bld.mkOp3(OP_XMAD, TYPE_U32, tmp0, b, a, c);
            insn = bld.mkOp3(OP_XMAD, TYPE_U32, tmp1, b, a, bld.mkImm(0));
   insn->setPredicate(i->cc, i->getPredicate());
            Value *pred = i->getPredicate();
            i->op = OP_XMAD;
   i->setSrc(0, b);
   i->setSrc(1, tmp1);
   i->setSrc(2, tmp0);
   i->subOp = NV50_IR_SUBOP_XMAD_PSL | NV50_IR_SUBOP_XMAD_CBCC;
               }
      bool
   LateAlgebraicOpt::visit(Instruction *i)
   {
      switch (i->op) {
   case OP_ADD:
      handleADD(i);
      case OP_MUL:
   case OP_MAD:
   case OP_FMA:
      handleMULMAD(i);
      default:
                     }
      // =============================================================================
      // Split 64-bit MUL and MAD
   class Split64BitOpPreRA : public Pass
   {
   private:
      virtual bool visit(BasicBlock *);
               };
      bool
   Split64BitOpPreRA::visit(BasicBlock *bb)
   {
      Instruction *i, *next;
            for (i = bb->getEntry(); i; i = next) {
               DataType hTy;
   switch (i->dType) {
   case TYPE_U64: hTy = TYPE_U32; break;
   case TYPE_S64: hTy = TYPE_S32; break;
   default:
                  if (i->op == OP_MAD || i->op == OP_MUL)
                  }
      void
   Split64BitOpPreRA::split64MulMad(Function *fn, Instruction *i, DataType hTy)
   {
      assert(i->op == OP_MAD || i->op == OP_MUL);
   assert(!isFloatType(i->dType) && !isFloatType(i->sType));
                     Value *zero = bld.mkImm(0u);
            // We want to compute `d = a * b (+ c)?`, where a, b, c and d are 64-bit
   // values (a, b and c might be 32-bit values), using 32-bit operations. This
   // gives the following operations:
   // * `d.low = low(a.low * b.low) (+ c.low)?`
   // * `d.high = low(a.high * b.low) + low(a.low * b.high)
   //           + high(a.low * b.low) (+ c.high)?`
   //
   // To compute the high bits, we can split in the following operations:
   // * `tmp1   = low(a.high * b.low) (+ c.high)?`
   // * `tmp2   = low(a.low * b.high) + tmp1`
   // * `d.high = high(a.low * b.low) + tmp2`
   //
            Value *op1[2];
   if (i->getSrc(0)->reg.size == 8)
         else {
      op1[0] = i->getSrc(0);
      }
   Value *op2[2];
   if (i->getSrc(1)->reg.size == 8)
         else {
      op2[0] = i->getSrc(1);
               Value *op3[2] = { NULL, NULL };
   if (i->op == OP_MAD) {
      if (i->getSrc(2)->reg.size == 8)
         else {
      op3[0] = i->getSrc(2);
                  Value *tmpRes1Hi = bld.getSSA();
   if (i->op == OP_MAD)
         else
                              // If it was a MAD, add the carry from the low bits
   // It is not needed if it was a MUL, since we added high(a.low * b.low) to
   // d.high
   if (i->op == OP_MAD)
         else
            Instruction *hiPart3 = bld.mkOp3(OP_MAD, hTy, def[1], op1[0], op2[0], tmpRes2Hi);
   hiPart3->subOp = NV50_IR_SUBOP_MUL_HIGH;
   if (i->op == OP_MAD)
                        }
      // =============================================================================
      static inline void
   updateLdStOffset(Instruction *ldst, int32_t offset, Function *fn)
   {
      if (offset != ldst->getSrc(0)->reg.data.offset) {
      if (ldst->getSrc(0)->refCount() > 1)
               }
      // Combine loads and stores, forward stores to loads where possible.
   class MemoryOpt : public Pass
   {
   private:
      class Record
   {
   public:
      Record *next;
   Instruction *insn;
   const Value *rel[2];
   const Value *base;
   int32_t offset;
   int8_t fileIndex;
   uint8_t size;
   bool locked;
                     inline void link(Record **);
   inline void unlink(Record **);
            public:
               Record *loads[DATA_FILE_COUNT];
                  private:
      virtual bool visit(BasicBlock *);
                              // merge @insn into load/store instruction from @rec
   bool combineLd(Record *rec, Instruction *ld);
            bool replaceLdFromLd(Instruction *ld, Record *ldRec);
   bool replaceLdFromSt(Instruction *ld, Record *stRec);
            void addRecord(Instruction *ldst);
   void purgeRecords(Instruction *const st, DataFile);
   void lockStores(Instruction *const ld);
         private:
         };
      MemoryOpt::MemoryOpt() : recordPool(sizeof(MemoryOpt::Record), 6)
   {
      for (int i = 0; i < DATA_FILE_COUNT; ++i) {
      loads[i] = NULL;
      }
      }
      void
   MemoryOpt::reset()
   {
      for (unsigned int i = 0; i < DATA_FILE_COUNT; ++i) {
      Record *it, *next;
   for (it = loads[i]; it; it = next) {
      next = it->next;
      }
   loads[i] = NULL;
   for (it = stores[i]; it; it = next) {
      next = it->next;
      }
         }
      bool
   MemoryOpt::combineLd(Record *rec, Instruction *ld)
   {
      int32_t offRc = rec->offset;
   int32_t offLd = ld->getSrc(0)->reg.data.offset;
   int sizeRc = rec->size;
   int sizeLd = typeSizeof(ld->dType);
   int size = sizeRc + sizeLd;
            if (!prog->getTarget()->
      isAccessSupported(ld->getSrc(0)->reg.file, typeOfSize(size)))
      // no unaligned loads
   if (((size == 0x8) && (MIN2(offLd, offRc) & 0x7)) ||
      ((size == 0xc) && (MIN2(offLd, offRc) & 0xf)))
      // for compute indirect loads are not guaranteed to be aligned
   if (prog->getType() == Program::TYPE_COMPUTE && rec->rel[0])
                     // lock any stores that overlap with the load being merged into the
   // existing record.
                     if (offLd < offRc) {
      int sz;
   for (sz = 0, d = 0; sz < sizeLd; sz += ld->getDef(d)->reg.size, ++d);
   // d: nr of definitions in ld
   // j: nr of definitions in rec->insn, move:
   for (d = d + j - 1; j > 0; --j, --d)
            if (rec->insn->getSrc(0)->refCount() > 1)
                     } else {
         }
   // move definitions of @ld to @rec->insn
   for (j = 0; sizeLd; ++j, ++d) {
      sizeLd -= ld->getDef(j)->reg.size;
               rec->size = size;
   rec->insn->getSrc(0)->reg.size = size;
                        }
      bool
   MemoryOpt::combineSt(Record *rec, Instruction *st)
   {
      int32_t offRc = rec->offset;
   int32_t offSt = st->getSrc(0)->reg.data.offset;
   int sizeRc = rec->size;
   int sizeSt = typeSizeof(st->dType);
   int s = sizeSt / 4;
   int size = sizeRc + sizeSt;
   int j, k;
   Value *src[4]; // no modifiers in ValueRef allowed for st
            if (!prog->getTarget()->
      isAccessSupported(st->getSrc(0)->reg.file, typeOfSize(size)))
      // no unaligned stores
   if (size == 8 && MIN2(offRc, offSt) & 0x7)
         // for compute indirect stores are not guaranteed to be aligned
   if (prog->getType() == Program::TYPE_COMPUTE && rec->rel[0])
            // There's really no great place to put this in a generic manner. Seemingly
   // wide stores at 0x60 don't work in GS shaders on SM50+. Don't combine
   // those.
   if (prog->getTarget()->getChipset() >= NVISA_GM107_CHIPSET &&
      prog->getType() == Program::TYPE_GEOMETRY &&
   st->getSrc(0)->reg.file == FILE_SHADER_OUTPUT &&
   rec->rel[0] == NULL &&
   MIN2(offRc, offSt) == 0x60)
         // remove any existing load/store records for the store being merged into
   // the existing record.
                     if (offRc < offSt) {
      // save values from @st
   for (s = 0; sizeSt; ++s) {
      sizeSt -= st->getSrc(s + 1)->reg.size;
      }
   // set record's values as low sources of @st
   for (j = 1; sizeRc; ++j) {
      sizeRc -= rec->insn->getSrc(j)->reg.size;
      }
   // set saved values as high sources of @st
   for (k = j, j = 0; j < s; ++j)
               } else {
      for (j = 1; sizeSt; ++j)
         for (s = 1; sizeRc; ++j, ++s) {
      sizeRc -= rec->insn->getSrc(s)->reg.size;
      }
      }
            delete_Instruction(prog, rec->insn);
   rec->insn = st;
   rec->size = size;
   rec->insn->getSrc(0)->reg.size = size;
   rec->insn->setType(typeOfSize(size));
      }
      void
   MemoryOpt::Record::set(const Instruction *ldst)
   {
      const Symbol *mem = ldst->getSrc(0)->asSym();
   fileIndex = mem->reg.fileIndex;
   rel[0] = ldst->getIndirect(0, 0);
   rel[1] = ldst->getIndirect(0, 1);
   offset = mem->reg.data.offset;
   base = mem->getBase();
      }
      void
   MemoryOpt::Record::link(Record **list)
   {
      next = *list;
   if (next)
         prev = NULL;
      }
      void
   MemoryOpt::Record::unlink(Record **list)
   {
      if (next)
         if (prev)
         else
      }
      MemoryOpt::Record **
   MemoryOpt::getList(const Instruction *insn)
   {
      if (insn->op == OP_LOAD || insn->op == OP_VFETCH)
            }
      void
   MemoryOpt::addRecord(Instruction *i)
   {
      Record **list = getList(i);
            it->link(list);
   it->set(i);
   it->insn = i;
      }
      MemoryOpt::Record *
   MemoryOpt::findRecord(const Instruction *insn, bool load, bool& isAdj) const
   {
      const Symbol *sym = insn->getSrc(0)->asSym();
   const int size = typeSizeof(insn->sType);
   Record *rec = NULL;
            for (; it; it = it->next) {
      if (it->locked && insn->op != OP_LOAD && insn->op != OP_VFETCH)
         if ((it->offset >> 4) != (sym->reg.data.offset >> 4) ||
      it->rel[0] != insn->getIndirect(0, 0) ||
   it->fileIndex != sym->reg.fileIndex ||
               if (it->offset < sym->reg.data.offset) {
      if (it->offset + it->size >= sym->reg.data.offset) {
      isAdj = (it->offset + it->size == sym->reg.data.offset);
   if (!isAdj)
         if (!(it->offset & 0x7))
         } else {
      isAdj = it->offset != sym->reg.data.offset;
   if (size <= it->size && !isAdj)
         else
   if (!(sym->reg.data.offset & 0x7))
      if (it->offset - size <= sym->reg.data.offset)
      }
      }
      bool
   MemoryOpt::replaceLdFromSt(Instruction *ld, Record *rec)
   {
      Instruction *st = rec->insn;
   int32_t offSt = rec->offset;
   int32_t offLd = ld->getSrc(0)->reg.data.offset;
            for (s = 1; offSt != offLd && st->srcExists(s); ++s)
         if (offSt != offLd)
            for (d = 0; ld->defExists(d) && st->srcExists(s); ++d, ++s) {
      if (ld->getDef(d)->reg.size != st->getSrc(s)->reg.size)
         if (st->getSrc(s)->reg.file != FILE_GPR)
            }
   ld->bb->remove(ld);
      }
      bool
   MemoryOpt::replaceLdFromLd(Instruction *ldE, Record *rec)
   {
      Instruction *ldR = rec->insn;
   int32_t offR = rec->offset;
   int32_t offE = ldE->getSrc(0)->reg.data.offset;
            assert(offR <= offE);
   for (dR = 0; offR < offE && ldR->defExists(dR); ++dR)
         if (offR != offE)
            for (dE = 0; ldE->defExists(dE) && ldR->defExists(dR); ++dE, ++dR) {
      if (ldE->getDef(dE)->reg.size != ldR->getDef(dR)->reg.size)
                     delete_Instruction(prog, ldE);
      }
      bool
   MemoryOpt::replaceStFromSt(Instruction *restrict st, Record *rec)
   {
      const Instruction *const ri = rec->insn;
            int32_t offS = st->getSrc(0)->reg.data.offset;
   int32_t offR = rec->offset;
   int32_t endS = offS + typeSizeof(st->dType);
                              if (offR < offS) {
      Value *vals[10];
   int s, n;
   int k = 0;
   // get non-replaced sources of ri
   for (s = 1; offR < offS; offR += ri->getSrc(s)->reg.size, ++s)
         n = s;
   // get replaced sources of st
   for (s = 1; st->srcExists(s); offS += st->getSrc(s)->reg.size, ++s)
         // skip replaced sources of ri
   for (s = n; offR < endS; offR += ri->getSrc(s)->reg.size, ++s);
   // get non-replaced sources after values covered by st
   for (; offR < endR; offR += ri->getSrc(s)->reg.size, ++s)
         assert((unsigned int)k <= ARRAY_SIZE(vals));
   for (s = 0; s < k; ++s)
            } else
   if (endR > endS) {
      int j, s;
   for (j = 1; offR < endS; offR += ri->getSrc(j++)->reg.size);
   for (s = 1; offS < endS; offS += st->getSrc(s++)->reg.size);
   for (; offR < endR; offR += ri->getSrc(j++)->reg.size)
      }
                     rec->insn = st;
                        }
      bool
   MemoryOpt::Record::overlaps(const Instruction *ldst) const
   {
      Record that;
            // This assumes that images/buffers can't overlap. They can.
   // TODO: Plumb the restrict logic through, and only skip when it's a
   // restrict situation, or there can implicitly be no writes.
   if (this->fileIndex != that.fileIndex && this->rel[1] == that.rel[1])
            if (this->rel[0] || that.rel[0])
            return
      (this->offset < that.offset + that.size) &&
   }
      // We must not eliminate stores that affect the result of @ld if
   // we find later stores to the same location, and we may no longer
   // merge them with later stores.
   // The stored value can, however, still be used to determine the value
   // returned by future loads.
   void
   MemoryOpt::lockStores(Instruction *const ld)
   {
      for (Record *r = stores[ld->src(0).getFile()]; r; r = r->next)
      if (!r->locked && r->overlaps(ld))
   }
      // Prior loads from the location of @st are no longer valid.
   // Stores to the location of @st may no longer be used to derive
   // the value at it nor be coalesced into later stores.
   void
   MemoryOpt::purgeRecords(Instruction *const st, DataFile f)
   {
      if (st)
            for (Record *r = loads[f]; r; r = r->next)
      if (!st || r->overlaps(st))
         for (Record *r = stores[f]; r; r = r->next)
      if (!st || r->overlaps(st))
   }
      bool
   MemoryOpt::visit(BasicBlock *bb)
   {
      bool ret = runOpt(bb);
   // Run again, one pass won't combine 4 32 bit ld/st to a single 128 bit ld/st
   // where 96 bit memory operations are forbidden.
   if (ret)
            }
      bool
   MemoryOpt::runOpt(BasicBlock *bb)
   {
      Instruction *ldst, *next;
   Record *rec;
            for (ldst = bb->getEntry(); ldst; ldst = next) {
      bool keep = true;
   bool isLoad = true;
            // TODO: Handle combining sub 4-bytes loads/stores.
   if (ldst->op == OP_STORE && typeSizeof(ldst->dType) < 4) {
      purgeRecords(ldst, ldst->src(0).getFile());
               if (ldst->op == OP_LOAD || ldst->op == OP_VFETCH) {
      if (ldst->subOp == NV50_IR_SUBOP_LOAD_LOCKED) {
      purgeRecords(ldst, ldst->src(0).getFile());
      }
   if (ldst->isDead()) {
      // might have been produced by earlier optimization
   delete_Instruction(prog, ldst);
         } else
   if (ldst->op == OP_STORE || ldst->op == OP_EXPORT) {
      if (ldst->subOp == NV50_IR_SUBOP_STORE_UNLOCKED) {
      purgeRecords(ldst, ldst->src(0).getFile());
      }
   if (typeSizeof(ldst->dType) == 4 &&
      ldst->src(1).getFile() == FILE_GPR &&
   ldst->getSrc(1)->getInsn()->op == OP_NOP) {
   delete_Instruction(prog, ldst);
      }
      } else {
      // TODO: maybe have all fixed ops act as barrier ?
   if (ldst->op == OP_CALL ||
      ldst->op == OP_BAR ||
   ldst->op == OP_MEMBAR) {
   purgeRecords(NULL, FILE_MEMORY_LOCAL);
   purgeRecords(NULL, FILE_MEMORY_GLOBAL);
   purgeRecords(NULL, FILE_MEMORY_SHARED);
      } else
   if (ldst->op == OP_ATOM || ldst->op == OP_CCTL) {
      if (ldst->src(0).getFile() == FILE_MEMORY_GLOBAL) {
      purgeRecords(NULL, FILE_MEMORY_LOCAL);
   purgeRecords(NULL, FILE_MEMORY_GLOBAL);
      } else {
            } else
   if (ldst->op == OP_EMIT || ldst->op == OP_RESTART) {
         }
      }
   if (ldst->getPredicate()) // TODO: handle predicated ld/st
         if (ldst->perPatch) // TODO: create separate per-patch lists
            if (isLoad) {
               // if ld l[]/g[] look for previous store to eliminate the reload
   if (file == FILE_MEMORY_GLOBAL || file == FILE_MEMORY_LOCAL) {
      // TODO: shared memory ?
   rec = findRecord(ldst, false, isAdjacent);
   if (rec && !isAdjacent)
               // or look for ld from the same location and replace this one
   rec = keep ? findRecord(ldst, true, isAdjacent) : NULL;
   if (rec) {
      if (!isAdjacent)
         else
      // or combine a previous load with this one
   }
   if (keep)
      } else {
      rec = findRecord(ldst, false, isAdjacent);
   if (rec) {
      if (!isAdjacent)
         else
      }
   if (keep)
      }
   if (keep)
      }
               }
      // =============================================================================
      // Turn control flow into predicated instructions (after register allocation !).
   // TODO:
   // Could move this to before register allocation on NVC0 and also handle nested
   // constructs.
   class FlatteningPass : public Pass
   {
   public:
            private:
      virtual bool visit(Function *);
            bool tryPredicateConditional(BasicBlock *);
   void predicateInstructions(BasicBlock *, Value *pred, CondCode cc);
   void tryPropagateBranch(BasicBlock *);
   inline bool isConstantCondition(Value *pred);
   inline bool mayPredicate(const Instruction *, const Value *pred) const;
               };
      bool
   FlatteningPass::isConstantCondition(Value *pred)
   {
      Instruction *insn = pred->getUniqueInsn();
   assert(insn);
   if (insn->op != OP_SET || insn->srcExists(2))
            for (int s = 0; s < 2 && insn->srcExists(s); ++s) {
      Instruction *ld = insn->getSrc(s)->getUniqueInsn();
   DataFile file;
   if (ld) {
      if (ld->op != OP_MOV && ld->op != OP_LOAD)
         if (ld->src(0).isIndirect(0))
            } else {
      file = insn->src(s).getFile();
   // catch $r63 on NVC0 and $r63/$r127 on NV50. Unfortunately maxGPR is
   // in register "units", which can vary between targets.
   if (file == FILE_GPR) {
      Value *v = insn->getSrc(s);
   int bytes = v->reg.data.id * MIN2(v->reg.size, 4);
   int units = bytes >> gpr_unit;
   if (units > prog->maxGPR)
         }
   if (file != FILE_IMMEDIATE && file != FILE_MEMORY_CONST)
      }
      }
      void
   FlatteningPass::removeFlow(Instruction *insn)
   {
      FlowInstruction *term = insn ? insn->asFlow() : NULL;
   if (!term)
                  if (term->op == OP_BRA) {
      // TODO: this might get more difficult when we get arbitrary BRAs
   if (ty == Graph::Edge::CROSS || ty == Graph::Edge::BACK)
      } else
   if (term->op != OP_JOIN)
                              if (pred && pred->refCount() == 0) {
      Instruction *pSet = pred->getUniqueInsn();
   pred->join->reg.data.id = -1; // deallocate
   if (pSet->isDead())
         }
      void
   FlatteningPass::predicateInstructions(BasicBlock *bb, Value *pred, CondCode cc)
   {
      for (Instruction *i = bb->getEntry(); i; i = i->next) {
      if (i->isNop())
         assert(!i->getPredicate());
      }
      }
      bool
   FlatteningPass::mayPredicate(const Instruction *insn, const Value *pred) const
   {
      if (insn->isPseudo())
                  if (!prog->getTarget()->mayPredicate(insn, pred))
         for (int d = 0; insn->defExists(d); ++d)
      if (insn->getDef(d)->equals(pred))
         }
      // If we jump to BRA/RET/EXIT, replace the jump with it.
   // NOTE: We do not update the CFG anymore here !
   //
   // TODO: Handle cases where we skip over a branch (maybe do that elsewhere ?):
   //  BB:0
   //   @p0 bra BB:2 -> @!p0 bra BB:3 iff (!) BB:2 immediately adjoins BB:1
   //  BB1:
   //   bra BB:3
   //  BB2:
   //   ...
   //  BB3:
   //   ...
   void
   FlatteningPass::tryPropagateBranch(BasicBlock *bb)
   {
      for (Instruction *i = bb->getExit(); i && i->op == OP_BRA; i = i->prev) {
               if (bf->getInsnCount() != 1)
            FlowInstruction *bra = i->asFlow();
            if (!rep || rep->getPredicate())
         if (rep->op != OP_BRA &&
      rep->op != OP_JOIN &&
               // TODO: If there are multiple branches to @rep, only the first would
   // be replaced, so only remove them after this pass is done ?
   // Also, need to check all incident blocks for fall-through exits and
   // add the branch there.
   bra->op = rep->op;
   bra->target.bb = rep->target.bb;
   if (bf->cfg.incidentCount() == 1)
         }
      bool
   FlatteningPass::visit(Function *fn)
   {
                  }
      bool
   FlatteningPass::visit(BasicBlock *bb)
   {
      if (tryPredicateConditional(bb))
            // try to attach join to previous instruction
   if (prog->getTarget()->hasJoin) {
      Instruction *insn = bb->getExit();
   if (insn && insn->op == OP_JOIN && !insn->getPredicate()) {
      insn = insn->prev;
   if (insn && !insn->getPredicate() &&
      !insn->asFlow() &&
   insn->op != OP_DISCARD &&
   insn->op != OP_TEXBAR &&
   !isTextureOp(insn->op) && // probably just nve4
   !isSurfaceOp(insn->op) && // not confirmed
   insn->op != OP_LINTERP && // probably just nve4
   insn->op != OP_PINTERP && // probably just nve4
   ((insn->op != OP_LOAD && insn->op != OP_STORE && insn->op != OP_ATOM) ||
   (typeSizeof(insn->dType) <= 4 && !insn->src(0).isIndirect(0))) &&
   !insn->isNop()) {
   insn->join = 1;
   bb->remove(bb->getExit());
                                 }
      bool
   FlatteningPass::tryPredicateConditional(BasicBlock *bb)
   {
      BasicBlock *bL = NULL, *bR = NULL;
   unsigned int nL = 0, nR = 0, limit = 12;
   Instruction *insn;
            mask = bb->initiatesSimpleConditional();
   if (!mask)
            assert(bb->getExit());
   Value *pred = bb->getExit()->getPredicate();
            if (isConstantCondition(pred))
                     if (mask & 1) {
      bL = BasicBlock::get(ei.getNode());
   for (insn = bL->getEntry(); insn; insn = insn->next, ++nL)
      if (!mayPredicate(insn, pred))
      if (nL > limit)
      }
            if (mask & 2) {
      bR = BasicBlock::get(ei.getNode());
   for (insn = bR->getEntry(); insn; insn = insn->next, ++nR)
      if (!mayPredicate(insn, pred))
      if (nR > limit)
               if (bL)
         if (bR)
            if (bb->joinAt) {
      bb->remove(bb->joinAt);
      }
            // remove potential join operations at the end of the conditional
   if (prog->getTarget()->joinAnterior) {
      bb = BasicBlock::get((bL ? bL : bR)->cfg.outgoing().getNode());
   if (bb->getEntry() && bb->getEntry()->op == OP_JOIN)
                  }
      // =============================================================================
      // Fold Immediate into MAD; must be done after register allocation due to
   // constraint SDST == SSRC2
   // TODO:
   // Does NVC0+ have other situations where this pass makes sense?
   class PostRaLoadPropagation : public Pass
   {
   private:
               void handleMADforNV50(Instruction *);
      };
      static bool
   post_ra_dead(Instruction *i)
   {
      for (int d = 0; i->defExists(d); ++d)
      if (i->getDef(d)->refCount())
         }
      // Fold Immediate into MAD; must be done after register allocation due to
   // constraint SDST == SSRC2
   void
   PostRaLoadPropagation::handleMADforNV50(Instruction *i)
   {
      if (i->def(0).getFile() != FILE_GPR ||
      i->src(0).getFile() != FILE_GPR ||
   i->src(1).getFile() != FILE_GPR ||
   i->src(2).getFile() != FILE_GPR ||
   i->getDef(0)->reg.data.id != i->getSrc(2)->reg.data.id)
         if (i->getDef(0)->reg.data.id >= 64 ||
      i->getSrc(0)->reg.data.id >= 64)
         if (i->flagsSrc >= 0 && i->getSrc(i->flagsSrc)->reg.data.id != 0)
            if (i->getPredicate())
            Value *vtmp;
            if (def && def->op == OP_SPLIT && typeSizeof(def->sType) == 4)
         if (def && def->op == OP_MOV && def->src(0).getFile() == FILE_IMMEDIATE) {
      vtmp = i->getSrc(1);
   if (isFloatType(i->sType)) {
         } else {
      ImmediateValue val;
   // getImmediate() has side-effects on the argument so this *shouldn't*
   // be folded into the assert()
   ASSERTED bool ret = def->src(0).getImmediate(val);
   assert(ret);
   if (i->getSrc(1)->reg.data.id & 1)
         val.reg.data.u32 &= 0xffff;
               /* There's no post-RA dead code elimination, so do it here
   * XXX: if we add more code-removing post-RA passes, we might
   *      want to create a post-RA dead-code elim pass */
   if (post_ra_dead(vtmp->getInsn())) {
      Value *src = vtmp->getInsn()->getSrc(0);
   // Careful -- splits will have already been removed from the
   // functions. Don't double-delete.
   if (vtmp->getInsn()->bb)
         if (src->getInsn() && post_ra_dead(src->getInsn()))
            }
      void
   PostRaLoadPropagation::handleMADforNVC0(Instruction *i)
   {
      if (i->def(0).getFile() != FILE_GPR ||
      i->src(0).getFile() != FILE_GPR ||
   i->src(1).getFile() != FILE_GPR ||
   i->src(2).getFile() != FILE_GPR ||
   i->getDef(0)->reg.data.id != i->getSrc(2)->reg.data.id)
         // TODO: gm107 can also do this for S32, maybe other chipsets as well
   if (i->dType != TYPE_F32)
            if ((i->src(2).mod | Modifier(NV50_IR_MOD_NEG)) != Modifier(NV50_IR_MOD_NEG))
            ImmediateValue val;
            if (i->src(0).getImmediate(val))
         else if (i->src(1).getImmediate(val))
         else
            if ((i->src(s).mod | Modifier(NV50_IR_MOD_NEG)) != Modifier(NV50_IR_MOD_NEG))
            if (s == 1)
            Instruction *imm = i->getSrc(1)->getInsn();
   i->setSrc(1, imm->getSrc(0));
   if (post_ra_dead(imm))
      }
      bool
   PostRaLoadPropagation::visit(Instruction *i)
   {
      switch (i->op) {
   case OP_FMA:
   case OP_MAD:
      if (prog->getTarget()->getChipset() < 0xc0)
         else
            default:
                     }
      // =============================================================================
      // Common subexpression elimination. Stupid O^2 implementation.
   class LocalCSE : public Pass
   {
   private:
                           };
      class GlobalCSE : public Pass
   {
   private:
         };
      bool
   Instruction::isActionEqual(const Instruction *that) const
   {
      if (this->op != that->op ||
      this->dType != that->dType ||
   this->sType != that->sType)
      if (this->cc != that->cc)
            if (this->asTex()) {
      if (memcmp(&this->asTex()->tex,
            &that->asTex()->tex,
   } else
   if (this->asCmp()) {
      if (this->asCmp()->setCond != that->asCmp()->setCond)
      } else
   if (this->asFlow()) {
         } else
   if (this->op == OP_PHI && this->bb != that->bb) {
      /* TODO: we could probably be a bit smarter here by following the
   * control flow, but honestly, it is quite painful to check */
      } else {
      if (this->ipa != that->ipa ||
      this->lanes != that->lanes ||
   this->perPatch != that->perPatch)
      if (this->postFactor != that->postFactor)
               if (this->subOp != that->subOp ||
      this->saturate != that->saturate ||
   this->rnd != that->rnd ||
   this->ftz != that->ftz ||
   this->dnz != that->dnz ||
   this->cache != that->cache ||
   this->mask != that->mask)
            }
      bool
   Instruction::isResultEqual(const Instruction *that) const
   {
               // NOTE: location of discard only affects tex with liveOnly and quadops
   if (!this->defExists(0) && this->op != OP_DISCARD)
            if (!isActionEqual(that))
            if (this->predSrc != that->predSrc)
            for (d = 0; this->defExists(d); ++d) {
      if (!that->defExists(d) ||
      !this->getDef(d)->equals(that->getDef(d), false))
   }
   if (that->defExists(d))
            for (s = 0; this->srcExists(s); ++s) {
      if (!that->srcExists(s))
         if (this->src(s).mod != that->src(s).mod)
         if (!this->getSrc(s)->equals(that->getSrc(s), true))
      }
   if (that->srcExists(s))
            if (op == OP_LOAD || op == OP_VFETCH || op == OP_ATOM) {
      switch (src(0).getFile()) {
   case FILE_MEMORY_CONST:
   case FILE_SHADER_INPUT:
         case FILE_SHADER_OUTPUT:
         default:
                        }
      // pull through common expressions from different in-blocks
   bool
   GlobalCSE::visit(BasicBlock *bb)
   {
      Instruction *phi, *next, *ik;
                     for (phi = bb->getPhi(); phi && phi->op == OP_PHI; phi = next) {
      next = phi->next;
   if (phi->getSrc(0)->refCount() > 1)
         ik = phi->getSrc(0)->getInsn();
   if (!ik)
         if (ik->defCount(0xff) > 1)
         for (s = 1; phi->srcExists(s); ++s) {
      if (phi->getSrc(s)->refCount() > 1)
         if (!phi->getSrc(s)->getInsn() ||
      !phi->getSrc(s)->getInsn()->isResultEqual(ik))
   }
   if (!phi->srcExists(s)) {
      assert(ik->op != OP_PHI);
   Instruction *entry = bb->getEntry();
   ik->bb->remove(ik);
   if (!entry || entry->op != OP_JOIN)
         else
         ik->setDef(0, phi->getDef(0));
                     }
      bool
   LocalCSE::tryReplace(Instruction **ptr, Instruction *i)
   {
               // TODO: maybe relax this later (causes trouble with OP_UNION)
   if (i->isPredicated())
            if (!old->isResultEqual(i))
            for (int d = 0; old->defExists(d); ++d)
         delete_Instruction(prog, old);
   *ptr = NULL;
      }
      bool
   LocalCSE::visit(BasicBlock *bb)
   {
               do {
                        // will need to know the order of instructions
   int serial = 0;
   for (ir = bb->getFirst(); ir; ir = ir->next)
            for (ir = bb->getFirst(); ir; ir = next) {
                              if (ir->fixed) {
      ops[ir->op].insert(ir);
               for (s = 0; ir->srcExists(s); ++s)
      if (ir->getSrc(s)->asLValue())
               if (src) {
      for (Value::UseIterator it = src->uses.begin();
      it != src->uses.end(); ++it) {
   Instruction *ik = (*it)->getInsn();
   if (ik && ik->bb == ir->bb && ik->serial < ir->serial)
      if (tryReplace(&ir, ik))
      } else {
      DLLIST_FOR_EACH(&ops[ir->op], iter)
   {
      Instruction *ik = reinterpret_cast<Instruction *>(iter.get());
   if (tryReplace(&ir, ik))
                  if (ir)
         else
      }
   for (unsigned int i = 0; i <= OP_LAST; ++i)
                     }
      // =============================================================================
      // Remove computations of unused values.
   class DeadCodeElim : public Pass
   {
   public:
      DeadCodeElim() : deadCount(0) {}
         private:
                           };
      bool
   DeadCodeElim::buryAll(Program *prog)
   {
      do {
      deadCount = 0;
   if (!this->run(prog, false, false))
                  }
      bool
   DeadCodeElim::visit(BasicBlock *bb)
   {
               for (Instruction *i = bb->getExit(); i; i = prev) {
      prev = i->prev;
   if (i->isDead()) {
      ++deadCount;
      } else
   if (i->defExists(1) &&
      i->subOp == 0 &&
   (i->op == OP_VFETCH || i->op == OP_LOAD)) {
      } else
   if (i->defExists(0) && !i->getDef(0)->refCount()) {
      if (i->op == OP_ATOM ||
      i->op == OP_SUREDP ||
   i->op == OP_SUREDB) {
   const Target *targ = prog->getTarget();
   if (targ->getChipset() >= NVISA_GF100_CHIPSET ||
      i->subOp != NV50_IR_SUBOP_ATOM_CAS)
      if (i->op == OP_ATOM && i->subOp == NV50_IR_SUBOP_ATOM_EXCH) {
      i->cache = CACHE_CV;
   i->op = OP_STORE;
         } else if (i->op == OP_LOAD && i->subOp == NV50_IR_SUBOP_LOAD_LOCKED) {
      i->setDef(0, i->getDef(1));
            }
      }
      // Each load can go into up to 4 destinations, any of which might potentially
   // be dead (i.e. a hole). These can always be split into 2 loads, independent
   // of where the holes are. We find the first contiguous region, put it into
   // the first load, and then put the second contiguous region into the second
   // load. There can be at most 2 contiguous regions.
   //
   // Note that there are some restrictions, for example it's not possible to do
   // a 64-bit load that's not 64-bit aligned, so such a load has to be split
   // up. Also hardware doesn't support 96-bit loads, so those also have to be
   // split into a 64-bit and 32-bit load.
   void
   DeadCodeElim::checkSplitLoad(Instruction *ld1)
   {
      Instruction *ld2 = NULL; // can get at most 2 loads
   Value *def1[4];
   Value *def2[4];
   int32_t addr1, addr2;
   int32_t size1, size2;
   int d, n1, n2;
            for (d = 0; ld1->defExists(d); ++d)
      if (!ld1->getDef(d)->refCount() && ld1->getDef(d)->reg.data.id < 0)
      if (mask == 0xffffffff)
            addr1 = ld1->getSrc(0)->reg.data.offset;
   n1 = n2 = 0;
            // Compute address/width for first load
   for (d = 0; ld1->defExists(d); ++d) {
      if (mask & (1 << d)) {
      if (size1 && (addr1 & 0x7))
         def1[n1] = ld1->getDef(d);
      } else
   if (!n1) {
         } else {
                     // Scale back the size of the first load until it can be loaded. This
   // typically happens for TYPE_B96 loads.
   while (n1 &&
         !prog->getTarget()->isAccessSupported(ld1->getSrc(0)->reg.file,
      size1 -= def1[--n1]->reg.size;
               // Compute address/width for second load
   for (addr2 = addr1 + size1; ld1->defExists(d); ++d) {
      if (mask & (1 << d)) {
      assert(!size2 || !(addr2 & 0x7));
   def2[n2] = ld1->getDef(d);
      } else if (!n2) {
      assert(!n2);
      } else {
                     // Make sure that we've processed all the values
   for (; ld1->defExists(d); ++d)
            updateLdStOffset(ld1, addr1, func);
   ld1->setType(typeOfSize(size1));
   for (d = 0; d < 4; ++d)
            if (!n2)
            ld2 = cloneShallow(func, ld1);
   updateLdStOffset(ld2, addr2, func);
   ld2->setType(typeOfSize(size2));
   for (d = 0; d < 4; ++d)
               }
      // =============================================================================
      #define RUN_PASS(l, n, f)                       \
      if (level >= (l)) {                          \
      if (dbgFlags & NV50_IR_DEBUG_VERBOSE)     \
         n pass;                                   \
   if (!pass.f(this))                        \
            bool
   Program::optimizeSSA(int level)
   {
      RUN_PASS(1, DeadCodeElim, buryAll);
   RUN_PASS(1, CopyPropagation, run);
   RUN_PASS(1, MergeSplits, run);
   RUN_PASS(2, GlobalCSE, run);
   RUN_PASS(1, LocalCSE, run);
   RUN_PASS(2, AlgebraicOpt, run);
   RUN_PASS(2, ModifierFolding, run); // before load propagation -> less checks
   RUN_PASS(1, ConstantFolding, foldAll);
   RUN_PASS(0, Split64BitOpPreRA, run);
   RUN_PASS(2, LateAlgebraicOpt, run);
   RUN_PASS(1, LoadPropagation, run);
   RUN_PASS(1, IndirectPropagation, run);
   RUN_PASS(4, MemoryOpt, run);
   RUN_PASS(2, LocalCSE, run);
               }
      bool
   Program::optimizePostRA(int level)
   {
      RUN_PASS(2, FlatteningPass, run);
               }
      }
