   /*
   * Copyright 2011 Christoph Bumiller
   *           2014 Red Hat Inc.
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
      #include "nv50_ir_target_gm107.h"
   #include "nv50_ir_lowering_gm107.h"
      namespace nv50_ir {
      Target *getTargetGM107(unsigned int chipset)
   {
         }
      // BULTINS / LIBRARY FUNCTIONS:
      // lazyness -> will just hardcode everything for the time being
      #include "lib/gm107.asm.h"
      void
   TargetGM107::getBuiltinCode(const uint32_t **code, uint32_t *size) const
   {
      *code = (const uint32_t *)&gm107_builtin_code[0];
      }
      uint32_t
   TargetGM107::getBuiltinOffset(int builtin) const
   {
      assert(builtin < NVC0_BUILTIN_COUNT);
      }
      bool
   TargetGM107::isOpSupported(operation op, DataType ty) const
   {
      switch (op) {
   case OP_SAD:
   case OP_DIV:
   case OP_MOD:
         case OP_SQRT:
      if (ty == TYPE_F64)
            case OP_XMAD:
      if (isFloatType(ty))
            default:
                     }
      // Return true when an instruction supports the reuse flag. When supported, the
   // hardware will use the operand reuse cache introduced since Maxwell, which
   // should try to reduce bank conflicts by caching values for the subsequent
   // instructions. Note that the next instructions have to use the same GPR id in
   // the same operand slot.
   bool
   TargetGM107::isReuseSupported(const Instruction *insn) const
   {
               // TODO: double-check!
   switch (cl) {
   case OPCLASS_ARITH:
   case OPCLASS_COMPARE:
   case OPCLASS_LOGIC:
   case OPCLASS_MOVE:
   case OPCLASS_SHIFT:
         case OPCLASS_BITFIELD:
      if (insn->op == OP_INSBF || insn->op == OP_EXTBF)
            default:
         }
      }
      // Return true when an instruction requires to set up a barrier because it
   // doesn't operate at a fixed latency. Variable latency instructions are memory
   // operations, double precision operations, special function unit operations
   // and other low throughput instructions.
   bool
   TargetGM107::isBarrierRequired(const Instruction *insn) const
   {
               if (insn->dType == TYPE_F64 || insn->sType == TYPE_F64)
            switch (cl) {
   case OPCLASS_ATOMIC:
   case OPCLASS_LOAD:
   case OPCLASS_STORE:
   case OPCLASS_SURFACE:
   case OPCLASS_TEXTURE:
         case OPCLASS_SFU:
      switch (insn->op) {
   case OP_COS:
   case OP_EX2:
   case OP_LG2:
   case OP_LINTERP:
   case OP_PINTERP:
   case OP_RCP:
   case OP_RSQ:
   case OP_SIN:
   case OP_SQRT:
         default:
         }
      case OPCLASS_BITFIELD:
      switch (insn->op) {
   case OP_BFIND:
   case OP_POPCNT:
         default:
         }
      case OPCLASS_CONTROL:
      switch (insn->op) {
   case OP_EMIT:
   case OP_RESTART:
         default:
         }
      case OPCLASS_OTHER:
      switch (insn->op) {
   case OP_AFETCH:
   case OP_PFETCH:
   case OP_PIXLD:
   case OP_SHFL:
         case OP_RDSV:
         default:
         }
      case OPCLASS_ARITH:
      if ((insn->op == OP_MUL || insn->op == OP_MAD) &&
      !isFloatType(insn->dType))
         case OPCLASS_CONVERT:
      if (insn->def(0).getFile() != FILE_PREDICATE &&
      insn->src(0).getFile() != FILE_PREDICATE)
         default:
         }
      }
      bool
   TargetGM107::canDualIssue(const Instruction *a, const Instruction *b) const
   {
      // TODO
      }
      // Return the number of stall counts needed to complete a single instruction.
   // On Maxwell GPUs, the pipeline depth is 6, but some instructions require
   // different number of stall counts like memory operations.
   int
   TargetGM107::getLatency(const Instruction *insn) const
   {
      // TODO: better values! This should be good enough for now though.
   switch (insn->op) {
   case OP_EMIT:
   case OP_EXPORT:
   case OP_PIXLD:
   case OP_RESTART:
   case OP_STORE:
   case OP_SUSTB:
   case OP_SUSTP:
         case OP_SHFL:
         case OP_ADD:
   case OP_AND:
   case OP_EXTBF:
   case OP_FMA:
   case OP_INSBF:
   case OP_MAD:
   case OP_MAX:
   case OP_MIN:
   case OP_MOV:
   case OP_MUL:
   case OP_NOT:
   case OP_OR:
   case OP_PREEX2:
   case OP_PRESIN:
   case OP_QUADOP:
   case OP_SELP:
   case OP_SET:
   case OP_SET_AND:
   case OP_SET_OR:
   case OP_SET_XOR:
   case OP_SHL:
   case OP_SHLADD:
   case OP_SHR:
   case OP_SLCT:
   case OP_SUB:
   case OP_VOTE:
   case OP_XOR:
   case OP_XMAD:
      if (insn->dType != TYPE_F64)
            case OP_RDSV:
         case OP_ABS:
   case OP_CEIL:
   case OP_CVT:
   case OP_FLOOR:
   case OP_NEG:
   case OP_SAT:
   case OP_TRUNC:
      if (insn->op == OP_CVT && (insn->def(0).getFile() == FILE_PREDICATE ||
                  case OP_BFIND:
   case OP_COS:
   case OP_EX2:
   case OP_LG2:
   case OP_POPCNT:
   case OP_QUADON:
   case OP_QUADPOP:
   case OP_RCP:
   case OP_RSQ:
   case OP_SIN:
   case OP_SQRT:
         default:
         }
   // Use the maximum number of stall counts for other instructions.
      }
      // Return the operand read latency which is the number of stall counts before
   // an instruction can read its sources. For memory operations like ATOM, LOAD
   // and STORE, the memory access has to be indirect.
   int
   TargetGM107::getReadLatency(const Instruction *insn) const
   {
      switch (insn->op) {
   case OP_ABS:
   case OP_BFIND:
   case OP_CEIL:
   case OP_COS:
   case OP_EX2:
   case OP_FLOOR:
   case OP_LG2:
   case OP_NEG:
   case OP_POPCNT:
   case OP_RCP:
   case OP_RSQ:
   case OP_SAT:
   case OP_SIN:
   case OP_SQRT:
   case OP_SULDB:
   case OP_SULDP:
   case OP_SUREDB:
   case OP_SUREDP:
   case OP_SUSTB:
   case OP_SUSTP:
   case OP_TRUNC:
         case OP_CVT:
      if (insn->def(0).getFile() != FILE_PREDICATE &&
      insn->src(0).getFile() != FILE_PREDICATE)
         case OP_ATOM:
   case OP_LOAD:
   case OP_STORE:
      if (insn->src(0).isIndirect(0)) {
      switch (insn->src(0).getFile()) {
   case FILE_MEMORY_SHARED:
   case FILE_MEMORY_CONST:
         case FILE_MEMORY_GLOBAL:
   case FILE_MEMORY_LOCAL:
         default:
            }
      case OP_EXPORT:
   case OP_PFETCH:
   case OP_SHFL:
   case OP_VFETCH:
         default:
         }
      }
      bool
   TargetGM107::isCS2RSV(SVSemantic sv) const
   {
         }
      bool
   TargetGM107::runLegalizePass(Program *prog, CGStage stage) const
   {
      if (stage == CG_STAGE_PRE_SSA) {
      GM107LoweringPass pass(prog);
      } else
   if (stage == CG_STAGE_POST_RA) {
      NVC0LegalizePostRA pass(prog);
      } else
   if (stage == CG_STAGE_SSA) {
      GM107LegalizeSSA pass;
      }
      }
      CodeEmitter *
   TargetGM107::getCodeEmitter(Program::Type type)
   {
         }
      } // namespace nv50_ir
