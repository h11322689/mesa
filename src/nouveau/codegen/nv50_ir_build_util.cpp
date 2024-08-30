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
      namespace nv50_ir {
      BuildUtil::BuildUtil()
   {
         }
      BuildUtil::BuildUtil(Program *prog)
   {
         }
      void
   BuildUtil::init(Program *prog)
   {
               func = NULL;
   bb = NULL;
                     memset(imms, 0, sizeof(imms));
      }
      void
   BuildUtil::addImmediate(ImmediateValue *imm)
   {
      if (immCount > (NV50_IR_BUILD_IMM_HT_SIZE * 3) / 4)
                     while (imms[pos])
         imms[pos] = imm;
      }
      Instruction *
   BuildUtil::mkOp1(operation op, DataType ty, Value *dst, Value *src)
   {
               insn->setDef(0, dst);
            insert(insn);
      }
      Instruction *
   BuildUtil::mkOp2(operation op, DataType ty, Value *dst,
         {
               insn->setDef(0, dst);
   insn->setSrc(0, src0);
            insert(insn);
      }
      Instruction *
   BuildUtil::mkOp3(operation op, DataType ty, Value *dst,
         {
               insn->setDef(0, dst);
   insn->setSrc(0, src0);
   insn->setSrc(1, src1);
            insert(insn);
      }
      Instruction *
   BuildUtil::mkLoad(DataType ty, Value *dst, Symbol *mem, Value *ptr)
   {
               insn->setDef(0, dst);
   insn->setSrc(0, mem);
   if (ptr)
            insert(insn);
      }
      Instruction *
   BuildUtil::mkStore(operation op, DataType ty, Symbol *mem, Value *ptr,
         {
               insn->setSrc(0, mem);
   insn->setSrc(1, stVal);
   if (ptr)
            insert(insn);
      }
      Instruction *
   BuildUtil::mkFetch(Value *dst, DataType ty, DataFile file, int32_t offset,
         {
                        insn->setIndirect(0, 0, attrRel);
            // already inserted
      }
      Instruction *
   BuildUtil::mkInterp(unsigned mode, Value *dst, int32_t offset, Value *rel)
   {
      operation op = OP_LINTERP;
            if ((mode & NV50_IR_INTERP_MODE_MASK) == NV50_IR_INTERP_FLAT)
         else
   if ((mode & NV50_IR_INTERP_MODE_MASK) == NV50_IR_INTERP_PERSPECTIVE)
                     Instruction *insn = mkOp1(op, ty, dst, sym);
   insn->setIndirect(0, 0, rel);
   insn->setInterpolate(mode);
      }
      Instruction *
   BuildUtil::mkMov(Value *dst, Value *src, DataType ty)
   {
               insn->setDef(0, dst);
            insert(insn);
      }
      Instruction *
   BuildUtil::mkMovToReg(int id, Value *src)
   {
               insn->setDef(0, new_LValue(func, FILE_GPR));
   insn->getDef(0)->reg.data.id = id;
            insert(insn);
      }
      Instruction *
   BuildUtil::mkMovFromReg(Value *dst, int id)
   {
               insn->setDef(0, dst);
   insn->setSrc(0, new_LValue(func, FILE_GPR));
            insert(insn);
      }
      Instruction *
   BuildUtil::mkCvt(operation op,
         {
               insn->setType(dstTy, srcTy);
   insn->setDef(0, dst);
            insert(insn);
      }
      CmpInstruction *
   BuildUtil::mkCmp(operation op, CondCode cc, DataType dstTy, Value *dst,
         {
               insn->setType((dst->reg.file == FILE_PREDICATE ||
         insn->setCondition(cc);
   insn->setDef(0, dst);
   insn->setSrc(0, src0);
   insn->setSrc(1, src1);
   if (src2)
            if (dst->reg.file == FILE_FLAGS)
            insert(insn);
      }
      TexInstruction *
   BuildUtil::mkTex(operation op, TexTarget targ,
                     {
               for (size_t d = 0; d < def.size() && def[d]; ++d)
         for (size_t s = 0; s < src.size() && src[s]; ++s)
                     insert(tex);
      }
      Instruction *
   BuildUtil::mkQuadop(uint8_t q, Value *def, uint8_t l, Value *src0, Value *src1)
   {
      Instruction *quadop = mkOp2(OP_QUADOP, TYPE_F32, def, src0, src1);
   quadop->subOp = q;
   quadop->lanes = l;
      }
      Instruction *
   BuildUtil::mkSelect(Value *pred, Value *dst, Value *trSrc, Value *flSrc)
   {
      LValue *def0 = getSSA();
            mkMov(def0, trSrc)->setPredicate(CC_P, pred);
               }
      Instruction *
   BuildUtil::mkSplit(Value *h[2], uint8_t halfSize, Value *val)
   {
                        if (val->reg.file == FILE_IMMEDIATE)
            if (isMemoryFile(val->reg.file)) {
      h[0] = cloneShallow(getFunction(), val);
   h[1] = cloneShallow(getFunction(), val);
   h[0]->reg.size = halfSize;
   h[1]->reg.size = halfSize;
      } else {
      // The value might already be the result of a split, and further
   // splitting it can lead to issues regarding spill-offsets computations
   // among others. By forcing a move between the two splits, this can be
   // avoided.
   Instruction* valInsn = val->getInsn();
   if (valInsn && valInsn->op == OP_SPLIT)
            h[0] = getSSA(halfSize, val->reg.file);
   h[1] = getSSA(halfSize, val->reg.file);
   insn = mkOp1(OP_SPLIT, fTy, h[0], val);
      }
      }
      FlowInstruction *
   BuildUtil::mkFlow(operation op, void *targ, CondCode cc, Value *pred)
   {
               if (pred)
            insert(insn);
      }
      void
   BuildUtil::mkClobber(DataFile f, uint32_t rMask, int unit)
   {
      static const uint16_t baseSize2[16] =
   {
      0x0000, 0x0010, 0x0011, 0x0020, 0x0012, 0x1210, 0x1211, 0x1220,
                        for (; rMask; rMask >>= 4, base += 4) {
      const uint32_t mask = rMask & 0xf;
   if (!mask)
         int base1 = (baseSize2[mask] >>  0) & 0xf;
   int size1 = (baseSize2[mask] >>  4) & 0xf;
   int base2 = (baseSize2[mask] >>  8) & 0xf;
   int size2 = (baseSize2[mask] >> 12) & 0xf;
   Instruction *insn = mkOp(OP_NOP, TYPE_NONE, NULL);
   if (true) { // size1 can't be 0
      LValue *reg = new_LValue(func, f);
   reg->reg.size = size1 << unit;
   reg->reg.data.id = base + base1;
      }
   if (size2) {
      LValue *reg = new_LValue(func, f);
   reg->reg.size = size2 << unit;
   reg->reg.data.id = base + base2;
            }
      ImmediateValue *
   BuildUtil::mkImm(uint16_t u)
   {
               imm->reg.size = 2;
   imm->reg.type = TYPE_U16;
               }
      ImmediateValue *
   BuildUtil::mkImm(uint32_t u)
   {
               while (imms[pos] && imms[pos]->reg.data.u32 != u)
            ImmediateValue *imm = imms[pos];
   if (!imm) {
      imm = new_ImmediateValue(prog, u);
      }
      }
      ImmediateValue *
   BuildUtil::mkImm(uint64_t u)
   {
               imm->reg.size = 8;
   imm->reg.type = TYPE_U64;
               }
      ImmediateValue *
   BuildUtil::mkImm(float f)
   {
      union {
      float f32;
      } u;
   u.f32 = f;
      }
      ImmediateValue *
   BuildUtil::mkImm(double d)
   {
         }
      Value *
   BuildUtil::loadImm(Value *dst, float f)
   {
         }
      Value *
   BuildUtil::loadImm(Value *dst, double d)
   {
         }
      Value *
   BuildUtil::loadImm(Value *dst, uint16_t u)
   {
         }
      Value *
   BuildUtil::loadImm(Value *dst, uint32_t u)
   {
         }
      Value *
   BuildUtil::loadImm(Value *dst, uint64_t u)
   {
         }
      Symbol *
   BuildUtil::mkSymbol(DataFile file, int8_t fileIndex, DataType ty,
         {
               sym->setOffset(baseAddr);
   sym->reg.type = ty;
               }
      Symbol *
   BuildUtil::mkSysVal(SVSemantic svName, uint32_t svIndex)
   {
                        switch (svName) {
   case SV_POSITION:
   case SV_FACE:
   case SV_YDIR:
   case SV_POINT_SIZE:
   case SV_POINT_COORD:
   case SV_CLIP_DISTANCE:
   case SV_TESS_OUTER:
   case SV_TESS_INNER:
   case SV_TESS_COORD:
      sym->reg.type = TYPE_F32;
      default:
      sym->reg.type = TYPE_U32;
      }
            sym->reg.data.sv.sv = svName;
               }
      Symbol *
   BuildUtil::mkTSVal(TSSemantic tsName)
   {
      Symbol *sym = new_Symbol(prog, FILE_THREAD_STATE, 0);
   sym->reg.type = TYPE_U32;
   sym->reg.size = typeSizeof(sym->reg.type);
   sym->reg.data.ts = tsName;
      }
         Instruction *
   BuildUtil::split64BitOpPostRA(Function *fn, Instruction *i,
               {
      DataType hTy;
            switch (i->dType) {
   case TYPE_U64: hTy = TYPE_U32; break;
   case TYPE_S64: hTy = TYPE_S32; break;
   case TYPE_F64:
      if (i->op == OP_MOV) {
      hTy = TYPE_U32;
      }
      default:
                  switch (i->op) {
   case OP_MOV: srcNr = 1; break;
   case OP_ADD:
   case OP_SUB:
      if (!carry)
         srcNr = 2;
      case OP_SELP: srcNr = 3; break;
   default:
      // TODO when needed
               i->setType(hTy);
   i->setDef(0, cloneShallow(fn, i->getDef(0)));
   i->getDef(0)->reg.size = 4;
   Instruction *lo = i;
   Instruction *hi = cloneForward(fn, i);
                     for (int s = 0; s < srcNr; ++s) {
      if (lo->getSrc(s)->reg.size < 8) {
      if (s == 2)
         else
      } else {
      if (lo->getSrc(s)->refCount() > 1)
                        switch (hi->src(s).getFile()) {
   case FILE_IMMEDIATE:
      hi->getSrc(s)->reg.data.u64 >>= 32;
      case FILE_MEMORY_CONST:
   case FILE_MEMORY_SHARED:
   case FILE_SHADER_INPUT:
   case FILE_SHADER_OUTPUT:
      hi->getSrc(s)->reg.data.offset += 4;
      default:
      assert(hi->src(s).getFile() == FILE_GPR);
   hi->getSrc(s)->reg.data.id++;
            }
   if (srcNr == 2) {
      lo->setFlagsDef(1, carry);
      }
      }
      } // namespace nv50_ir
