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
      Target *getTargetNVC0(unsigned int chipset)
   {
         }
      TargetNVC0::TargetNVC0(unsigned int card) :
         {
      chipset = card;
      }
      // BULTINS / LIBRARY FUNCTIONS:
      // lazyness -> will just hardcode everything for the time being
      #include "lib/gf100.asm.h"
   #include "lib/gk104.asm.h"
   #include "lib/gk110.asm.h"
      void
   TargetNVC0::getBuiltinCode(const uint32_t **code, uint32_t *size) const
   {
      switch (chipset & ~0xf) {
   case 0xe0:
      if (chipset < NVISA_GK20A_CHIPSET) {
      *code = (const uint32_t *)&gk104_builtin_code[0];
   *size = sizeof(gk104_builtin_code);
      }
      case 0xf0:
   case 0x100:
      *code = (const uint32_t *)&gk110_builtin_code[0];
   *size = sizeof(gk110_builtin_code);
      default:
      *code = (const uint32_t *)&gf100_builtin_code[0];
   *size = sizeof(gf100_builtin_code);
         }
      uint32_t
   TargetNVC0::getBuiltinOffset(int builtin) const
   {
               switch (chipset & ~0xf) {
   case 0xe0:
      if (chipset < NVISA_GK20A_CHIPSET)
            case 0xf0:
   case 0x100:
         default:
            }
      struct nvc0_opProperties
   {
      operation op;
   unsigned int mNeg   : 4;
   unsigned int mAbs   : 4;
   unsigned int mNot   : 4;
   unsigned int mSat   : 4;
   unsigned int fConst : 3;
      };
      static const struct nvc0_opProperties _initProps[] =
   {
      //           neg  abs  not  sat  c[]  imm
   { OP_ADD,    0x3, 0x3, 0x0, 0x8, 0x2, 0x2 | 0x8 },
   { OP_SUB,    0x3, 0x3, 0x0, 0x0, 0x2, 0x2 | 0x8 },
   { OP_MUL,    0x3, 0x0, 0x0, 0x8, 0x2, 0x2 | 0x8 },
   { OP_MAX,    0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   { OP_MIN,    0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   { OP_MAD,    0x7, 0x0, 0x0, 0x8, 0x6, 0x2 }, // special c[] constraint
   { OP_FMA,    0x7, 0x0, 0x0, 0x8, 0x6, 0x2 }, // keep the same as OP_MAD
   { OP_SHLADD, 0x5, 0x0, 0x0, 0x0, 0x4, 0x6 },
   { OP_MADSP,  0x0, 0x0, 0x0, 0x0, 0x6, 0x2 },
   { OP_ABS,    0x0, 0x0, 0x0, 0x0, 0x1, 0x0 },
   { OP_NEG,    0x0, 0x1, 0x0, 0x0, 0x1, 0x0 },
   { OP_CVT,    0x1, 0x1, 0x0, 0x8, 0x1, 0x0 },
   { OP_CEIL,   0x1, 0x1, 0x0, 0x8, 0x1, 0x0 },
   { OP_FLOOR,  0x1, 0x1, 0x0, 0x8, 0x1, 0x0 },
   { OP_TRUNC,  0x1, 0x1, 0x0, 0x8, 0x1, 0x0 },
   { OP_AND,    0x0, 0x0, 0x3, 0x0, 0x2, 0x2 | 0x8 },
   { OP_OR,     0x0, 0x0, 0x3, 0x0, 0x2, 0x2 | 0x8 },
   { OP_XOR,    0x0, 0x0, 0x3, 0x0, 0x2, 0x2 | 0x8 },
   { OP_SHL,    0x0, 0x0, 0x0, 0x0, 0x2, 0x2 },
   { OP_SHR,    0x0, 0x0, 0x0, 0x0, 0x2, 0x2 },
   { OP_SET,    0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   { OP_SLCT,   0x4, 0x0, 0x0, 0x0, 0x6, 0x2 }, // special c[] constraint
   { OP_PREEX2, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1 },
   { OP_PRESIN, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1 },
   { OP_COS,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_SIN,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_EX2,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_LG2,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_RCP,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_RSQ,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_SQRT,   0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_DFDX,   0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
   { OP_DFDY,   0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
   { OP_CALL,   0x0, 0x0, 0x0, 0x0, 0x1, 0x0 },
   { OP_POPCNT, 0x0, 0x0, 0x3, 0x0, 0x2, 0x2 },
   { OP_INSBF,  0x0, 0x0, 0x0, 0x0, 0x6, 0x2 },
   { OP_EXTBF,  0x0, 0x0, 0x0, 0x0, 0x2, 0x2 },
   { OP_BFIND,  0x0, 0x0, 0x1, 0x0, 0x1, 0x1 },
   { OP_PERMT,  0x0, 0x0, 0x0, 0x0, 0x6, 0x2 },
   { OP_SET_AND, 0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   { OP_SET_OR, 0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   { OP_SET_XOR, 0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   // saturate only:
   { OP_LINTERP, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0 },
      };
      static const struct nvc0_opProperties _initPropsNVE4[] = {
      { OP_SULDB,   0x0, 0x0, 0x0, 0x0, 0x2, 0x0 },
   { OP_SUSTB,   0x0, 0x0, 0x0, 0x0, 0x2, 0x0 },
   { OP_SUSTP,   0x0, 0x0, 0x0, 0x0, 0x2, 0x0 },
   { OP_SUCLAMP, 0x0, 0x0, 0x0, 0x0, 0x2, 0x2 },
   { OP_SUBFM,   0x0, 0x0, 0x0, 0x0, 0x6, 0x2 },
      };
      static const struct nvc0_opProperties _initPropsGM107[] = {
      { OP_SULDB,   0x0, 0x0, 0x0, 0x0, 0x0, 0x2 },
   { OP_SULDP,   0x0, 0x0, 0x0, 0x0, 0x0, 0x2 },
   { OP_SUSTB,   0x0, 0x0, 0x0, 0x0, 0x0, 0x4 },
   { OP_SUSTP,   0x0, 0x0, 0x0, 0x0, 0x0, 0x4 },
   { OP_SUREDB,  0x0, 0x0, 0x0, 0x0, 0x0, 0x4 },
   { OP_SUREDP,  0x0, 0x0, 0x0, 0x0, 0x0, 0x4 },
      };
      void TargetNVC0::initProps(const struct nvc0_opProperties *props, int size)
   {
      for (int i = 0; i < size; ++i) {
               for (int s = 0; s < 3; ++s) {
      if (prop->mNeg & (1 << s))
         if (prop->mAbs & (1 << s))
         if (prop->mNot & (1 << s))
         if (prop->fConst & (1 << s))
         if (prop->fImmd & (1 << s))
         if (prop->fImmd & 8)
      }
   if (prop->mSat & 8)
         }
      void TargetNVC0::initOpInfo()
   {
               static const operation commutative[] =
   {
      OP_ADD, OP_MUL, OP_MAD, OP_FMA, OP_AND, OP_OR, OP_XOR, OP_MAX, OP_MIN,
               static const operation shortForm[] =
   {
                  static const operation noDest[] =
   {
      OP_STORE, OP_EXPORT, OP_BRA, OP_CALL, OP_RET, OP_EXIT,
   OP_DISCARD, OP_CONT, OP_BREAK, OP_PRECONT, OP_PREBREAK, OP_PRERET,
   OP_JOIN, OP_JOINAT, OP_BRKPT, OP_MEMBAR, OP_EMIT, OP_RESTART,
   OP_QUADON, OP_QUADPOP, OP_TEXBAR, OP_SUSTB, OP_SUSTP, OP_SUREDP,
               static const operation noPred[] =
   {
      OP_CALL, OP_PRERET, OP_QUADON, OP_QUADPOP,
               for (i = 0; i < DATA_FILE_COUNT; ++i)
                  for (i = 0; i < OP_LAST; ++i) {
      opInfo[i].variants = NULL;
   opInfo[i].op = (operation)i;
   opInfo[i].srcTypes = 1 << (int)TYPE_F32;
   opInfo[i].dstTypes = 1 << (int)TYPE_F32;
   opInfo[i].immdBits = 0;
            for (j = 0; j < opInfo[i].srcNr; ++j) {
      opInfo[i].srcMods[j] = 0;
      }
   opInfo[i].dstMods = 0;
            opInfo[i].hasDest = 1;
   opInfo[i].vector = (i >= OP_TEX && i <= OP_TEXCSAA);
   opInfo[i].commutative = false; /* set below */
   opInfo[i].pseudo = (i < OP_MOV);
   opInfo[i].predicate = !opInfo[i].pseudo;
   opInfo[i].flow = (i >= OP_BRA && i <= OP_JOIN);
      }
   for (i = 0; i < ARRAY_SIZE(commutative); ++i)
         for (i = 0; i < ARRAY_SIZE(shortForm); ++i)
         for (i = 0; i < ARRAY_SIZE(noDest); ++i)
         for (i = 0; i < ARRAY_SIZE(noPred); ++i)
            initProps(_initProps, ARRAY_SIZE(_initProps));
   if (chipset >= NVISA_GM107_CHIPSET)
         else if (chipset >= NVISA_GK104_CHIPSET)
      }
      unsigned int
   TargetNVC0::getFileSize(DataFile file) const
   {
      const unsigned int smregs = (chipset >= NVISA_GK104_CHIPSET) ? 65536 : 32768;
   const unsigned int bs = (chipset >= NVISA_GV100_CHIPSET) ? 16 : 0;
            /* probably because of ugprs? */
   if (chipset >= NVISA_GV100_CHIPSET)
         else if (chipset >= NVISA_GK20A_CHIPSET)
         else
            switch (file) {
   case FILE_NULL:          return 0;
   case FILE_GPR:           return MIN2(gprs, smregs / threads);
   case FILE_PREDICATE:     return 7;
   case FILE_FLAGS:         return 1;
   case FILE_ADDRESS:       return 0;
   case FILE_BARRIER:       return bs;
   case FILE_IMMEDIATE:     return 0;
   case FILE_MEMORY_CONST:  return 65536;
   case FILE_SHADER_INPUT:  return 0x400;
   case FILE_SHADER_OUTPUT: return 0x400;
   case FILE_MEMORY_BUFFER: return 0xffffffff;
   case FILE_MEMORY_GLOBAL: return 0xffffffff;
   case FILE_MEMORY_SHARED: return 16 << 10;
   case FILE_MEMORY_LOCAL:  return 48 << 10;
   case FILE_SYSTEM_VALUE:  return 32;
   case FILE_THREAD_STATE:  return bs;
   default:
      assert(!"invalid file");
         }
      unsigned int
   TargetNVC0::getFileUnit(DataFile file) const
   {
      if (file == FILE_GPR || file == FILE_ADDRESS || file == FILE_SYSTEM_VALUE ||
      file == FILE_BARRIER || file == FILE_THREAD_STATE)
         }
      uint32_t
   TargetNVC0::getSVAddress(DataFile shaderFile, const Symbol *sym) const
   {
      const int idx = sym->reg.data.sv.index;
            const bool isInput = shaderFile == FILE_SHADER_INPUT;
            switch (sv) {
   case SV_POSITION:       return 0x070 + idx * 4;
   case SV_INSTANCE_ID:    return 0x2f8;
   case SV_VERTEX_ID:      return 0x2fc;
   case SV_PRIMITIVE_ID:   return isInput ? 0x060 : 0x040;
   case SV_LAYER:          return 0x064;
   case SV_VIEWPORT_INDEX: return 0x068;
   case SV_POINT_SIZE:     return 0x06c;
   case SV_CLIP_DISTANCE:  return 0x2c0 + idx * 4;
   case SV_POINT_COORD:    return 0x2e0 + idx * 4;
   case SV_FACE:           return 0x3fc;
   case SV_TESS_OUTER:     return 0x000 + idx * 4;
   case SV_TESS_INNER:     return 0x010 + idx * 4;
   case SV_TESS_COORD:     return 0x2f0 + idx * 4;
   case SV_NTID:           return kepler ? (0x00 + idx * 4) : ~0;
   case SV_NCTAID:         return kepler ? (0x0c + idx * 4) : ~0;
   case SV_GRIDID:         return kepler ? 0x18 : ~0;
   case SV_WORK_DIM:       return 0x1c;
   case SV_SAMPLE_INDEX:   return 0;
   case SV_SAMPLE_POS:     return 0;
   case SV_SAMPLE_MASK:    return 0;
   case SV_BASEVERTEX:     return 0;
   case SV_BASEINSTANCE:   return 0;
   case SV_DRAWID:         return 0;
   default:
            }
      bool
   TargetNVC0::insnCanLoad(const Instruction *i, int s,
         {
               // immediate 0 can be represented by GPR $r63/$r255
   if (sf == FILE_IMMEDIATE && ld->getSrc(0)->reg.data.u64 == 0)
      return (!i->isPseudo() &&
               if (s >= opInfo[i->op].srcNr)
         if (!(opInfo[i->op].srcFiles[s] & (1 << (int)sf)))
            // indirect loads can only be done by OP_LOAD/VFETCH/INTERP on nvc0
   if (ld->src(0).isIndirect(0))
         // these are implemented using shf.r and shf.l which can't load consts
   if ((i->op == OP_SHL || i->op == OP_SHR) && typeSizeof(i->sType) == 8 &&
      sf == FILE_MEMORY_CONST)
      // constant buffer loads can't be used with cbcc xmads
   if (i->op == OP_XMAD && sf == FILE_MEMORY_CONST &&
      (i->subOp & NV50_IR_SUBOP_XMAD_CMODE_MASK) == NV50_IR_SUBOP_XMAD_CBCC)
      // constant buffer loads for the third operand can't be used with psl/mrg xmads
   if (i->op == OP_XMAD && sf == FILE_MEMORY_CONST && s == 2 &&
      (i->subOp & (NV50_IR_SUBOP_XMAD_PSL | NV50_IR_SUBOP_XMAD_MRG)))
      // for xmads, immediates can't have the h1 flag set
   if (i->op == OP_XMAD && sf == FILE_IMMEDIATE && s < 2 &&
      i->subOp & NV50_IR_SUBOP_XMAD_H1(s))
         for (int k = 0; i->srcExists(k); ++k) {
      if (i->src(k).getFile() == FILE_IMMEDIATE) {
      if (k == 2 && i->op == OP_SUCLAMP) // special case
         if (k == 1 && i->op == OP_SHLADD) // special case
         if (i->getSrc(k)->reg.data.u64 != 0)
      } else
   if (i->src(k).getFile() != FILE_GPR &&
      i->src(k).getFile() != FILE_PREDICATE &&
   i->src(k).getFile() != FILE_FLAGS) {
                  // only loads can do sub 4 byte addressing
   if (sf == FILE_MEMORY_CONST &&
      (ld->getSrc(0)->reg.data.offset & 0x3)
   && i->op != OP_LOAD)
         // not all instructions support full 32 bit immediates
   if (sf == FILE_IMMEDIATE) {
               if (opInfo[i->op].immdBits != 0xffffffff || typeSizeof(i->sType) > 4) {
      switch (i->sType) {
   case TYPE_F64:
      if (reg.data.u64 & 0x00000fffffffffffULL)
            case TYPE_F32:
      if (reg.data.u32 & 0xfff)
            case TYPE_S32:
   case TYPE_U32:
      // with u32, 0xfffff counts as 0xffffffff as well
   if (reg.data.s32 > 0x7ffff || reg.data.s32 < -0x80000)
         // XMADs can only have 16-bit immediates
   if (i->op == OP_XMAD && reg.data.u32 > 0xffff)
            case TYPE_U8:
   case TYPE_S8:
   case TYPE_U16:
   case TYPE_S16:
   case TYPE_F16:
         default:
            } else
   if (i->op == OP_ADD && i->sType == TYPE_F32) {
      // add f32 LIMM cannot saturate
   if (i->saturate && (reg.data.u32 & 0xfff))
                     }
      bool
   TargetNVC0::insnCanLoadOffset(const Instruction *insn, int s, int offset) const
   {
      const ValueRef& ref = insn->src(s);
   offset += insn->src(s).get()->reg.data.offset;
   if (ref.getFile() == FILE_MEMORY_CONST &&
      (insn->op != OP_LOAD || insn->subOp != NV50_IR_SUBOP_LDC_IS))
         }
      bool
   TargetNVC0::isAccessSupported(DataFile file, DataType ty) const
   {
      if (ty == TYPE_NONE)
         if (file == FILE_MEMORY_CONST) {
      if (getChipset() >= NVISA_GM107_CHIPSET)
         else
   if (getChipset() >= NVISA_GK104_CHIPSET) // wrong encoding ?
      }
   if (ty == TYPE_B96)
            }
      bool
   TargetNVC0::isOpSupported(operation op, DataType ty) const
   {
      if (op == OP_SAD && ty != TYPE_S32 && ty != TYPE_U32)
         if (op == OP_SQRT || op == OP_DIV || op == OP_MOD)
         if (op == OP_XMAD)
            }
      bool
   TargetNVC0::isModSupported(const Instruction *insn, int s, Modifier mod) const
   {
      if (!isFloatType(insn->dType)) {
      switch (insn->op) {
   case OP_ABS:
   case OP_NEG:
   case OP_CVT:
   case OP_CEIL:
   case OP_FLOOR:
   case OP_TRUNC:
   case OP_AND:
   case OP_OR:
   case OP_XOR:
   case OP_POPCNT:
   case OP_BFIND:
   case OP_XMAD:
         case OP_SET:
      if (insn->sType != TYPE_F32)
            case OP_ADD:
      if (mod.abs())
         if (insn->src(s ? 0 : 1).mod.neg())
            case OP_SUB:
      if (s == 0)
            case OP_SHLADD:
      if (s == 1)
         if (insn->src(s ? 0 : 2).mod.neg())
            default:
            }
   if (s >= opInfo[insn->op].srcNr || s >= 3)
            }
      bool
   TargetNVC0::mayPredicate(const Instruction *insn, const Value *pred) const
   {
      if (insn->getPredicate())
            }
      bool
   TargetNVC0::isSatSupported(const Instruction *insn) const
   {
      if (insn->op == OP_CVT)
         if (!(opInfo[insn->op].dstMods & NV50_IR_MOD_SAT))
            if (insn->dType == TYPE_U32)
            // add f32 LIMM cannot saturate
   if (insn->op == OP_ADD && insn->sType == TYPE_F32) {
      if (insn->getSrc(1)->asImm() &&
      insn->getSrc(1)->reg.data.u32 & 0xfff)
               }
      bool
   TargetNVC0::isPostMultiplySupported(operation op, float f, int& e) const
   {
      if (op != OP_MUL)
         f = fabsf(f);
   e = static_cast<int>(log2f(f));
   if (e < -3 || e > 3)
            }
      // TODO: better values
   // this could be more precise, e.g. depending on the issue-to-read/write delay
   // of the depending instruction, but it's good enough
   int TargetNVC0::getLatency(const Instruction *i) const
   {
      if (chipset >= 0xe4) {
      if (i->dType == TYPE_F64 || i->sType == TYPE_F64)
         switch (i->op) {
   case OP_LINTERP:
   case OP_PINTERP:
         case OP_LOAD:
      if (i->src(0).getFile() == FILE_MEMORY_CONST)
            case OP_VFETCH:
         default:
      if (Target::getOpClass(i->op) == OPCLASS_TEXTURE)
         if (i->op == OP_MUL && i->dType != TYPE_F32)
               } else {
      if (i->op == OP_LOAD) {
      if (i->cache == CACHE_CV)
            }
      }
      }
      // These are "inverse" throughput values, i.e. the number of cycles required
   // to issue a specific instruction for a full warp (32 threads).
   //
   // Assuming we have more than 1 warp in flight, a higher issue latency results
   // in a lower result latency since the MP will have spent more time with other
   // warps.
   // This also helps to determine the number of cycles between instructions in
   // a single warp.
   //
   int TargetNVC0::getThroughput(const Instruction *i) const
   {
      // TODO: better values
   if (i->dType == TYPE_F32) {
      switch (i->op) {
   case OP_ADD:
   case OP_MUL:
   case OP_MAD:
   case OP_FMA:
         case OP_CVT:
   case OP_CEIL:
   case OP_FLOOR:
   case OP_TRUNC:
   case OP_SET:
   case OP_SLCT:
   case OP_MIN:
   case OP_MAX:
         case OP_RCP:
   case OP_RSQ:
   case OP_LG2:
   case OP_SIN:
   case OP_COS:
   case OP_PRESIN:
   case OP_PREEX2:
   default:
            } else
   if (i->dType == TYPE_U32 || i->dType == TYPE_S32) {
      switch (i->op) {
   case OP_ADD:
   case OP_AND:
   case OP_OR:
   case OP_XOR:
   case OP_NOT:
         case OP_MUL:
   case OP_MAD:
   case OP_CVT:
   case OP_SET:
   case OP_SLCT:
   case OP_SHL:
   case OP_SHR:
   case OP_NEG:
   case OP_ABS:
   case OP_MIN:
   case OP_MAX:
   default:
            } else
   if (i->dType == TYPE_F64) {
         } else {
            }
      bool TargetNVC0::canDualIssue(const Instruction *a, const Instruction *b) const
   {
      const OpClass clA = operationClass[a->op];
            if (getChipset() >= 0xe4) {
      // not texturing
   // not if the 2nd instruction isn't necessarily executed
   if (clA == OPCLASS_TEXTURE || clA == OPCLASS_FLOW)
            // Check that a and b don't write to the same sources, nor that b reads
   // anything that a writes.
   if (!a->canCommuteDefDef(b) || !a->canCommuteDefSrc(b))
            // anything with MOV
   if (a->op == OP_MOV || b->op == OP_MOV)
         if (clA == clB) {
      switch (clA) {
   // there might be more
   case OPCLASS_COMPARE:
      if ((a->op == OP_MIN || a->op == OP_MAX) &&
      (b->op == OP_MIN || b->op == OP_MAX))
         case OPCLASS_ARITH:
         default:
         }
   // only F32 arith or integer additions
   return (a->dType == TYPE_F32 || a->op == OP_ADD ||
      }
   // nothing with TEXBAR
   if (a->op == OP_TEXBAR || b->op == OP_TEXBAR)
         // no loads and stores accessing the same space
   if ((clA == OPCLASS_LOAD && clB == OPCLASS_STORE) ||
      (clB == OPCLASS_LOAD && clA == OPCLASS_STORE))
   if (a->src(0).getFile() == b->src(0).getFile())
      // no > 32-bit ops
   if (typeSizeof(a->dType) > 4 || typeSizeof(b->dType) > 4 ||
      typeSizeof(a->sType) > 4 || typeSizeof(b->sType) > 4)
         } else {
            }
      } // namespace nv50_ir
