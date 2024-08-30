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
      #include "nv50_ir_target_nv50.h"
      namespace nv50_ir {
      Target *getTargetNV50(unsigned int chipset)
   {
         }
      TargetNV50::TargetNV50(unsigned int card) : Target(true, true, false)
   {
               wposMask = 0;
   for (unsigned int i = 0; i <= SV_LAST; ++i)
               }
      #if 0
   // BULTINS / LIBRARY FUNCTIONS:
      // TODO
   static const uint32_t nvc0_builtin_code[] =
   {
   };
      static const uint16_t nvc0_builtin_offsets[NV50_BUILTIN_COUNT] =
   {
   };
   #endif
      void
   TargetNV50::getBuiltinCode(const uint32_t **code, uint32_t *size) const
   {
      *code = NULL;
      }
      uint32_t
   TargetNV50::getBuiltinOffset(int builtin) const
   {
         }
      struct nv50_opProperties
   {
      operation op;
   unsigned int mNeg    : 4;
   unsigned int mAbs    : 4;
   unsigned int mNot    : 4;
   unsigned int mSat    : 4;
   unsigned int fConst  : 3;
   unsigned int fShared : 3;
   unsigned int fAttrib : 3;
      };
      static const struct nv50_opProperties _initProps[] =
   {
      //           neg  abs  not  sat  c[]  s[], a[], imm
   { OP_ADD,    0x3, 0x0, 0x0, 0x8, 0x2, 0x1, 0x1, 0x2 },
   { OP_SUB,    0x3, 0x0, 0x0, 0x8, 0x2, 0x1, 0x1, 0x2 },
   { OP_MUL,    0x3, 0x0, 0x0, 0x0, 0x2, 0x1, 0x1, 0x2 },
   { OP_MAX,    0x3, 0x3, 0x0, 0x0, 0x2, 0x1, 0x1, 0x0 },
   { OP_MIN,    0x3, 0x3, 0x0, 0x0, 0x2, 0x1, 0x1, 0x0 },
   { OP_MAD,    0x7, 0x0, 0x0, 0x8, 0x6, 0x1, 0x1, 0x0 }, // special constraint
   { OP_ABS,    0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x0 },
   { OP_NEG,    0x0, 0x1, 0x0, 0x0, 0x0, 0x1, 0x1, 0x0 },
   { OP_CVT,    0x1, 0x1, 0x0, 0x8, 0x0, 0x1, 0x1, 0x0 },
   { OP_AND,    0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x0, 0x2 },
   { OP_OR,     0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x0, 0x2 },
   { OP_XOR,    0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x0, 0x2 },
   { OP_SHL,    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2 },
   { OP_SHR,    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2 },
   { OP_SET,    0x3, 0x3, 0x0, 0x0, 0x2, 0x1, 0x1, 0x0 },
   { OP_PREEX2, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
   { OP_PRESIN, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
   { OP_EX2,    0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0, 0x0 },
   { OP_LG2,    0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
   { OP_RCP,    0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
   { OP_RSQ,    0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
   { OP_DFDX,   0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
      };
      void TargetNV50::initOpInfo()
   {
               static const operation commutativeList[] =
   {
      OP_ADD, OP_MUL, OP_MAD, OP_FMA, OP_AND, OP_OR, OP_XOR, OP_MAX, OP_MIN,
      };
   static const operation shortFormList[] =
   {
      OP_MOV, OP_ADD, OP_SUB, OP_MUL, OP_MAD, OP_SAD, OP_RCP, OP_LINTERP,
      };
   static const operation noDestList[] =
   {
      OP_STORE, OP_EXPORT, OP_BRA, OP_CALL, OP_RET, OP_EXIT,
   OP_DISCARD, OP_CONT, OP_BREAK, OP_PRECONT, OP_PREBREAK, OP_PRERET,
   OP_JOIN, OP_JOINAT, OP_BRKPT, OP_MEMBAR, OP_EMIT, OP_RESTART,
   OP_QUADON, OP_QUADPOP, OP_TEXBAR, OP_SUSTB, OP_SUSTP, OP_SUREDP,
      };
   static const operation noPredList[] =
   {
      OP_CALL, OP_PREBREAK, OP_PRERET, OP_QUADON, OP_QUADPOP, OP_JOINAT,
               for (i = 0; i < DATA_FILE_COUNT; ++i)
                  for (i = 0; i < OP_LAST; ++i) {
      opInfo[i].variants = NULL;
   opInfo[i].op = (operation)i;
   opInfo[i].srcTypes = 1 << (int)TYPE_F32;
   opInfo[i].dstTypes = 1 << (int)TYPE_F32;
   opInfo[i].immdBits = 0xffffffff;
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
   for (i = 0; i < ARRAY_SIZE(commutativeList); ++i)
         for (i = 0; i < ARRAY_SIZE(shortFormList); ++i)
         for (i = 0; i < ARRAY_SIZE(noDestList); ++i)
         for (i = 0; i < ARRAY_SIZE(noPredList); ++i)
            for (i = 0; i < ARRAY_SIZE(_initProps); ++i) {
               for (int s = 0; s < 3; ++s) {
      if (prop->mNeg & (1 << s))
         if (prop->mAbs & (1 << s))
         if (prop->mNot & (1 << s))
         if (prop->fConst & (1 << s))
         if (prop->fShared & (1 << s))
         if (prop->fAttrib & (1 << s))
         if (prop->fImm & (1 << s))
      }
   if (prop->mSat & 8)
               if (chipset >= 0xa0)
      }
      unsigned int
   TargetNV50::getFileSize(DataFile file) const
   {
      switch (file) {
   case FILE_NULL:          return 0;
   case FILE_GPR:           return 254; // in 16-bit units **
   case FILE_PREDICATE:     return 0;
   case FILE_FLAGS:         return 4;
   case FILE_ADDRESS:       return 4;
   case FILE_BARRIER:       return 0;
   case FILE_IMMEDIATE:     return 0;
   case FILE_MEMORY_CONST:  return 65536;
   case FILE_SHADER_INPUT:  return 0x200;
   case FILE_SHADER_OUTPUT: return 0x200;
   case FILE_MEMORY_BUFFER: return 0xffffffff;
   case FILE_MEMORY_GLOBAL: return 0xffffffff;
   case FILE_MEMORY_SHARED: return 16 << 10;
   case FILE_MEMORY_LOCAL:  return 48 << 10;
   case FILE_SYSTEM_VALUE:  return 16;
   default:
      assert(!"invalid file");
      }
      }
      unsigned int
   TargetNV50::getFileUnit(DataFile file) const
   {
      if (file == FILE_GPR || file == FILE_ADDRESS)
         if (file == FILE_SYSTEM_VALUE)
            }
      uint32_t
   TargetNV50::getSVAddress(DataFile shaderFile, const Symbol *sym) const
   {
      switch (sym->reg.data.sv.sv) {
   case SV_FACE:
         case SV_POSITION:
   {
      uint32_t addr = sysvalLocation[sym->reg.data.sv.sv];
   for (int c = 0; c < sym->reg.data.sv.index; ++c)
      if (wposMask & (1 << c))
         }
   case SV_PRIMITIVE_ID:
      return shaderFile == FILE_SHADER_INPUT ? 0x18 :
      case SV_NCTAID:
         case SV_CTAID:
         case SV_NTID:
         case SV_TID:
   case SV_COMBINED_TID:
         case SV_SAMPLE_POS:
         case SV_THREAD_KILL:
         default:
            }
      // long:  rrr, arr, rcr, acr, rrc, arc, gcr, grr
   // short: rr, ar, rc, gr
   // immd:  ri, gi
   bool
   TargetNV50::insnCanLoad(const Instruction *i, int s,
         {
               // immediate 0 can be represented by GPR $r63/$r127
   // this does not work with global memory ld/st/atom
   if (sf == FILE_IMMEDIATE && ld->getSrc(0)->reg.data.u64 == 0)
      return (!i->isPseudo() &&
         !i->asTex() &&
   i->op != OP_EXPORT &&
   i->op != OP_STORE &&
         if (sf == FILE_IMMEDIATE && (i->predSrc >= 0 || i->flagsDef >= 0))
         if (s >= opInfo[i->op].srcNr)
         if (!(opInfo[i->op].srcFiles[s] & (1 << (int)sf)))
         if (s == 2 && i->src(1).getFile() != FILE_GPR)
            // NOTE: don't rely on flagsDef
   if (sf == FILE_IMMEDIATE)
      for (int d = 0; i->defExists(d); ++d)
                        for (int z = 0; z < Target::operationSrcNr[i->op]; ++z) {
      DataFile zf = (z == s) ? sf : i->src(z).getFile();
   switch (zf) {
   case FILE_GPR:
         case FILE_MEMORY_SHARED:
   case FILE_SHADER_INPUT:
      mode |= 1 << (z * 2);
      case FILE_MEMORY_CONST:
      mode |= 2 << (z * 2);
      case FILE_IMMEDIATE:
         default:
                     switch (mode) {
   case 0x00:
   case 0x01:
   case 0x03:
   case 0x08:
   case 0x0c:
   case 0x20:
   case 0x21:
         case 0x09:
      // Shader inputs get transformed to p[] in geometry shaders, and those
   // aren't allowed to be used at the same time as c[].
   if (ld->bb->getProgram()->getType() == Program::TYPE_GEOMETRY)
            case 0x0d:
      if (ld->bb->getProgram()->getType() != Program::TYPE_GEOMETRY)
            default:
                           if ((i->op == OP_MUL || i->op == OP_MAD) && !isFloatType(i->dType)) {
      // 32-bit MUL will be split into 16-bit MULs
   if (ld->src(0).isIndirect(0))
         if (sf == FILE_IMMEDIATE)
         if (i->subOp == NV50_IR_SUBOP_MUL_HIGH && sf == FILE_MEMORY_CONST)
            } else {
                  if (sf == FILE_IMMEDIATE) {
      if (ldSize == 2 && (i->op == OP_AND || i->op == OP_OR || i->op == OP_XOR))
                                 if (ldSize < 4 && sf == FILE_SHADER_INPUT) // no < 4-byte aligned a[] access
         if (ld->getSrc(0)->reg.data.offset > (int32_t)(127 * ldSize))
            if (ld->src(0).isIndirect(0)) {
      for (int z = 0; i->srcExists(z); ++z)
                  // s[] access only possible in CP, $aX always applies
   if (sf == FILE_MEMORY_SHARED)
         if (!ld->bb) // can't check type ...
                  // $aX applies to c[] only in VP, FP, GP if p[] is not accessed
   if (pt == Program::TYPE_COMPUTE)
         if (pt == Program::TYPE_GEOMETRY) {
      if (sf == FILE_MEMORY_CONST)
            }
      }
      }
      bool
   TargetNV50::insnCanLoadOffset(const Instruction *i, int s, int offset) const
   {
      if (!i->src(s).isIndirect(0))
         offset += i->src(s).get()->reg.data.offset;
   if (i->op == OP_LOAD || i->op == OP_STORE || i->op == OP_ATOM) {
      // There are some restrictions in theory, but in practice they're never
   // going to be hit. However offsets on global/shared memory are just
   // plain not supported.
   return i->src(s).getFile() != FILE_MEMORY_GLOBAL &&
      }
      }
      bool
   TargetNV50::isAccessSupported(DataFile file, DataType ty) const
   {
      if (ty == TYPE_B96 || ty == TYPE_NONE)
         if (typeSizeof(ty) > 4)
      return (file == FILE_MEMORY_LOCAL) || (file == FILE_MEMORY_GLOBAL) ||
         }
      bool
   TargetNV50::isOpSupported(operation op, DataType ty) const
   {
      if (ty == TYPE_F64 && chipset < 0xa0)
            switch (op) {
   case OP_PRERET:
         case OP_TXG:
         case OP_SQRT:
   case OP_DIV:
   case OP_MOD:
   case OP_SET_AND:
   case OP_SET_OR:
   case OP_SET_XOR:
   case OP_SLCT:
   case OP_SELP:
   case OP_POPCNT:
   case OP_INSBF:
   case OP_EXTBF:
   case OP_EXIT: // want exit modifier instead (on NOP if required)
   case OP_MEMBAR:
   case OP_SHLADD:
   case OP_XMAD:
         case OP_SAD:
         case OP_SET:
         default:
            }
      bool
   TargetNV50::isModSupported(const Instruction *insn, int s, Modifier mod) const
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
         case OP_ADD:
      if (insn->src(s ? 0 : 1).mod.neg())
            case OP_SUB:
      if (s == 0)
            case OP_SET:
      if (insn->sType != TYPE_F32)
            default:
            }
   if (s >= opInfo[insn->op].srcNr || s >= 3)
            }
      bool
   TargetNV50::mayPredicate(const Instruction *insn, const Value *pred) const
   {
      if (insn->getPredicate() || insn->flagsSrc >= 0)
         for (int s = 0; insn->srcExists(s); ++s)
      if (insn->src(s).getFile() == FILE_IMMEDIATE)
         }
      bool
   TargetNV50::isSatSupported(const Instruction *insn) const
   {
      if (insn->op == OP_CVT)
         if (insn->dType != TYPE_F32)
            }
      int TargetNV50::getLatency(const Instruction *i) const
   {
      // TODO: tune these values
   if (i->op == OP_LOAD) {
      switch (i->src(0).getFile()) {
   case FILE_MEMORY_LOCAL:
   case FILE_MEMORY_GLOBAL:
   case FILE_MEMORY_BUFFER:
         default:
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
   int TargetNV50::getThroughput(const Instruction *i) const
   {
      // TODO: tune these values
   if (i->dType == TYPE_F32) {
      switch (i->op) {
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
         } else
   if (i->dType == TYPE_F64) {
         } else {
            }
      static void
   recordLocation(uint16_t *locs, uint8_t *masks,
         {
               switch (var->sn) {
   case TGSI_SEMANTIC_POSITION: locs[SV_POSITION] = addr; break;
   case TGSI_SEMANTIC_INSTANCEID: locs[SV_INSTANCE_ID] = addr; break;
   case TGSI_SEMANTIC_VERTEXID: locs[SV_VERTEX_ID] = addr; break;
   case TGSI_SEMANTIC_PRIMID: locs[SV_PRIMITIVE_ID] = addr; break;
   case TGSI_SEMANTIC_LAYER: locs[SV_LAYER] = addr; break;
   case TGSI_SEMANTIC_VIEWPORT_INDEX: locs[SV_VIEWPORT_INDEX] = addr; break;
   default:
         }
   if (var->sn == TGSI_SEMANTIC_POSITION && masks)
      }
      static void
   recordLocationSysVal(uint16_t *locs, uint8_t *masks,
         {
               switch (var->sn) {
   case SYSTEM_VALUE_FRAG_COORD: locs[SV_POSITION] = addr; break;
   case SYSTEM_VALUE_INSTANCE_ID: locs[SV_INSTANCE_ID] = addr; break;
   case SYSTEM_VALUE_VERTEX_ID: locs[SV_VERTEX_ID] = addr; break;
   case SYSTEM_VALUE_PRIMITIVE_ID: locs[SV_PRIMITIVE_ID] = addr; break;
   default:
         }
   // TODO is this even hit?
   if (var->sn == SYSTEM_VALUE_FRAG_COORD && masks)
      }
      void
   TargetNV50::parseDriverInfo(const struct nv50_ir_prog_info *info,
         {
      unsigned int i;
   for (i = 0; i < info_out->numOutputs; ++i)
         for (i = 0; i < info_out->numInputs; ++i)
         for (i = 0; i < info_out->numSysVals; ++i)
            if (sysvalLocation[SV_POSITION] >= 0x200) {
      // not assigned by driver, but we need it internally
   wposMask = 0x8;
                  }
      } // namespace nv50_ir
