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
      namespace nv50_ir {
      const uint8_t Target::operationSrcNr[] =
   {
      0, 0,                   // NOP, PHI
   0, 0, 0,                // UNION, SPLIT, MERGE
   1, 1, 2,                // MOV, LOAD, STORE
   2, 2, 2, 2, 2, 3, 3, 3, // ADD, SUB, MUL, DIV, MOD, MAD, FMA, SAD
   3, 3,                   // SHLADD, XMAD
   1, 1, 1,                // ABS, NEG, NOT
   2, 2, 2, 3, 2, 2, 3,    // AND, OR, XOR, LOP3_LUT, SHL, SHR, SHF
   2, 2, 1,                // MAX, MIN, SAT
   1, 1, 1, 1,             // CEIL, FLOOR, TRUNC, CVT
   3, 3, 3, 2, 3, 3,       // SET_AND,OR,XOR, SET, SELP, SLCT
   1, 1, 1, 1, 1, 1,       // RCP, RSQ, LG2, SIN, COS, EX2
   1, 1, 1,                // PRESIN, PREEX2, SQRT
   0, 0, 0, 0, 0,          // BRA, CALL, RET, CONT, BREAK,
   0, 0, 0,                // PRERET,CONT,BREAK
   0, 0, 0, 0, 0, 0,       // BRKPT, JOINAT, JOIN, DISCARD, EXIT, MEMBAR
   1, 1, 1, 2, 1, 2,       // VFETCH, PFETCH, AFETCH, EXPORT, LINTERP, PINTERP
   1, 1, 1,                // EMIT, RESTART, FINAL
   1, 1, 1,                // TEX, TXB, TXL,
   1, 1, 1, 1, 1, 1, 2,    // TXF, TXQ, TXD, TXG, TXLQ, TEXCSAA, TEXPREP
   1, 1, 2, 2, 2, 2, 2,    // SULDB, SULDP, SUSTB, SUSTP, SUREDB, SUREDP, SULEA
   3, 3, 3, 1, 3,          // SUBFM, SUCLAMP, SUEAU, SUQ, MADSP
   0,                      // TEXBAR
   1, 1,                   // DFDX, DFDY
   1, 1, 2, 0, 0,          // RDSV, PIXLD, QUADOP, QUADON, QUADPOP
   2, 3, 2, 1, 1, 2, 3,    // POPCNT, INSBF, EXTBF, BFIND, BREV, BMSK, PERMT
   2,                      // SGXT
   3, 2,                   // ATOM, BAR
   2, 2, 2, 2, 3, 2,       // VADD, VAVG, VMIN, VMAX, VSAD, VSET,
   2, 2, 2, 1,             // VSHR, VSHL, VSEL, CCTL
   3,                      // SHFL
   1,                      // VOTE
   1,                      // BUFQ
   1,                      // WARPSYNC
      };
      const OpClass Target::operationClass[] =
   {
      // NOP; PHI; UNION, SPLIT, MERGE
   OPCLASS_OTHER,
   OPCLASS_PSEUDO,
   OPCLASS_PSEUDO, OPCLASS_PSEUDO, OPCLASS_PSEUDO,
   // MOV; LOAD; STORE
   OPCLASS_MOVE,
   OPCLASS_LOAD,
   OPCLASS_STORE,
   // ADD, SUB, MUL; DIV, MOD; MAD, FMA, SAD, SHLADD, XMAD
   OPCLASS_ARITH, OPCLASS_ARITH, OPCLASS_ARITH,
   OPCLASS_ARITH, OPCLASS_ARITH,
   OPCLASS_ARITH, OPCLASS_ARITH, OPCLASS_ARITH, OPCLASS_ARITH, OPCLASS_ARITH,
   // ABS, NEG; NOT, AND, OR, XOR, LOP3_LUT; SHL, SHR, SHF
   OPCLASS_CONVERT, OPCLASS_CONVERT,
   OPCLASS_LOGIC, OPCLASS_LOGIC, OPCLASS_LOGIC, OPCLASS_LOGIC, OPCLASS_LOGIC,
   OPCLASS_SHIFT, OPCLASS_SHIFT, OPCLASS_SHIFT,
   // MAX, MIN
   OPCLASS_COMPARE, OPCLASS_COMPARE,
   // SAT, CEIL, FLOOR, TRUNC; CVT
   OPCLASS_CONVERT, OPCLASS_CONVERT, OPCLASS_CONVERT, OPCLASS_CONVERT,
   OPCLASS_CONVERT,
   // SET(AND,OR,XOR); SELP, SLCT
   OPCLASS_COMPARE, OPCLASS_COMPARE, OPCLASS_COMPARE, OPCLASS_COMPARE,
   OPCLASS_COMPARE, OPCLASS_COMPARE,
   // RCP, RSQ, LG2, SIN, COS; EX2, PRESIN, PREEX2, SQRT
   OPCLASS_SFU, OPCLASS_SFU, OPCLASS_SFU, OPCLASS_SFU, OPCLASS_SFU,
   OPCLASS_SFU, OPCLASS_SFU, OPCLASS_SFU, OPCLASS_SFU,
   // BRA, CALL, RET; CONT, BREAK, PRE(RET,CONT,BREAK); BRKPT, JOINAT, JOIN
   OPCLASS_FLOW, OPCLASS_FLOW, OPCLASS_FLOW,
   OPCLASS_FLOW, OPCLASS_FLOW, OPCLASS_FLOW, OPCLASS_FLOW, OPCLASS_FLOW,
   OPCLASS_FLOW, OPCLASS_FLOW, OPCLASS_FLOW,
   // DISCARD, EXIT
   OPCLASS_FLOW, OPCLASS_FLOW,
   // MEMBAR
   OPCLASS_CONTROL,
   // VFETCH, PFETCH, AFETCH, EXPORT
   OPCLASS_LOAD, OPCLASS_OTHER, OPCLASS_OTHER, OPCLASS_STORE,
   // LINTERP, PINTERP
   OPCLASS_SFU, OPCLASS_SFU,
   // EMIT, RESTART, FINAL
   OPCLASS_CONTROL, OPCLASS_CONTROL, OPCLASS_CONTROL,
   // TEX, TXB, TXL, TXF; TXQ, TXD, TXG, TXLQ; TEXCSAA, TEXPREP
   OPCLASS_TEXTURE, OPCLASS_TEXTURE, OPCLASS_TEXTURE, OPCLASS_TEXTURE,
   OPCLASS_TEXTURE, OPCLASS_TEXTURE, OPCLASS_TEXTURE, OPCLASS_TEXTURE,
   OPCLASS_TEXTURE, OPCLASS_TEXTURE,
   // SULDB, SULDP, SUSTB, SUSTP; SUREDB, SUREDP, SULEA
   OPCLASS_SURFACE, OPCLASS_SURFACE, OPCLASS_ATOMIC, OPCLASS_SURFACE,
   OPCLASS_SURFACE, OPCLASS_SURFACE, OPCLASS_SURFACE,
   // SUBFM, SUCLAMP, SUEAU, SUQ, MADSP
   OPCLASS_OTHER, OPCLASS_OTHER, OPCLASS_OTHER, OPCLASS_OTHER, OPCLASS_ARITH,
   // TEXBAR
   OPCLASS_OTHER,
   // DFDX, DFDY, RDSV; PIXLD, QUADOP, QUADON, QUADPOP
   OPCLASS_OTHER, OPCLASS_OTHER, OPCLASS_OTHER,
   OPCLASS_OTHER, OPCLASS_OTHER, OPCLASS_CONTROL, OPCLASS_CONTROL,
   // POPCNT, INSBF, EXTBF, BFIND, BREV, BMSK; PERMT, SGXT
   OPCLASS_BITFIELD, OPCLASS_BITFIELD, OPCLASS_BITFIELD, OPCLASS_BITFIELD,
   OPCLASS_BITFIELD, OPCLASS_BITFIELD, OPCLASS_BITFIELD, OPCLASS_BITFIELD,
   // ATOM, BAR
   OPCLASS_ATOMIC, OPCLASS_CONTROL,
   // VADD, VAVG, VMIN, VMAX
   OPCLASS_VECTOR, OPCLASS_VECTOR, OPCLASS_VECTOR, OPCLASS_VECTOR,
   // VSAD, VSET, VSHR, VSHL
   OPCLASS_VECTOR, OPCLASS_VECTOR, OPCLASS_VECTOR, OPCLASS_VECTOR,
   // VSEL, CCTL
   OPCLASS_VECTOR, OPCLASS_CONTROL,
   // SHFL
   OPCLASS_OTHER,
   // VOTE
   OPCLASS_OTHER,
   // BUFQ
   OPCLASS_OTHER,
   // WARPSYNC
   OPCLASS_OTHER,
      };
         extern Target *getTargetGV100(unsigned int chipset);
   extern Target *getTargetGM107(unsigned int chipset);
   extern Target *getTargetNVC0(unsigned int chipset);
   extern Target *getTargetNV50(unsigned int chipset);
      Target *Target::create(unsigned int chipset)
   {
      STATIC_ASSERT(ARRAY_SIZE(operationSrcNr) == OP_LAST + 1);
   STATIC_ASSERT(ARRAY_SIZE(operationClass) == OP_LAST + 1);
   switch (chipset & ~0xf) {
   case 0x190:
   case 0x170:
   case 0x160:
   case 0x140:
         case 0x110:
   case 0x120:
   case 0x130:
         case 0xc0:
   case 0xd0:
   case 0xe0:
   case 0xf0:
   case 0x100:
         case 0x50:
   case 0x80:
   case 0x90:
   case 0xa0:
         default:
      ERROR("unsupported target: NV%x\n", chipset);
         }
      void Target::destroy(Target *targ)
   {
         }
      CodeEmitter::CodeEmitter(const Target *target) : targ(target), code(NULL),
         {
   }
      void
   CodeEmitter::setCodeLocation(void *ptr, uint32_t size)
   {
      code = reinterpret_cast<uint32_t *>(ptr);
   codeSize = 0;
      }
      void
   CodeEmitter::printBinary() const
   {
      uint32_t *bin = code - codeSize / 4;
   INFO("program binary (%u bytes)", codeSize);
   for (unsigned int pos = 0; pos < codeSize / 4; ++pos) {
      if ((pos % 8) == 0)
            }
      }
      static inline uint32_t sizeToBundlesNVE4(uint32_t size)
   {
         }
      void
   CodeEmitter::prepareEmission(Program *prog)
   {
      for (ArrayList::Iterator fi = prog->allFuncs.iterator();
      !fi.end(); fi.next()) {
   Function *func = reinterpret_cast<Function *>(fi.get());
   func->binPos = prog->binSize;
            // adjust sizes & positions for scheduling info:
   if (prog->getTarget()->hasSWSched) {
      uint32_t adjPos = func->binPos;
   BasicBlock *bb = NULL;
   for (int i = 0; i < func->bbCount; ++i) {
      bb = func->bbArray[i];
   int32_t adjSize = bb->binSize;
   if (adjPos % 64) {
      adjSize -= 64 - adjPos % 64;
   if (adjSize < 0)
      }
   adjSize = bb->binSize + sizeToBundlesNVE4(adjSize) * 8;
   bb->binPos = adjPos;
   bb->binSize = adjSize;
      }
   if (bb)
                     }
      void
   CodeEmitter::prepareEmission(Function *func)
   {
      func->bbCount = 0;
                     for (IteratorRef it = func->cfg.iteratorCFG(); !it->end(); it->next())
      }
      void
   CodeEmitter::prepareEmission(BasicBlock *bb)
   {
      Instruction *i, *next;
   Function *func = bb->getFunction();
   int j;
                     for (; j >= 0; --j) {
      BasicBlock *in = func->bbArray[j];
            if (exit && exit->op == OP_BRA && exit->asFlow()->target.bb == bb) {
                                       }
   bb->binPos = in->binPos + in->binSize;
   if (in->binSize) // no more no-op branches to bb
      }
            if (!bb->getExit())
            // determine encoding size, try to group short instructions
   nShort = 0;
   for (i = bb->getEntry(); i; i = next) {
               i->encSize = getMinEncodingSize(i);
   if (next && i->encSize < 8)
         else
   if ((nShort & 1) && next && getMinEncodingSize(next) == 4) {
      if (i->isCommutationLegal(i->next)) {
      bb->permuteAdjacent(i, next);
   next->encSize = 4;
   next = i;
   i = i->prev;
      } else
   if (i->isCommutationLegal(i->prev) && next->next) {
      bb->permuteAdjacent(i->prev, i);
   next->encSize = 4;
   next = next->next;
   bb->binSize += 4;
      } else {
      i->encSize = 8;
   i->prev->encSize = 8;
   bb->binSize += 4;
         } else {
      i->encSize = 8;
   if (nShort & 1) {
      i->prev->encSize = 8;
      }
      }
               if (bb->getExit()->encSize == 4) {
      assert(nShort);
   bb->getExit()->encSize = 8;
            if ((bb->getExit()->prev->encSize == 4) && !(nShort & 1)) {
      bb->binSize += 8;
         }
               }
      bool
   Program::emitBinary(struct nv50_ir_prog_info_out *info)
   {
                        if (dbgFlags & NV50_IR_DEBUG_BASIC)
            if (!binSize) {
      code = NULL;
      }
   code = reinterpret_cast<uint32_t *>(MALLOC(binSize));
   if (!code)
         emit->setCodeLocation(code, binSize);
            for (ArrayList::Iterator fi = allFuncs.iterator(); !fi.end(); fi.next()) {
                        for (int b = 0; b < fn->bbCount; ++b) {
      for (Instruction *i = fn->bbArray[b]->getEntry(); i; i = i->next) {
      emit->emitInstruction(i);
   info->bin.instructions++;
   if ((typeSizeof(i->sType) == 8 || typeSizeof(i->dType) == 8) &&
      (isFloatType(i->sType) || isFloatType(i->dType)))
         }
   info->io.fp64 |= fp64;
   info->bin.relocData = emit->getRelocInfo();
            // the nvc0 driver will print the binary itself together with the header
   if ((dbgFlags & NV50_IR_DEBUG_BASIC) && getTarget()->getChipset() < 0xc0)
            delete emit;
      }
      #define RELOC_ALLOC_INCREMENT 8
      bool
   CodeEmitter::addReloc(RelocEntry::Type ty, int w, uint32_t data, uint32_t m,
         {
               if (!(n % RELOC_ALLOC_INCREMENT)) {
      size_t size = sizeof(RelocInfo) + n * sizeof(RelocEntry);
   relocInfo = reinterpret_cast<RelocInfo *>(
      REALLOC(relocInfo, n ? size : 0,
      if (!relocInfo)
         if (n == 0)
      }
            relocInfo->entry[n].data = data;
   relocInfo->entry[n].mask = m;
   relocInfo->entry[n].offset = codeSize + w * 4;
   relocInfo->entry[n].bitPos = s;
               }
      bool
   CodeEmitter::addInterp(int ipa, int reg, FixupApply apply)
   {
               if (!(n % RELOC_ALLOC_INCREMENT)) {
      size_t size = sizeof(FixupInfo) + n * sizeof(FixupEntry);
   fixupInfo = reinterpret_cast<FixupInfo *>(
      REALLOC(fixupInfo, n ? size : 0,
      if (!fixupInfo)
         if (n == 0)
      }
                        }
      void
   RelocEntry::apply(uint32_t *binary, const RelocInfo *info) const
   {
               switch (type) {
   case TYPE_CODE: value = info->codePos; break;
   case TYPE_BUILTIN: value = info->libPos; break;
   case TYPE_DATA: value = info->dataPos; break;
   default:
      assert(0);
      }
   value += data;
            binary[offset / 4] &= ~mask;
      }
      } // namespace nv50_ir
         #include "nv50_ir_driver.h"
      extern "C" {
      void
   nv50_ir_relocate_code(void *relocData, uint32_t *code,
                     {
               info->codePos = codePos;
   info->libPos = libPos;
            for (unsigned int i = 0; i < info->count; ++i)
      }
      void
   nv50_ir_apply_fixups(void *fixupData, uint32_t *code,
               {
      nv50_ir::FixupInfo *info = reinterpret_cast<nv50_ir::FixupInfo *>(
            // force_persample_interp: all non-flat -> per-sample
   // flatshade: all color -> flat
   // alphatest: PIPE_FUNC_* to use with alphatest
   // msaa: false = sample id -> 0 for interpolateAtSample
   nv50_ir::FixupData data(force_persample_interp, flatshade, alphatest, msaa);
   for (unsigned i = 0; i < info->count; ++i)
      }
      void
   nv50_ir_get_target_library(uint32_t chipset,
         {
      nv50_ir::Target *targ = nv50_ir::Target::create(chipset);
   targ->getBuiltinCode(code, size);
      }
      }
