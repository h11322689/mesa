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
   #include "nv50_ir_target_nv50.h"
      namespace nv50_ir {
      #define NV50_OP_ENC_LONG     0
   #define NV50_OP_ENC_SHORT    1
   #define NV50_OP_ENC_IMM      2
   #define NV50_OP_ENC_LONG_ALT 3
      class CodeEmitterNV50 : public CodeEmitter
   {
   public:
                                       private:
                     private:
      inline void defId(const ValueDef&, const int pos);
   inline void srcId(const ValueRef&, const int pos);
            inline void srcAddr16(const ValueRef&, bool adj, const int pos);
            void emitFlagsRd(const Instruction *);
                              void setAReg16(const Instruction *, int s);
            void setDst(const Value *);
   void setDst(const Instruction *, int d);
   void setSrcFileBits(const Instruction *, int enc);
            void emitForm_MAD(const Instruction *);
   void emitForm_ADD(const Instruction *);
   void emitForm_MUL(const Instruction *);
            void emitLoadStoreSizeLG(DataType ty, int pos);
            void roundMode_MAD(const Instruction *);
                     void emitLOAD(const Instruction *);
   void emitSTORE(const Instruction *);
   void emitMOV(const Instruction *);
   void emitRDSV(const Instruction *);
   void emitNOP();
   void emitINTERP(const Instruction *);
   void emitPFETCH(const Instruction *);
            void emitUADD(const Instruction *);
   void emitAADD(const Instruction *);
   void emitFADD(const Instruction *);
   void emitDADD(const Instruction *);
   void emitIMUL(const Instruction *);
   void emitFMUL(const Instruction *);
   void emitDMUL(const Instruction *);
   void emitFMAD(const Instruction *);
   void emitDMAD(const Instruction *);
   void emitIMAD(const Instruction *);
                     void emitPreOp(const Instruction *);
            void emitShift(const Instruction *);
   void emitARL(const Instruction *, unsigned int shl);
   void emitLogicOp(const Instruction *);
            void emitCVT(const Instruction *);
            void emitTEX(const TexInstruction *);
   void emitTXQ(const TexInstruction *);
                     void emitFlow(const Instruction *, uint8_t flowOp);
   void emitPRERETEmu(const FlowInstruction *);
               };
      #define SDATA(a) ((a).rep()->reg.data)
   #define DDATA(a) ((a).rep()->reg.data)
      void CodeEmitterNV50::srcId(const ValueRef& src, const int pos)
   {
      assert(src.get());
      }
      void CodeEmitterNV50::srcId(const ValueRef *src, const int pos)
   {
      assert(src->get());
      }
      void CodeEmitterNV50::srcAddr16(const ValueRef& src, bool adj, const int pos)
   {
                        assert(!adj || src.get()->reg.size <= 4);
   if (adj)
                     if (offset < 0)
               }
      void CodeEmitterNV50::srcAddr8(const ValueRef& src, const int pos)
   {
                                    }
      void CodeEmitterNV50::defId(const ValueDef& def, const int pos)
   {
                  }
      void
   CodeEmitterNV50::roundMode_MAD(const Instruction *insn)
   {
      switch (insn->rnd) {
   case ROUND_M: code[1] |= 1 << 22; break;
   case ROUND_P: code[1] |= 2 << 22; break;
   case ROUND_Z: code[1] |= 3 << 22; break;
   default:
      assert(insn->rnd == ROUND_N);
         }
      void
   CodeEmitterNV50::emitMNeg12(const Instruction *i)
   {
      code[1] |= i->src(0).mod.neg() << 26;
      }
      void CodeEmitterNV50::emitCondCode(CondCode cc, DataType ty, int pos)
   {
                        switch (cc) {
   case CC_LT:  enc = 0x1; break;
   case CC_LTU: enc = 0x9; break;
   case CC_EQ:  enc = 0x2; break;
   case CC_EQU: enc = 0xa; break;
   case CC_LE:  enc = 0x3; break;
   case CC_LEU: enc = 0xb; break;
   case CC_GT:  enc = 0x4; break;
   case CC_GTU: enc = 0xc; break;
   case CC_NE:  enc = 0x5; break;
   case CC_NEU: enc = 0xd; break;
   case CC_GE:  enc = 0x6; break;
   case CC_GEU: enc = 0xe; break;
   case CC_TR:  enc = 0xf; break;
            case CC_O:  enc = 0x10; break;
   case CC_C:  enc = 0x11; break;
   case CC_A:  enc = 0x12; break;
   case CC_S:  enc = 0x13; break;
   case CC_NS: enc = 0x1c; break;
   case CC_NA: enc = 0x1d; break;
   case CC_NC: enc = 0x1e; break;
            default:
      enc = 0;
   assert(!"invalid condition code");
      }
   if (ty != TYPE_NONE && !isFloatType(ty))
               }
      void
   CodeEmitterNV50::emitFlagsRd(const Instruction *i)
   {
                        if (s >= 0) {
      assert(i->getSrc(s)->reg.file == FILE_FLAGS);
   emitCondCode(i->cc, TYPE_NONE, 32 + 7);
      } else {
            }
      void
   CodeEmitterNV50::emitFlagsWr(const Instruction *i)
   {
                        // find flags definition and check that it is the last def
   if (flagsDef < 0) {
      for (int d = 0; i->defExists(d); ++d)
      if (i->def(d).getFile() == FILE_FLAGS)
      if (flagsDef >= 0 && false) // TODO: enforce use of flagsDef at some point
      }
   if (flagsDef == 0 && i->defExists(1))
            if (flagsDef >= 0)
         }
      void
   CodeEmitterNV50::setARegBits(unsigned int u)
   {
      code[0] |= (u & 3) << 26;
      }
      void
   CodeEmitterNV50::setAReg16(const Instruction *i, int s)
   {
      if (i->srcExists(s)) {
      s = i->src(s).indirect[0];
   if (s >= 0)
         }
      void
   CodeEmitterNV50::setImmediate(const Instruction *i, int s)
   {
      const ImmediateValue *imm = i->src(s).get()->asImm();
                     if (i->src(s).mod & Modifier(NV50_IR_MOD_NOT))
            code[1] |= 3;
   code[0] |= (u & 0x3f) << 16;
      }
      void
   CodeEmitterNV50::setDst(const Value *dst)
   {
                        if (reg->data.id < 0 || reg->file == FILE_FLAGS) {
      code[0] |= (127 << 2) | 1;
      } else {
      int id;
   if (reg->file == FILE_SHADER_OUTPUT) {
      code[1] |= 8;
      } else {
         }
         }
      void
   CodeEmitterNV50::setDst(const Instruction *i, int d)
   {
      if (i->defExists(d)) {
         } else
   if (!d) {
      code[0] |= 0x01fc; // bit bucket
         }
      // 3 * 2 bits:
   // 0: r
   // 1: a/s
   // 2: c
   // 3: i
   void
   CodeEmitterNV50::setSrcFileBits(const Instruction *i, int enc)
   {
               for (unsigned int s = 0; s < Target::operationSrcNr[i->op]; ++s) {
      switch (i->src(s).getFile()) {
   case FILE_GPR:
         case FILE_MEMORY_SHARED:
   case FILE_SHADER_INPUT:
      mode |= 1 << (s * 2);
      case FILE_MEMORY_CONST:
      mode |= 2 << (s * 2);
      case FILE_IMMEDIATE:
      mode |= 3 << (s * 2);
      default:
      ERROR("invalid file on source %i: %u\n", s, i->src(s).getFile());
   assert(0);
         }
   switch (mode) {
   case 0x00: // rrr
         case 0x01: // arr/grr
      if (progType == Program::TYPE_GEOMETRY && i->src(0).isIndirect(0)) {
      code[0] |= 0x01800000;
   if (enc == NV50_OP_ENC_LONG || enc == NV50_OP_ENC_LONG_ALT)
      } else {
      if (enc == NV50_OP_ENC_SHORT)
         else
      }
      case 0x03: // irr
      assert(i->op == OP_MOV);
      case 0x0c: // rir
         case 0x0d: // gir
      assert(progType == Program::TYPE_GEOMETRY ||
         code[0] |= 0x01000000;
   if (progType == Program::TYPE_GEOMETRY && i->src(0).isIndirect(0)) {
      int reg = i->src(0).getIndirect(0)->rep()->reg.data.id;
   assert(reg < 3);
      }
      case 0x08: // rcr
      code[0] |= (enc == NV50_OP_ENC_LONG_ALT) ? 0x01000000 : 0x00800000;
   code[1] |= (i->getSrc(1)->reg.fileIndex << 22);
      case 0x09: // acr/gcr
      if (progType == Program::TYPE_GEOMETRY && i->src(0).isIndirect(0)) {
         } else {
      code[0] |= (enc == NV50_OP_ENC_LONG_ALT) ? 0x01000000 : 0x00800000;
      }
   code[1] |= (i->getSrc(1)->reg.fileIndex << 22);
      case 0x20: // rrc
      code[0] |= 0x01000000;
   code[1] |= (i->getSrc(2)->reg.fileIndex << 22);
      case 0x21: // arc
      code[0] |= 0x01000000;
   code[1] |= 0x00200000 | (i->getSrc(2)->reg.fileIndex << 22);
   assert(progType != Program::TYPE_GEOMETRY);
      default:
      ERROR("not encodable: %x\n", mode);
   assert(0);
      }
   if (progType != Program::TYPE_COMPUTE)
            if ((mode & 3) == 1) {
               switch (i->sType) {
   case TYPE_U8:
         case TYPE_U16:
      code[0] |= 1 << pos;
      case TYPE_S16:
      code[0] |= 2 << pos;
      default:
      code[0] |= 3 << pos;
   assert(i->getSrc(0)->reg.size == 4);
            }
      void
   CodeEmitterNV50::setSrc(const Instruction *i, unsigned int s, int slot)
   {
      if (Target::operationSrcNr[i->op] <= s)
                  unsigned int id = (reg->file == FILE_GPR) ?
      reg->data.id :
         switch (slot) {
   case 0: code[0] |= id << 9; break;
   case 1: code[0] |= id << 16; break;
   case 2: code[1] |= id << 14; break;
   default:
      assert(0);
         }
      // the default form:
   //  - long instruction
   //  - 1 to 3 sources in slots 0, 1, 2 (rrr, arr, rcr, acr, rrc, arc, gcr, grr)
   //  - address & flags
   void
   CodeEmitterNV50::emitForm_MAD(const Instruction *i)
   {
      assert(i->encSize == 8);
            emitFlagsRd(i);
                     setSrcFileBits(i, NV50_OP_ENC_LONG);
   setSrc(i, 0, 0);
   setSrc(i, 1, 1);
            if (i->getIndirect(0, 0)) {
      assert(!i->srcExists(1) || !i->getIndirect(1, 0));
   assert(!i->srcExists(2) || !i->getIndirect(2, 0));
      } else if (i->srcExists(1) && i->getIndirect(1, 0)) {
      assert(!i->srcExists(2) || !i->getIndirect(2, 0));
      } else {
            }
      // like default form, but 2nd source in slot 2, and no 3rd source
   void
   CodeEmitterNV50::emitForm_ADD(const Instruction *i)
   {
      assert(i->encSize == 8);
            emitFlagsRd(i);
                     setSrcFileBits(i, NV50_OP_ENC_LONG_ALT);
   setSrc(i, 0, 0);
   if (i->predSrc != 1)
            if (i->getIndirect(0, 0)) {
      assert(!i->getIndirect(1, 0));
      } else {
            }
      // default short form (rr, ar, rc, gr)
   void
   CodeEmitterNV50::emitForm_MUL(const Instruction *i)
   {
      assert(i->encSize == 4 && !(code[0] & 1));
   assert(i->defExists(0));
                     setSrcFileBits(i, NV50_OP_ENC_SHORT);
   setSrc(i, 0, 0);
      }
      // usual immediate form
   // - 1 to 3 sources where second is immediate (rir, gir)
   // - no address or predicate possible
   void
   CodeEmitterNV50::emitForm_IMM(const Instruction *i)
   {
      assert(i->encSize == 8);
                              setSrcFileBits(i, NV50_OP_ENC_IMM);
   if (Target::operationSrcNr[i->op] > 1) {
      setSrc(i, 0, 0);
   setImmediate(i, 1);
      } else {
            }
      void
   CodeEmitterNV50::emitLoadStoreSizeLG(DataType ty, int pos)
   {
               switch (ty) {
   case TYPE_F32: // fall through
   case TYPE_S32: // fall through
   case TYPE_U32:  enc = 0x6; break;
   case TYPE_B128: enc = 0x5; break;
   case TYPE_F64: // fall through
   case TYPE_S64: // fall through
   case TYPE_U64:  enc = 0x4; break;
   case TYPE_S16:  enc = 0x3; break;
   case TYPE_U16:  enc = 0x2; break;
   case TYPE_S8:   enc = 0x1; break;
   case TYPE_U8:   enc = 0x0; break;
   default:
      enc = 0;
   assert(!"invalid load/store type");
      }
      }
      void
   CodeEmitterNV50::emitLoadStoreSizeCS(DataType ty)
   {
      switch (ty) {
   case TYPE_U8: break;
   case TYPE_U16: code[1] |= 0x4000; break;
   case TYPE_S16: code[1] |= 0x8000; break;
   case TYPE_F32:
   case TYPE_S32:
   case TYPE_U32: code[1] |= 0xc000; break;
   default:
      assert(0);
         }
      void
   CodeEmitterNV50::emitLOAD(const Instruction *i)
   {
      DataFile sf = i->src(0).getFile();
            switch (sf) {
   case FILE_SHADER_INPUT:
      if (progType == Program::TYPE_GEOMETRY && i->src(0).isIndirect(0))
         else
      // use 'mov' where we can
      code[1] = 0x00200000 | (i->lanes << 14);
   if (typeSizeof(i->dType) == 4)
            case FILE_MEMORY_SHARED:
      if (targ->getChipset() >= 0x84) {
      assert(offset <= (int32_t)(0x3fff * typeSizeof(i->sType)));
                                          if (i->subOp == NV50_IR_SUBOP_LOAD_LOCKED)
      } else {
      assert(offset <= (int32_t)(0x1f * typeSizeof(i->sType)));
   code[0] = 0x10000001;
   code[1] = 0x00200000 | (i->lanes << 14);
      }
      case FILE_MEMORY_CONST:
      code[0] = 0x10000001;
   code[1] = 0x20000000 | (i->getSrc(0)->reg.fileIndex << 22);
   if (typeSizeof(i->dType) == 4)
         emitLoadStoreSizeCS(i->sType);
      case FILE_MEMORY_LOCAL:
      code[0] = 0xd0000001;
   code[1] = 0x40000000;
      case FILE_MEMORY_GLOBAL:
      code[0] = 0xd0000001 | (i->getSrc(0)->reg.fileIndex << 16);
   code[1] = 0x80000000;
      default:
      assert(!"invalid load source file");
      }
   if (sf == FILE_MEMORY_LOCAL ||
      sf == FILE_MEMORY_GLOBAL)
                  emitFlagsRd(i);
            if (i->src(0).getFile() == FILE_MEMORY_GLOBAL) {
         } else {
      setAReg16(i, 0);
         }
      void
   CodeEmitterNV50::emitSTORE(const Instruction *i)
   {
      DataFile f = i->getSrc(0)->reg.file;
            switch (f) {
   case FILE_SHADER_OUTPUT:
      code[0] = 0x00000001 | ((offset >> 2) << 9);
   code[1] = 0x80c00000;
   srcId(i->src(1), 32 + 14);
      case FILE_MEMORY_GLOBAL:
      code[0] = 0xd0000001 | (i->getSrc(0)->reg.fileIndex << 16);
   code[1] = 0xa0000000;
   emitLoadStoreSizeLG(i->dType, 21 + 32);
   srcId(i->src(1), 2);
      case FILE_MEMORY_LOCAL:
      code[0] = 0xd0000001;
   code[1] = 0x60000000;
   emitLoadStoreSizeLG(i->dType, 21 + 32);
   srcId(i->src(1), 2);
      case FILE_MEMORY_SHARED:
      code[0] = 0x00000001;
   code[1] = 0xe0000000;
   if (i->subOp == NV50_IR_SUBOP_STORE_UNLOCKED)
         switch (typeSizeof(i->dType)) {
   case 1:
      code[0] |= offset << 9;
   code[1] |= 0x00400000;
      case 2:
      code[0] |= (offset >> 1) << 9;
      case 4:
      code[0] |= (offset >> 2) << 9;
   code[1] |= 0x04200000;
      default:
      assert(0);
      }
   srcId(i->src(1), 32 + 14);
      default:
      assert(!"invalid store destination file");
               if (f == FILE_MEMORY_GLOBAL)
         else
            if (f == FILE_MEMORY_LOCAL)
               }
      void
   CodeEmitterNV50::emitMOV(const Instruction *i)
   {
      DataFile sf = i->getSrc(0)->reg.file;
                     if (sf == FILE_FLAGS) {
      assert(i->flagsSrc >= 0);
   code[0] = 0x00000001;
   code[1] = 0x20000000;
   defId(i->def(0), 2);
      } else
   if (sf == FILE_ADDRESS) {
      code[0] = 0x00000001;
   code[1] = 0x40000000;
   defId(i->def(0), 2);
   setARegBits(SDATA(i->src(0)).id + 1);
      } else
   if (df == FILE_FLAGS) {
      assert(i->flagsDef >= 0);
   code[0] = 0x00000001;
   code[1] = 0xa0000000;
   srcId(i->src(0), 9);
   emitFlagsRd(i);
      } else
   if (sf == FILE_IMMEDIATE) {
      code[0] = 0x10000001;
   code[1] = 0x00000003;
               } else {
      if (i->encSize == 4) {
      code[0] = 0x10000000;
   code[0] |= (typeSizeof(i->dType) == 2) ? 0 : 0x00008000;
      } else {
      code[0] = 0x10000001;
   code[1] = (typeSizeof(i->dType) == 2) ? 0 : 0x04000000;
   code[1] |= (i->lanes << 14);
   setDst(i, 0);
      }
      }
   if (df == FILE_SHADER_OUTPUT) {
      assert(i->encSize == 8);
         }
      static inline uint8_t getSRegEncoding(const ValueRef &ref)
   {
      switch (SDATA(ref).sv.sv) {
   case SV_PHYSID:        return 0;
   case SV_CLOCK:         return 1;
      // case SV_PM_COUNTER:    return 4 + SDATA(ref).sv.index;
      case SV_SAMPLE_INDEX:  return 8;
   default:
      assert(!"no sreg for system value");
         }
      void
   CodeEmitterNV50::emitRDSV(const Instruction *i)
   {
      code[0] = 0x00000001;
   code[1] = 0x60000000 | (getSRegEncoding(i->src(0)) << 14);
   defId(i->def(0), 2);
      }
      void
   CodeEmitterNV50::emitNOP()
   {
      code[0] = 0xf0000001;
      }
      void
   CodeEmitterNV50::emitQUADOP(const Instruction *i, uint8_t lane, uint8_t quOp)
   {
      code[0] = 0xc0000000 | (lane << 16);
            code[0] |= (quOp & 0x03) << 20;
                     if (!i->srcExists(1) || i->predSrc == 1)
      }
      /* NOTE: This returns the base address of a vertex inside the primitive.
   * src0 is an immediate, the index (not offset) of the vertex
   * inside the primitive. XXX: signed or unsigned ?
   * src1 (may be NULL) should use whatever units the hardware requires
   * (on nv50 this is bytes, so, relative index * 4; signed 16 bit value).
   */
   void
   CodeEmitterNV50::emitPFETCH(const Instruction *i)
   {
      const uint32_t prim = i->src(0).get()->reg.data.u32;
            if (i->def(0).getFile() == FILE_ADDRESS) {
      // shl $aX a[] 0
   code[0] = 0x00000001 | ((DDATA(i->def(0)).id + 1) << 2);
   code[1] = 0xc0200000;
   code[0] |= prim << 9;
      } else
   if (i->srcExists(1)) {
      // ld b32 $rX a[$aX+base]
   code[0] = 0x00000001;
   code[1] = 0x04200000 | (0xf << 14);
   defId(i->def(0), 2);
   code[0] |= prim << 9;
      } else {
      // mov b32 $rX a[]
   code[0] = 0x10000001;
   code[1] = 0x04200000 | (0xf << 14);
   defId(i->def(0), 2);
      }
      }
      void
   nv50_interpApply(const FixupEntry *entry, uint32_t *code, const FixupData& data)
   {
      int ipa = entry->ipa;
   int encSize = entry->reg;
            if ((ipa & NV50_IR_INTERP_SAMPLE_MASK) == NV50_IR_INTERP_DEFAULT &&
      (ipa & NV50_IR_INTERP_MODE_MASK) != NV50_IR_INTERP_FLAT) {
   if (data.force_persample_interp) {
      if (encSize == 8)
         else
      } else {
      if (encSize == 8)
         else
            }
      void
   CodeEmitterNV50::emitINTERP(const Instruction *i)
   {
               defId(i->def(0), 2);
   srcAddr8(i->src(0), 16);
            if (i->encSize != 8 && i->getInterpMode() == NV50_IR_INTERP_FLAT) {
         } else {
      if (i->op == OP_PINTERP) {
      code[0] |= 1 << 25;
      }
   if (i->getSampleMode() == NV50_IR_INTERP_CENTROID)
               if (i->encSize == 8) {
      if (i->getInterpMode() == NV50_IR_INTERP_FLAT)
         else
         code[0] &= ~0x03000000;
   code[0] |= 1;
                  }
      void
   CodeEmitterNV50::emitMINMAX(const Instruction *i)
   {
      if (i->dType == TYPE_F64) {
      code[0] = 0xe0000000;
      } else {
      code[0] = 0x30000000;
   code[1] = 0x80000000;
   if (i->op == OP_MIN)
            switch (i->dType) {
   case TYPE_F32: code[0] |= 0x80000000; break;
   case TYPE_S32: code[1] |= 0x8c000000; break;
   case TYPE_U32: code[1] |= 0x84000000; break;
   case TYPE_S16: code[1] |= 0x80000000; break;
   case TYPE_U16: break;
   default:
      assert(0);
                  code[1] |= i->src(0).mod.abs() << 20;
   code[1] |= i->src(0).mod.neg() << 26;
   code[1] |= i->src(1).mod.abs() << 19;
               }
      void
   CodeEmitterNV50::emitFMAD(const Instruction *i)
   {
      const int neg_mul = i->src(0).mod.neg() ^ i->src(1).mod.neg();
                     if (i->src(1).getFile() == FILE_IMMEDIATE) {
      code[1] = 0;
   emitForm_IMM(i);
   code[0] |= neg_mul << 15;
   code[0] |= neg_add << 22;
   if (i->saturate)
      } else
   if (i->encSize == 4) {
      emitForm_MUL(i);
   code[0] |= neg_mul << 15;
   code[0] |= neg_add << 22;
   if (i->saturate)
      } else {
      code[1]  = neg_mul << 26;
   code[1] |= neg_add << 27;
   if (i->saturate)
               }
      void
   CodeEmitterNV50::emitDMAD(const Instruction *i)
   {
      const int neg_mul = i->src(0).mod.neg() ^ i->src(1).mod.neg();
            assert(i->encSize == 8);
            code[1] = 0x40000000;
            code[1] |= neg_mul << 26;
                        }
      void
   CodeEmitterNV50::emitFADD(const Instruction *i)
   {
      const int neg0 = i->src(0).mod.neg();
                              if (i->src(1).getFile() == FILE_IMMEDIATE) {
      code[1] = 0;
   emitForm_IMM(i);
   code[0] |= neg0 << 15;
   code[0] |= neg1 << 22;
   if (i->saturate)
      } else
   if (i->encSize == 8) {
      code[1] = 0;
   emitForm_ADD(i);
   code[1] |= neg0 << 26;
   code[1] |= neg1 << 27;
   if (i->saturate)
      } else {
      emitForm_MUL(i);
   code[0] |= neg0 << 15;
   code[0] |= neg1 << 22;
   if (i->saturate)
         }
      void
   CodeEmitterNV50::emitDADD(const Instruction *i)
   {
      const int neg0 = i->src(0).mod.neg();
            assert(!(i->src(0).mod | i->src(1).mod).abs());
   assert(!i->saturate);
            code[1] = 0x60000000;
                     code[1] |= neg0 << 26;
      }
      void
   CodeEmitterNV50::emitUADD(const Instruction *i)
   {
      const int neg0 = i->src(0).mod.neg();
                     if (i->src(1).getFile() == FILE_IMMEDIATE) {
      code[0] |= (typeSizeof(i->dType) == 2) ? 0 : 0x00008000;
   code[1] = 0;
      } else
   if (i->encSize == 8) {
      code[1] = (typeSizeof(i->dType) == 2) ? 0 : 0x04000000;
      } else {
      code[0] |= (typeSizeof(i->dType) == 2) ? 0 : 0x00008000;
      }
   assert(!(neg0 && neg1));
   code[0] |= neg0 << 28;
            if (i->flagsSrc >= 0) {
      // addc == sub | subr
   assert(!(code[0] & 0x10400000) && !i->getPredicate());
   code[0] |= 0x10400000;
         }
      void
   CodeEmitterNV50::emitAADD(const Instruction *i)
   {
               code[0] = 0xd0000001 | (i->getSrc(s)->reg.data.u16 << 9);
                              if (s && i->srcExists(0))
      }
      void
   CodeEmitterNV50::emitIMUL(const Instruction *i)
   {
               if (i->src(1).getFile() == FILE_IMMEDIATE) {
      if (i->sType == TYPE_S16)
         code[1] = 0;
      } else
   if (i->encSize == 8) {
      code[1] = (i->sType == TYPE_S16) ? (0x8000 | 0x4000) : 0x0000;
      } else {
      if (i->sType == TYPE_S16)
               }
      void
   CodeEmitterNV50::emitFMUL(const Instruction *i)
   {
                        if (i->src(1).getFile() == FILE_IMMEDIATE) {
      code[1] = 0;
   emitForm_IMM(i);
   if (neg)
         if (i->saturate)
      } else
   if (i->encSize == 8) {
      code[1] = i->rnd == ROUND_Z ? 0x0000c000 : 0;
   if (neg)
         if (i->saturate)
            } else {
      emitForm_MUL(i);
   if (neg)
         if (i->saturate)
         }
      void
   CodeEmitterNV50::emitDMUL(const Instruction *i)
   {
               assert(!i->saturate);
            code[1] = 0x80000000;
            if (neg)
                        }
      void
   CodeEmitterNV50::emitIMAD(const Instruction *i)
   {
      int mode;
            assert(!i->src(0).mod && !i->src(1).mod && !i->src(2).mod);
   if (!isSignedType(i->sType))
         else if (i->saturate)
         else
            if (i->src(1).getFile() == FILE_IMMEDIATE) {
      code[1] = 0;
   emitForm_IMM(i);
   code[0] |= (mode & 1) << 8 | (mode & 2) << 14;
   if (i->flagsSrc >= 0) {
      assert(!(code[0] & 0x10400000));
   assert(SDATA(i->src(i->flagsSrc)).id == 0);
         } else
   if (i->encSize == 4) {
      emitForm_MUL(i);
   code[0] |= (mode & 1) << 8 | (mode & 2) << 14;
   if (i->flagsSrc >= 0) {
      assert(!(code[0] & 0x10400000));
   assert(SDATA(i->src(i->flagsSrc)).id == 0);
         } else {
      code[1] = mode << 29;
            if (i->flagsSrc >= 0) {
      // add with carry from $cX
   assert(!(code[1] & 0x0c000000) && !i->getPredicate());
   code[1] |= 0xc << 24;
            }
      void
   CodeEmitterNV50::emitISAD(const Instruction *i)
   {
      if (i->encSize == 8) {
      code[0] = 0x50000000;
   switch (i->sType) {
   case TYPE_U32: code[1] = 0x04000000; break;
   case TYPE_S32: code[1] = 0x0c000000; break;
   case TYPE_U16: code[1] = 0x00000000; break;
   case TYPE_S16: code[1] = 0x08000000; break;
   default:
      assert(0);
      }
      } else {
      switch (i->sType) {
   case TYPE_U32: code[0] = 0x50008000; break;
   case TYPE_S32: code[0] = 0x50008100; break;
   case TYPE_U16: code[0] = 0x50000000; break;
   case TYPE_S16: code[0] = 0x50000100; break;
   default:
      assert(0);
      }
         }
      static void
   alphatestSet(const FixupEntry *entry, uint32_t *code, const FixupData& data)
   {
      int loc = entry->loc;
            switch (data.alphatest) {
   case PIPE_FUNC_NEVER: enc = 0x0; break;
   case PIPE_FUNC_LESS: enc = 0x1; break;
   case PIPE_FUNC_EQUAL: enc = 0x2; break;
   case PIPE_FUNC_LEQUAL: enc = 0x3; break;
   case PIPE_FUNC_GREATER: enc = 0x4; break;
   case PIPE_FUNC_NOTEQUAL: enc = 0x5; break;
   case PIPE_FUNC_GEQUAL: enc = 0x6; break;
   default:
   case PIPE_FUNC_ALWAYS: enc = 0xf; break;
            code[loc + 1] &= ~(0x1f << 14);
      }
      void
   CodeEmitterNV50::emitSET(const Instruction *i)
   {
      code[0] = 0x30000000;
            switch (i->sType) {
   case TYPE_F64:
      code[0] = 0xe0000000;
   code[1] = 0xe0000000;
      case TYPE_F32: code[0] |= 0x80000000; break;
   case TYPE_S32: code[1] |= 0x0c000000; break;
   case TYPE_U32: code[1] |= 0x04000000; break;
   case TYPE_S16: code[1] |= 0x08000000; break;
   case TYPE_U16: break;
   default:
      assert(0);
                        if (i->src(0).mod.neg()) code[1] |= 0x04000000;
   if (i->src(1).mod.neg()) code[1] |= 0x08000000;
   if (i->src(0).mod.abs()) code[1] |= 0x00100000;
                     if (i->subOp == 1) {
            }
      void
   CodeEmitterNV50::roundMode_CVT(RoundMode rnd)
   {
      switch (rnd) {
   case ROUND_NI: code[1] |= 0x08000000; break;
   case ROUND_M:  code[1] |= 0x00020000; break;
   case ROUND_MI: code[1] |= 0x08020000; break;
   case ROUND_P:  code[1] |= 0x00040000; break;
   case ROUND_PI: code[1] |= 0x08040000; break;
   case ROUND_Z:  code[1] |= 0x00060000; break;
   case ROUND_ZI: code[1] |= 0x08060000; break;
   default:
      assert(rnd == ROUND_N);
         }
      void
   CodeEmitterNV50::emitCVT(const Instruction *i)
   {
      const bool f2f = isFloatType(i->dType) && isFloatType(i->sType);
   RoundMode rnd;
            switch (i->op) {
   case OP_CEIL:  rnd = f2f ? ROUND_PI : ROUND_P; break;
   case OP_FLOOR: rnd = f2f ? ROUND_MI : ROUND_M; break;
   case OP_TRUNC: rnd = f2f ? ROUND_ZI : ROUND_Z; break;
   default:
      rnd = i->rnd;
               if (i->op == OP_NEG && i->dType == TYPE_U32)
         else
                     switch (dType) {
   case TYPE_F64:
      switch (i->sType) {
   case TYPE_F64: code[1] = 0xc4404000; break;
   case TYPE_S64: code[1] = 0x44414000; break;
   case TYPE_U64: code[1] = 0x44404000; break;
   case TYPE_F32: code[1] = 0xc4400000; break;
   case TYPE_S32: code[1] = 0x44410000; break;
   case TYPE_U32: code[1] = 0x44400000; break;
   default:
      assert(0);
      }
      case TYPE_S64:
      switch (i->sType) {
   case TYPE_F64: code[1] = 0x8c404000; break;
   case TYPE_F32: code[1] = 0x8c400000; break;
   default:
      assert(0);
      }
      case TYPE_U64:
      switch (i->sType) {
   case TYPE_F64: code[1] = 0x84404000; break;
   case TYPE_F32: code[1] = 0x84400000; break;
   default:
      assert(0);
      }
      case TYPE_F32:
      switch (i->sType) {
   case TYPE_F64: code[1] = 0xc0404000; break;
   case TYPE_S64: code[1] = 0x40414000; break;
   case TYPE_U64: code[1] = 0x40404000; break;
   case TYPE_F32: code[1] = 0xc4004000; break;
   case TYPE_S32: code[1] = 0x44014000; break;
   case TYPE_U32: code[1] = 0x44004000; break;
   case TYPE_F16: code[1] = 0xc4000000; break;
   case TYPE_U16: code[1] = 0x44000000; break;
   case TYPE_S16: code[1] = 0x44010000; break;
   case TYPE_S8:  code[1] = 0x44018000; break;
   case TYPE_U8:  code[1] = 0x44008000; break;
   default:
      assert(0);
      }
      case TYPE_S32:
      switch (i->sType) {
   case TYPE_F64: code[1] = 0x88404000; break;
   case TYPE_F32: code[1] = 0x8c004000; break;
   case TYPE_S32: code[1] = 0x0c014000; break;
   case TYPE_U32: code[1] = 0x0c004000; break;
   case TYPE_F16: code[1] = 0x8c000000; break;
   case TYPE_S16: code[1] = 0x0c010000; break;
   case TYPE_U16: code[1] = 0x0c000000; break;
   case TYPE_S8:  code[1] = 0x0c018000; break;
   case TYPE_U8:  code[1] = 0x0c008000; break;
   default:
      assert(0);
      }
      case TYPE_U32:
      switch (i->sType) {
   case TYPE_F64: code[1] = 0x80404000; break;
   case TYPE_F32: code[1] = 0x84004000; break;
   case TYPE_S32: code[1] = 0x04014000; break;
   case TYPE_U32: code[1] = 0x04004000; break;
   case TYPE_F16: code[1] = 0x84000000; break;
   case TYPE_S16: code[1] = 0x04010000; break;
   case TYPE_U16: code[1] = 0x04000000; break;
   case TYPE_S8:  code[1] = 0x04018000; break;
   case TYPE_U8:  code[1] = 0x04008000; break;
   default:
      assert(0);
      }
      case TYPE_F16:
      switch (i->sType) {
   case TYPE_F16: code[1] = 0xc0000000; break;
   case TYPE_F32: code[1] = 0xc0004000; break;
   default:
      assert(0);
      }
      case TYPE_S16:
      switch (i->sType) {
   case TYPE_F32: code[1] = 0x88004000; break;
   case TYPE_S32: code[1] = 0x08014000; break;
   case TYPE_U32: code[1] = 0x08004000; break;
   case TYPE_F16: code[1] = 0x88000000; break;
   case TYPE_S16: code[1] = 0x08010000; break;
   case TYPE_U16: code[1] = 0x08000000; break;
   case TYPE_S8:  code[1] = 0x08018000; break;
   case TYPE_U8:  code[1] = 0x08008000; break;
   default:
      assert(0);
      }
      case TYPE_U16:
      switch (i->sType) {
   case TYPE_F32: code[1] = 0x80004000; break;
   case TYPE_S32: code[1] = 0x00014000; break;
   case TYPE_U32: code[1] = 0x00004000; break;
   case TYPE_F16: code[1] = 0x80000000; break;
   case TYPE_S16: code[1] = 0x00010000; break;
   case TYPE_U16: code[1] = 0x00000000; break;
   case TYPE_S8:  code[1] = 0x00018000; break;
   case TYPE_U8:  code[1] = 0x00008000; break;
   default:
      assert(0);
      }
      case TYPE_S8:
      switch (i->sType) {
   case TYPE_S32: code[1] = 0x08094000; break;
   case TYPE_U32: code[1] = 0x08084000; break;
   case TYPE_F16: code[1] = 0x88080000; break;
   case TYPE_S16: code[1] = 0x08090000; break;
   case TYPE_U16: code[1] = 0x08080000; break;
   case TYPE_S8:  code[1] = 0x08098000; break;
   case TYPE_U8:  code[1] = 0x08088000; break;
   default:
      assert(0);
      }
      case TYPE_U8:
      switch (i->sType) {
   case TYPE_S32: code[1] = 0x00094000; break;
   case TYPE_U32: code[1] = 0x00084000; break;
   case TYPE_F16: code[1] = 0x80080000; break;
   case TYPE_S16: code[1] = 0x00090000; break;
   case TYPE_U16: code[1] = 0x00080000; break;
   case TYPE_S8:  code[1] = 0x00098000; break;
   case TYPE_U8:  code[1] = 0x00088000; break;
   default:
      assert(0);
      }
      default:
      assert(0);
      }
   if (typeSizeof(i->sType) == 1 && i->getSrc(0)->reg.size == 4)
                     switch (i->op) {
   case OP_ABS: code[1] |= 1 << 20; break;
   case OP_SAT: code[1] |= 1 << 19; break;
   case OP_NEG: code[1] |= 1 << 29; break;
   default:
         }
   code[1] ^= i->src(0).mod.neg() << 29;
   code[1] |= i->src(0).mod.abs() << 20;
   if (i->saturate)
                        }
      void
   CodeEmitterNV50::emitPreOp(const Instruction *i)
   {
      code[0] = 0xb0000000;
            code[1] |= i->src(0).mod.abs() << 20;
               }
      void
   CodeEmitterNV50::emitSFnOp(const Instruction *i, uint8_t subOp)
   {
               if (i->encSize == 4) {
      assert(i->op == OP_RCP);
   assert(!i->saturate);
   code[0] |= i->src(0).mod.abs() << 15;
   code[0] |= i->src(0).mod.neg() << 22;
      } else {
      code[1] = subOp << 29;
   code[1] |= i->src(0).mod.abs() << 20;
   code[1] |= i->src(0).mod.neg() << 26;
   if (i->saturate) {
      assert(subOp == 6 && i->op == OP_EX2);
      }
         }
      void
   CodeEmitterNV50::emitNOT(const Instruction *i)
   {
      code[0] = 0xd0000000;
            switch (i->sType) {
   case TYPE_U32:
   case TYPE_S32:
      code[1] |= 0x04000000;
      default:
         }
   emitForm_MAD(i);
      }
      void
   CodeEmitterNV50::emitLogicOp(const Instruction *i)
   {
      code[0] = 0xd0000000;
            if (i->src(1).getFile() == FILE_IMMEDIATE) {
      switch (i->op) {
   case OP_OR:  code[0] |= 0x0100; break;
   case OP_XOR: code[0] |= 0x8000; break;
   default:
      assert(i->op == OP_AND);
      }
   if (i->src(0).mod & Modifier(NV50_IR_MOD_NOT))
               } else {
      switch (i->op) {
   case OP_AND: code[1] = 0x00000000; break;
   case OP_OR:  code[1] = 0x00004000; break;
   case OP_XOR: code[1] = 0x00008000; break;
   default:
      assert(0);
      }
   if (typeSizeof(i->dType) == 4)
         if (i->src(0).mod & Modifier(NV50_IR_MOD_NOT))
         if (i->src(1).mod & Modifier(NV50_IR_MOD_NOT))
                  }
      void
   CodeEmitterNV50::emitARL(const Instruction *i, unsigned int shl)
   {
      code[0] = 0x00000001 | (shl << 16);
                     setSrcFileBits(i, NV50_OP_ENC_IMM);
   setSrc(i, 0, 0);
      }
      void
   CodeEmitterNV50::emitShift(const Instruction *i)
   {
      if (i->def(0).getFile() == FILE_ADDRESS) {
      assert(i->srcExists(1) && i->src(1).getFile() == FILE_IMMEDIATE);
      } else {
      code[0] = 0x30000001;
   code[1] = (i->op == OP_SHR) ? 0xe0000000 : 0xc0000000;
   if (typeSizeof(i->dType) == 4)
         if (i->op == OP_SHR && isSignedType(i->sType))
            if (i->src(1).getFile() == FILE_IMMEDIATE) {
      code[1] |= 1 << 20;
   code[0] |= (i->getSrc(1)->reg.data.u32 & 0x7f) << 16;
   defId(i->def(0), 2);
   srcId(i->src(0), 9);
      } else {
               }
      void
   CodeEmitterNV50::emitOUT(const Instruction *i)
   {
      code[0] = (i->op == OP_EMIT) ? 0xf0000201 : 0xf0000401;
               }
      void
   CodeEmitterNV50::emitTEX(const TexInstruction *i)
   {
      code[0] = 0xf0000001;
            switch (i->op) {
   case OP_TXB:
      code[1] = 0x20000000;
      case OP_TXL:
      code[1] = 0x40000000;
      case OP_TXF:
      code[0] |= 0x01000000;
      case OP_TXG:
      code[0] |= 0x01000000;
   code[1] = 0x80000000;
      case OP_TXLQ:
      code[1] = 0x60020000;
      default:
      assert(i->op == OP_TEX);
               code[0] |= i->tex.r << 9;
                     if (i->op == OP_TXB || i->op == OP_TXL || i->op == OP_TXF)
         if (i->tex.target.isShadow())
                           if (i->tex.target.isCube()) {
         } else
   if (i->tex.useOffsets) {
      code[1] |= (i->tex.offset[0] & 0xf) << 24;
   code[1] |= (i->tex.offset[1] & 0xf) << 20;
               code[0] |= (i->tex.mask & 0x3) << 25;
            if (i->tex.liveOnly)
         if (i->tex.derivAll)
                        }
      void
   CodeEmitterNV50::emitTXQ(const TexInstruction *i)
   {
               code[0] = 0xf0000001;
            code[0] |= i->tex.r << 9;
            code[0] |= (i->tex.mask & 0x3) << 25;
                        }
      void
   CodeEmitterNV50::emitTEXPREP(const TexInstruction *i)
   {
      code[0] = 0xf8000001 | (3 << 22) | (i->tex.s << 17) | (i->tex.r << 9);
            code[0] |= (i->tex.mask & 0x3) << 25;
   code[1] |= (i->tex.mask & 0xc) << 12;
               }
      void
   CodeEmitterNV50::emitPRERETEmu(const FlowInstruction *i)
   {
               code[0] = 0x10000003; // bra
            switch (i->subOp) {
   case NV50_IR_SUBOP_EMU_PRERET + 0: // bra to the call
         case NV50_IR_SUBOP_EMU_PRERET + 1: // bra to skip the call
      pos += 8;
      default:
      assert(i->subOp == (NV50_IR_SUBOP_EMU_PRERET + 2));
   code[0] = 0x20000003; // call
   code[1] = 0x00000000; // no predicate
      }
   addReloc(RelocEntry::TYPE_CODE, 0, pos, 0x07fff800, 9);
      }
      void
   CodeEmitterNV50::emitFlow(const Instruction *i, uint8_t flowOp)
   {
      const FlowInstruction *f = i->asFlow();
   bool hasPred = false;
            code[0] = 0x00000003 | (flowOp << 28);
            switch (i->op) {
   case OP_BRA:
      hasPred = true;
   hasTarg = true;
      case OP_BREAK:
   case OP_BRKPT:
   case OP_DISCARD:
   case OP_RET:
      hasPred = true;
      case OP_CALL:
   case OP_PREBREAK:
   case OP_JOINAT:
      hasTarg = true;
      case OP_PRERET:
      hasTarg = true;
   if (i->subOp >= NV50_IR_SUBOP_EMU_PRERET) {
      emitPRERETEmu(f);
      }
      default:
                  if (hasPred)
            if (hasTarg && f) {
               if (f->op == OP_CALL) {
      if (f->builtin) {
         } else {
            } else {
                  code[0] |= ((pos >>  2) & 0xffff) << 11;
                              addReloc(relocTy, 0, pos, 0x07fff800, 9);
         }
      void
   CodeEmitterNV50::emitBAR(const Instruction *i)
   {
      ImmediateValue *barId = i->getSrc(0)->asImm();
            code[0] = 0x82000003 | (barId->reg.data.u32 << 21);
            if (i->subOp == NV50_IR_SUBOP_BAR_SYNC)
      }
      void
   CodeEmitterNV50::emitATOM(const Instruction *i)
   {
      uint8_t subOp;
   switch (i->subOp) {
   case NV50_IR_SUBOP_ATOM_ADD:  subOp = 0x0; break;
   case NV50_IR_SUBOP_ATOM_MIN:  subOp = 0x7; break;
   case NV50_IR_SUBOP_ATOM_MAX:  subOp = 0x6; break;
   case NV50_IR_SUBOP_ATOM_INC:  subOp = 0x4; break;
   case NV50_IR_SUBOP_ATOM_DEC:  subOp = 0x5; break;
   case NV50_IR_SUBOP_ATOM_AND:  subOp = 0xa; break;
   case NV50_IR_SUBOP_ATOM_OR:   subOp = 0xb; break;
   case NV50_IR_SUBOP_ATOM_XOR:  subOp = 0xc; break;
   case NV50_IR_SUBOP_ATOM_CAS:  subOp = 0x2; break;
   case NV50_IR_SUBOP_ATOM_EXCH: subOp = 0x1; break;
   default:
      assert(!"invalid subop");
      }
   code[0] = 0xd0000001;
   code[1] = 0xc0c00000 | (subOp << 2);
   if (isSignedType(i->dType))
            // args
   emitFlagsRd(i);
   if (i->subOp == NV50_IR_SUBOP_ATOM_EXCH ||
      i->subOp == NV50_IR_SUBOP_ATOM_CAS ||
   i->defExists(0)) {
   code[1] |= 0x20000000;
   setDst(i, 0);
   setSrc(i, 1, 1);
   // g[] pointer
      } else {
      srcId(i->src(1), 2);
   // g[] pointer
      }
   if (i->subOp == NV50_IR_SUBOP_ATOM_CAS)
               }
      bool
   CodeEmitterNV50::emitInstruction(Instruction *insn)
   {
      if (!insn->encSize) {
      ERROR("skipping unencodable instruction: "); insn->print();
      } else
   if (codeSize + insn->encSize > codeSizeLimit) {
      ERROR("code emitter output buffer too small\n");
               if (insn->bb->getProgram()->dbgFlags & NV50_IR_DEBUG_BASIC) {
                  switch (insn->op) {
   case OP_MOV:
      emitMOV(insn);
      case OP_EXIT:
   case OP_NOP:
   case OP_JOIN:
      emitNOP();
      case OP_VFETCH:
   case OP_LOAD:
      emitLOAD(insn);
      case OP_EXPORT:
   case OP_STORE:
      emitSTORE(insn);
      case OP_PFETCH:
      emitPFETCH(insn);
      case OP_RDSV:
      emitRDSV(insn);
      case OP_LINTERP:
   case OP_PINTERP:
      emitINTERP(insn);
      case OP_ADD:
   case OP_SUB:
      if (insn->dType == TYPE_F64)
         else if (isFloatType(insn->dType))
         else if (insn->getDef(0)->reg.file == FILE_ADDRESS)
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
      case OP_NOT:
      emitNOT(insn);
      case OP_AND:
   case OP_OR:
   case OP_XOR:
      emitLogicOp(insn);
      case OP_SHL:
   case OP_SHR:
      emitShift(insn);
      case OP_SET:
      emitSET(insn);
      case OP_MIN:
   case OP_MAX:
      emitMINMAX(insn);
      case OP_CEIL:
   case OP_FLOOR:
   case OP_TRUNC:
   case OP_ABS:
   case OP_NEG:
   case OP_SAT:
      emitCVT(insn);
      case OP_CVT:
      if (insn->def(0).getFile() == FILE_ADDRESS)
         else
   if (insn->def(0).getFile() == FILE_FLAGS ||
      insn->src(0).getFile() == FILE_FLAGS ||
   insn->src(0).getFile() == FILE_ADDRESS)
      else
            case OP_RCP:
      emitSFnOp(insn, 0);
      case OP_RSQ:
      emitSFnOp(insn, 2);
      case OP_LG2:
      emitSFnOp(insn, 3);
      case OP_SIN:
      emitSFnOp(insn, 4);
      case OP_COS:
      emitSFnOp(insn, 5);
      case OP_EX2:
      emitSFnOp(insn, 6);
      case OP_PRESIN:
   case OP_PREEX2:
      emitPreOp(insn);
      case OP_TEX:
   case OP_TXB:
   case OP_TXL:
   case OP_TXF:
   case OP_TXG:
   case OP_TXLQ:
      emitTEX(insn->asTex());
      case OP_TXQ:
      emitTXQ(insn->asTex());
      case OP_TEXPREP:
      emitTEXPREP(insn->asTex());
      case OP_EMIT:
   case OP_RESTART:
      emitOUT(insn);
      case OP_DISCARD:
      emitFlow(insn, 0x0);
      case OP_BRA:
      emitFlow(insn, 0x1);
      case OP_CALL:
      emitFlow(insn, 0x2);
      case OP_RET:
      emitFlow(insn, 0x3);
      case OP_PREBREAK:
      emitFlow(insn, 0x4);
      case OP_BREAK:
      emitFlow(insn, 0x5);
      case OP_QUADON:
      emitFlow(insn, 0x6);
      case OP_QUADPOP:
      emitFlow(insn, 0x7);
      case OP_JOINAT:
      emitFlow(insn, 0xa);
      case OP_PRERET:
      emitFlow(insn, 0xd);
      case OP_QUADOP:
      emitQUADOP(insn, insn->lanes, insn->subOp);
      case OP_DFDX:
      emitQUADOP(insn, 4, insn->src(0).mod.neg() ? 0x66 : 0x99);
      case OP_DFDY:
      emitQUADOP(insn, 5, insn->src(0).mod.neg() ? 0x5a : 0xa5);
      case OP_ATOM:
      emitATOM(insn);
      case OP_BAR:
      emitBAR(insn);
      case OP_PHI:
   case OP_UNION:
      ERROR("operation should have been eliminated\n");
      case OP_SQRT:
   case OP_SELP:
   case OP_SLCT:
   case OP_TXD:
   case OP_PRECONT:
   case OP_CONT:
   case OP_POPCNT:
   case OP_INSBF:
   case OP_EXTBF:
      ERROR("operation should have been lowered\n");
      default:
      ERROR("unknown op: %u\n", insn->op);
      }
   if (insn->join || insn->op == OP_JOIN)
         else
   if (insn->exit || insn->op == OP_EXIT)
                     code += insn->encSize / 4;
   codeSize += insn->encSize;
      }
      uint32_t
   CodeEmitterNV50::getMinEncodingSize(const Instruction *i) const
   {
               if (info.minEncSize > 4 || i->dType == TYPE_F64)
            // check constraints on dst and src operands
   for (int d = 0; i->defExists(d); ++d) {
      if (i->def(d).rep()->reg.data.id > 63 ||
      i->def(d).rep()->reg.file != FILE_GPR)
            for (int s = 0; i->srcExists(s); ++s) {
      DataFile sf = i->src(s).getFile();
   if (sf != FILE_GPR)
      if (sf != FILE_SHADER_INPUT || progType != Program::TYPE_FRAGMENT)
      if (i->src(s).rep()->reg.data.id > 63)
               // check modifiers & rounding
   if (i->join || i->lanes != 0xf || i->exit)
         if (i->op == OP_MUL && i->rnd != ROUND_N)
            if (i->asTex())
            // check constraints on short MAD
   if (info.srcNr >= 2 && i->srcExists(2)) {
      if (!i->defExists(0) ||
      (i->flagsSrc >= 0 && SDATA(i->src(i->flagsSrc)).id > 0) ||
   DDATA(i->def(0)).id != SDATA(i->src(2)).id)
               }
      // Change the encoding size of an instruction after BBs have been scheduled.
   static void
   makeInstructionLong(Instruction *insn)
   {
      if (insn->encSize == 8)
         Function *fn = insn->bb->getFunction();
   int n = 0;
                     if (n & 1) {
      adj = 8;
      } else
   if (insn->prev && insn->prev->encSize == 4) {
      adj = 8;
      }
            for (int i = fn->bbCount - 1; i >= 0 && fn->bbArray[i] != insn->bb; --i) {
         }
   fn->binSize += adj;
      }
      static bool
   trySetExitModifier(Instruction *insn)
   {
      if (insn->op == OP_DISCARD ||
      insn->op == OP_QUADON ||
   insn->op == OP_QUADPOP)
      for (int s = 0; insn->srcExists(s); ++s)
      if (insn->src(s).getFile() == FILE_IMMEDIATE)
      if (insn->asFlow()) {
      if (insn->op == OP_CALL) // side effects !
         if (insn->getPredicate()) // cannot do conditional exit (or can we ?)
            }
   insn->exit = 1;
   makeInstructionLong(insn);
      }
      static void
   replaceExitWithModifier(Function *func)
   {
               if (!epilogue->getExit() ||
      epilogue->getExit()->op != OP_EXIT) // only main will use OP_EXIT
         if (epilogue->getEntry()->op != OP_EXIT) {
      Instruction *insn = epilogue->getExit()->prev;
   if (!insn || !trySetExitModifier(insn))
            } else {
      for (Graph::EdgeIterator ei = func->cfgExit->incident();
      !ei.end(); ei.next()) {
                  if (!i || !trySetExitModifier(i))
                  int adj = epilogue->getExit()->encSize;
   epilogue->binSize -= adj;
   func->binSize -= adj;
            // There may be BB's that are laid out after the exit block
   for (int i = func->bbCount - 1; i >= 0 && func->bbArray[i] != epilogue; --i) {
            }
      void
   CodeEmitterNV50::prepareEmission(Function *func)
   {
                  }
      CodeEmitterNV50::CodeEmitterNV50(Program::Type type, const TargetNV50 *target) :
         {
      targ = target; // specialized
   code = NULL;
   codeSize = codeSizeLimit = 0;
      }
      CodeEmitter *
   TargetNV50::getCodeEmitter(Program::Type type)
   {
      CodeEmitterNV50 *emit = new CodeEmitterNV50(type, this);
      }
      } // namespace nv50_ir
