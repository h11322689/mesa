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
   #include "nv50_ir_driver.h"
      #include <inttypes.h>
      namespace nv50_ir {
      enum TextStyle
   {
      TXT_DEFAULT,
   TXT_GPR,
   TXT_REGISTER,
   TXT_FLAGS,
   TXT_MEM,
   TXT_IMMD,
   TXT_BRA,
      };
      static const char *_colour[8] =
   {
      "\x1b[00m",
   "\x1b[34m",
   "\x1b[35m",
   "\x1b[35m",
   "\x1b[36m",
   "\x1b[33m",
   "\x1b[37m",
      };
      static const char *_nocolour[8] =
   {
         };
      static const char **colour;
      static void init_colours()
   {
      if (getenv("NV50_PROG_DEBUG_NO_COLORS") != NULL)
         else
      }
      const char *operationStr[OP_LAST + 1] =
   {
      "nop",
   "phi",
   "union",
   "split",
   "merge",
   "mov",
   "ld",
   "st",
   "add",
   "sub",
   "mul",
   "div",
   "mod",
   "mad",
   "fma",
   "sad",
   "shladd",
   "xmad",
   "abs",
   "neg",
   "not",
   "and",
   "or",
   "xor",
   "lop3 lut",
   "shl",
   "shr",
   "shf",
   "max",
   "min",
   "sat",
   "ceil",
   "floor",
   "trunc",
   "cvt",
   "set and",
   "set or",
   "set xor",
   "set",
   "selp",
   "slct",
   "rcp",
   "rsq",
   "lg2",
   "sin",
   "cos",
   "ex2",
   "presin",
   "preex2",
   "sqrt",
   "bra",
   "call",
   "ret",
   "cont",
   "break",
   "preret",
   "precont",
   "prebreak",
   "brkpt",
   "joinat",
   "join",
   "discard",
   "exit",
   "membar",
   "vfetch",
   "pfetch",
   "afetch",
   "export",
   "linterp",
   "pinterp",
   "emit",
   "restart",
   "final",
   "tex",
   "texbias",
   "texlod",
   "texfetch",
   "texquery",
   "texgrad",
   "texgather",
   "texquerylod",
   "texcsaa",
   "texprep",
   "suldb",
   "suldp",
   "sustb",
   "sustp",
   "suredb",
   "suredp",
   "sulea",
   "subfm",
   "suclamp",
   "sueau",
   "suq",
   "madsp",
   "texbar",
   "dfdx",
   "dfdy",
   "rdsv",
   "pixld",
   "quadop",
   "quadon",
   "quadpop",
   "popcnt",
   "insbf",
   "extbf",
   "bfind",
   "brev",
   "bmsk",
   "permt",
   "sgxt",
   "atom",
   "bar",
   "vadd",
   "vavg",
   "vmin",
   "vmax",
   "vsad",
   "vset",
   "vshr",
   "vshl",
   "vsel",
   "cctl",
   "shfl",
   "vote",
   "bufq",
   "warpsync",
      };
      static const char *atomSubOpStr[] =
   {
         };
      static const char *ldstSubOpStr[] =
   {
         };
      static const char *subfmOpStr[] =
   {
         };
      static const char *shflOpStr[] =
   {
   "idx", "up", "down", "bfly"
   };
      static const char *pixldOpStr[] =
   {
         };
      static const char *rcprsqOpStr[] =
   {
         };
      static const char *emitOpStr[] =
   {
         };
      static const char *cctlOpStr[] =
   {
         };
      static const char *barOpStr[] =
   {
         };
      static const char *xmadOpCModeStr[] =
   {
         };
      static const char *DataTypeStr[] =
   {
      "-",
   "u8", "s8",
   "u16", "s16",
   "u32", "s32",
   "u64", "s64",
   "f16", "f32", "f64",
      };
      static const char *RoundModeStr[] =
   {
         };
      static const char *CondCodeStr[] =
   {
      "never",
   "lt",
   "eq",
   "le",
   "gt",
   "ne",
   "ge",
   "",
   "(invalid)",
   "ltu",
   "equ",
   "leu",
   "gtu",
   "neu",
   "geu",
   "",
   "no",
   "nc",
   "ns",
   "na",
   "a",
   "s",
   "c",
      };
      static const char *SemanticStr[] =
   {
      "POSITION",
   "VERTEX_ID",
   "INSTANCE_ID",
   "INVOCATION_ID",
   "PRIMITIVE_ID",
   "VERTEX_COUNT",
   "LAYER",
   "VIEWPORT_INDEX",
   "VIEWPORT_MASK",
   "Y_DIR",
   "FACE",
   "POINT_SIZE",
   "POINT_COORD",
   "CLIP_DISTANCE",
   "SAMPLE_INDEX",
   "SAMPLE_POS",
   "SAMPLE_MASK",
   "TESS_OUTER",
   "TESS_INNER",
   "TESS_COORD",
   "TID",
   "COMBINED_TID",
   "CTAID",
   "NTID",
   "GRIDID",
   "NCTAID",
   "LANEID",
   "PHYSID",
   "NPHYSID",
   "CLOCK",
   "LBASE",
   "SBASE",
   "VERTEX_STRIDE",
   "INVOCATION_INFO",
   "THREAD_KILL",
   "BASEVERTEX",
   "BASEINSTANCE",
   "DRAWID",
   "WORK_DIM",
   "LANEMASK_EQ",
   "LANEMASK_LT",
   "LANEMASK_LE",
   "LANEMASK_GT",
   "LANEMASK_GE",
   "?",
      };
      static const char *TSStr[17] =
   {
      "THREAD_STATE_ENUM0",
   "THREAD_STATE_ENUM1",
   "THREAD_STATE_ENUM2",
   "THREAD_STATE_ENUM3",
   "THREAD_STATE_ENUM4",
   "TRAP_RETURN_PC_LO",
   "TRAP_RETURN_PC_HI",
   "TRAP_RETURN_MASK",
   "MEXITED",
   "MKILL",
   "MACTIVE",
   "MATEXIT",
   "OPT_STACK",
   "API_CALL_DEPTH",
   "ATEXIT_PC_LO",
   "ATEXIT_PC_HI",
      };
      static const char *interpStr[16] =
   {
      "pass",
   "mul",
   "flat",
   "sc",
   "cent pass",
   "cent mul",
   "cent flat",
   "cent sc",
   "off pass",
   "off mul",
   "off flat",
   "off sc",
   "samp pass",
   "samp mul",
   "samp flat",
      };
      static const char *texMaskStr[16] =
   {
      "____",
   "r___",
   "_g__",
   "rg__",
   "__b_",
   "r_b_",
   "_gb_",
   "rgb_",
   "___a",
   "r__a",
   "_g_a",
   "rg_a",
   "__ba",
   "r_ba",
   "_gba",
      };
      static const char *gatherCompStr[4] =
   {
         };
      #define PRINT(args...)                                \
      do {                                               \
               #define SPACE_PRINT(cond, args...)                      \
      do {                                                 \
      if (cond)                                         \
                  #define SPACE()                                    \
      do {                                            \
      if (pos < size)                              \
            int Modifier::print(char *buf, size_t size) const
   {
               if (bits)
                     if (bits & NV50_IR_MOD_NOT)
         if (bits & NV50_IR_MOD_SAT)
         if (bits & NV50_IR_MOD_NEG)
         if (bits & NV50_IR_MOD_ABS)
               }
      int LValue::print(char *buf, size_t size, DataType ty) const
   {
      const char *postFix = "";
   size_t pos = 0;
   int idx = join->reg.data.id >= 0 ? join->reg.data.id : id;
   char p = join->reg.data.id >= 0 ? '$' : '%';
   char r;
            switch (reg.file) {
   case FILE_GPR:
      r = 'r'; col = TXT_GPR;
   if (reg.size == 2) {
      if (p == '$') {
      postFix = (idx & 1) ? "h" : "l";
      } else {
            } else
   if (reg.size == 8) {
         } else
   if (reg.size == 16) {
         } else
   if (reg.size == 12) {
         }
      case FILE_PREDICATE:
      r = 'p'; col = TXT_REGISTER;
   if (reg.size == 2)
         else
   if (reg.size == 4)
            case FILE_FLAGS:
      r = 'c'; col = TXT_FLAGS;
      case FILE_ADDRESS:
      r = 'a'; col = TXT_REGISTER;
      case FILE_BARRIER:
      r = 'b'; col = TXT_REGISTER;
      default:
      assert(!"invalid file for lvalue");
   r = '?';
                           }
      int ImmediateValue::print(char *buf, size_t size, DataType ty) const
   {
                        switch (ty) {
   case TYPE_F32: PRINT("%f", reg.data.f32); break;
   case TYPE_F64: PRINT("%f", reg.data.f64); break;
   case TYPE_U8:  PRINT("0x%02x", reg.data.u8); break;
   case TYPE_S8:  PRINT("%i", reg.data.s8); break;
   case TYPE_U16: PRINT("0x%04x", reg.data.u16); break;
   case TYPE_S16: PRINT("%i", reg.data.s16); break;
   case TYPE_U32: PRINT("0x%08x", reg.data.u32); break;
   case TYPE_S32: PRINT("%i", reg.data.s32); break;
   case TYPE_U64:
   case TYPE_S64:
   default:
      PRINT("0x%016" PRIx64, reg.data.u64);
      }
      }
      int Symbol::print(char *buf, size_t size, DataType ty) const
   {
         }
      int Symbol::print(char *buf, size_t size,
         {
               size_t pos = 0;
            if (ty == TYPE_NONE)
            if (reg.file == FILE_SYSTEM_VALUE) {
      PRINT("%ssv[%s%s:%i%s", colour[TXT_MEM],
         colour[TXT_REGISTER],
   if (rel) {
      PRINT("%s+", colour[TXT_DEFAULT]);
      }
   PRINT("%s]", colour[TXT_MEM]);
      } else if (reg.file == FILE_THREAD_STATE) {
      PRINT("%sts[%s%s%s]", colour[TXT_MEM], colour[TXT_REGISTER],
                     switch (reg.file) {
   case FILE_MEMORY_CONST:  c = 'c'; break;
   case FILE_SHADER_INPUT:  c = 'a'; break;
   case FILE_SHADER_OUTPUT: c = 'o'; break;
   case FILE_MEMORY_BUFFER: c = 'b'; break; // Only used before lowering
   case FILE_MEMORY_GLOBAL: c = 'g'; break;
   case FILE_MEMORY_SHARED: c = 's'; break;
   case FILE_MEMORY_LOCAL:  c = 'l'; break;
   case FILE_BARRIER:       c = 'b'; break;
   default:
      assert(!"invalid file");
   c = '?';
               if (c == 'c')
         else
            if (dimRel) {
      pos += dimRel->print(&buf[pos], size - pos, TYPE_S32);
               if (rel) {
      pos += rel->print(&buf[pos], size - pos);
      } else {
         }
               }
      void Instruction::print() const
   {
                        char buf[BUFSZ];
   int s, d;
                     if (join)
            if (predSrc >= 0) {
      const size_t pre = pos;
   if (getSrc(predSrc)->reg.file == FILE_PREDICATE) {
      if (cc == CC_NOT_P)
      } else {
         }
   if (pos > pre)
         pos += getSrc(predSrc)->print(&buf[pos], BUFSZ - pos);
               if (saturate)
            if (asFlow()) {
      PRINT("%s", operationStr[op]);
   if (asFlow()->indirect)
         if (asFlow()->absolute)
         if (op == OP_CALL && asFlow()->builtin) {
         } else
   if (op == OP_CALL && asFlow()->target.fn) {
      PRINT(" %s%s:%i", colour[TXT_BRA],
            } else
   if (asFlow()->target.bb)
      } else {
      if (asTex())
         else
         if (op == OP_LINTERP || op == OP_PINTERP)
         switch (op) {
   case OP_SUREDP:
   case OP_SUREDB:
   case OP_ATOM:
      if (subOp < ARRAY_SIZE(atomSubOpStr))
            case OP_LOAD:
   case OP_STORE:
      if (subOp < ARRAY_SIZE(ldstSubOpStr))
            case OP_SUBFM:
      if (subOp < ARRAY_SIZE(subfmOpStr))
            case OP_SHFL:
      if (subOp < ARRAY_SIZE(shflOpStr))
            case OP_PIXLD:
      if (subOp < ARRAY_SIZE(pixldOpStr))
            case OP_RCP:
   case OP_RSQ:
      if (subOp < ARRAY_SIZE(rcprsqOpStr))
            case OP_EMIT:
      if (subOp < ARRAY_SIZE(emitOpStr))
            case OP_CCTL:
      if (subOp < ARRAY_SIZE(cctlOpStr))
            case OP_BAR:
      if (subOp < ARRAY_SIZE(barOpStr))
            case OP_XMAD: {
      if (subOp & NV50_IR_SUBOP_XMAD_PSL)
         if (subOp & NV50_IR_SUBOP_XMAD_MRG)
         unsigned cmode = (subOp & NV50_IR_SUBOP_XMAD_CMODE_MASK);
   cmode >>= NV50_IR_SUBOP_XMAD_CMODE_SHIFT;
   if (cmode && cmode <= ARRAY_SIZE(xmadOpCModeStr))
         for (int i = 0; i < 2; i++)
            }
   default:
      if (subOp)
            }
   if (perPatch)
         if (asTex()) {
      PRINT("%s %s$r%u $s%u ", asTex()->tex.target.getName(),
         if (op == OP_TXG)
                     if (postFactor)
                     if (rnd != ROUND_N)
            if (defExists(1))
         for (d = 0; defExists(d); ++d) {
      SPACE();
      }
   if (d > 1)
         else
   if (!d && !asFlow())
            if (asCmp())
            if (sType != dType)
            for (s = 0; srcExists(s); ++s) {
      if (s == predSrc || src(s).usedAsPtr)
         const size_t pre = pos;
   SPACE();
   pos += src(s).mod.print(&buf[pos], BUFSZ - pos);
   if (pos > pre + 1)
         if (src(s).isIndirect(0) || src(s).isIndirect(1))
      pos += getSrc(s)->asSym()->print(&buf[pos], BUFSZ - pos,
            else
      }
   if (exit)
                                 }
      class PrintPass : public Pass
   {
   public:
               virtual bool visit(Function *);
   virtual bool visit(BasicBlock *);
         private:
      int serial;
      };
      bool
   PrintPass::visit(Function *fn)
   {
                        if (!fn->outs.empty())
         for (std::deque<ValueRef>::iterator it = fn->outs.begin();
      it != fn->outs.end();
   ++it) {
   it->get()->print(str, sizeof(str), typeOfSize(it->get()->reg.size));
               if (!fn->ins.empty())
         for (std::deque<ValueDef>::iterator it = fn->ins.begin();
      it != fn->ins.end();
   ++it) {
   it->get()->print(str, sizeof(str), typeOfSize(it->get()->reg.size));
      }
               }
      bool
   PrintPass::visit(BasicBlock *bb)
   {
   #if 0
      INFO("---\n");
   for (Graph::EdgeIterator ei = bb->cfg.incident(); !ei.end(); ei.next())
      INFO(" <- BB:%i (%s)\n",
         #endif
               if (bb->idom())
            INFO("df = { ");
   for (DLList::Iterator df = bb->getDF().iterator(); !df.end(); df.next())
                     for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next())
      INFO(" -> BB:%i (%s)\n",
                  }
      bool
   PrintPass::visit(Instruction *insn)
   {
      if (omit_serial)
         else
         serial++;
   insn->print();
      }
      void
   Function::print()
   {
      PrintPass pass(prog->driver->omitLineNum);
      }
      void
   Program::print()
   {
      PrintPass pass(driver->omitLineNum);
   init_colours();
      }
      void
   Function::printLiveIntervals() const
   {
               for (ArrayList::Iterator it = allLValues.iterator(); !it.end(); it.next()) {
      const Value *lval = Value::get(it)->asLValue();
   if (lval && !lval->livei.isEmpty()) {
      INFO("livei(%%%i): ", lval->id);
            }
      } // namespace nv50_ir
      extern void
   nv50_ir_prog_info_out_print(struct nv50_ir_prog_info_out *info_out)
   {
               INFO("{\n");
   INFO("   \"target\":\"%d\",\n", info_out->target);
            // Bin
   INFO("   \"bin\":{\n");
   INFO("      \"maxGPR\":\"%d\",\n", info_out->bin.maxGPR);
   INFO("      \"tlsSpace\":\"%d\",\n", info_out->bin.tlsSpace);
   INFO("      \"smemSize\":\"%d\",\n", info_out->bin.smemSize);
   INFO("      \"codeSize\":\"%d\",\n", info_out->bin.codeSize);
            // RelocInfo
   INFO("      \"RelocInfo\":");
   if (!info_out->bin.relocData) {
         } else {
      nv50_ir::RelocInfo *reloc = (nv50_ir::RelocInfo *)info_out->bin.relocData;
   INFO("{\n");
   INFO("         \"codePos\":\"%d\",\n", reloc->codePos);
   INFO("         \"libPos\":\"%d\",\n", reloc->libPos);
   INFO("         \"dataPos\":\"%d\",\n", reloc->dataPos);
   INFO("         \"count\":\"%d\",\n", reloc->count);
   INFO("         \"RelocEntry\":[\n");
   for (unsigned int i = 0; i < reloc->count; i++) {
      INFO("            {\"data\":\"%d\",\t\"mask\":\"%d\",\t\"offset\":\"%d\",\t\"bitPos\":\"%d\",\t\"type\":\"%d\"}",
            }
   INFO("\n");
   INFO("         ]\n");
               // FixupInfo
   INFO("      \"FixupInfo\":");
   if (!info_out->bin.fixupData) {
         } else {
      nv50_ir::FixupInfo *fixup = (nv50_ir::FixupInfo *)info_out->bin.fixupData;
   INFO("{\n");
   INFO("         \"count\":\"%d\"\n", fixup->count);
   INFO("         \"FixupEntry\":[\n");
   for (unsigned int i = 0; i < fixup->count; i++) {
      INFO("            {\"apply\":\"%p\",\t\"ipa\":\"%d\",\t\"reg\":\"%d\",\t\"loc\":\"%d\"}\n",
      }
   INFO("\n");
   INFO("         ]\n");
                        if (info_out->numSysVals) {
      INFO("   \"sv\":[\n");
   for (i = 0; i < info_out->numSysVals; i++) {
         INFO("      {\"sn\":\"%d\"}\n",
   }
      }
   if (info_out->numInputs) {
      INFO("   \"in\":[\n");
   for (i = 0; i < info_out->numInputs; i++) {
      INFO("      {\"id\":\"%d\",\t\"sn\":\"%d\",\t\"si\":\"%d\"}\n",
      }
      }
   if (info_out->numOutputs) {
      INFO("   \"out\":[\n");
   for (i = 0; i < info_out->numOutputs; i++) {
      INFO("      {\"id\":\"%d\",\t\"sn\":\"%d\",\t\"si\":\"%d\"}\n",
      }
               INFO("   \"numInputs\":\"%d\",\n", info_out->numInputs);
   INFO("   \"numOutputs\":\"%d\",\n", info_out->numOutputs);
   INFO("   \"numPatchConstants\":\"%d\",\n", info_out->numPatchConstants);
            INFO("   \"prop\":{\n");
   switch (info_out->type) {
      case PIPE_SHADER_VERTEX:
      INFO("      \"vp\": {\"usesDrawParameters\":\"%s\"}\n",
            case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL:
      INFO("      \"tp\":{\n");
   INFO("         \"outputPatchSize\":\"%d\"\n", info_out->prop.tp.outputPatchSize);
   INFO("         \"partitioning\":\"%d\"\n", info_out->prop.tp.partitioning);
   INFO("         \"winding\":\"%d\"\n", info_out->prop.tp.winding);
   INFO("         \"domain\":\"%d\"\n", info_out->prop.tp.domain);
   INFO("         \"outputPrim\":\"%d\"\n", info_out->prop.tp.outputPrim);
      case PIPE_SHADER_GEOMETRY:
      INFO("      \"gp\":{\n");
   INFO("         \"outputPrim\":\"%d\"\n", info_out->prop.gp.outputPrim);
   INFO("         \"instancesCount\":\"%d\"\n", info_out->prop.gp.instanceCount);
   INFO("         \"maxVertices\":\"%d\"\n", info_out->prop.gp.maxVertices);
      case PIPE_SHADER_FRAGMENT:
      INFO("      \"fp\":{\n");
   INFO("         \"numColourResults\":\"%d\"\n", info_out->prop.fp.numColourResults);
   INFO("         \"writesDepth\":\"%s\"\n", info_out->prop.fp.writesDepth ? "true" : "false");
   INFO("         \"earlyFragTests\":\"%s\"\n", info_out->prop.fp.earlyFragTests ? "true" : "false");
   INFO("         \"postDepthCoverage\":\"%s\"\n", info_out->prop.fp.postDepthCoverage ? "true" : "false");
   INFO("         \"usesDiscard\":\"%s\"\n", info_out->prop.fp.usesDiscard ? "true" : "false");
   INFO("         \"usesSampleMaskIn\":\"%s\"\n", info_out->prop.fp.usesSampleMaskIn ? "true" : "false");
   INFO("         \"readsFramebuffer\":\"%s\"\n", info_out->prop.fp.readsFramebuffer ? "true" : "false");
   INFO("         \"readsSampleLocations\":\"%s\"\n", info_out->prop.fp.readsSampleLocations ? "true" : "false");
   INFO("         \"separateFragData\":\"%s\"\n", info_out->prop.fp.separateFragData ? "true" : "false");
      default:
      }
   INFO("      }\n");
            INFO("   \"io\":{\n");
   INFO("      \"clipDistances\":\"%d\"\n", info_out->io.clipDistances);
   INFO("      \"cullDistances\":\"%d\"\n", info_out->io.cullDistances);
   INFO("      \"genUserClip\":\"%d\"\n", info_out->io.genUserClip);
   INFO("      \"instanceId\":\"%d\"\n", info_out->io.instanceId);
   INFO("      \"vertexId\":\"%d\"\n", info_out->io.vertexId);
   INFO("      \"edgeFlagIn\":\"%d\"\n", info_out->io.edgeFlagIn);
   INFO("      \"edgeFlagOut\":\"%d\"\n", info_out->io.edgeFlagOut);
   INFO("      \"fragDepth\":\"%d\"\n", info_out->io.fragDepth);
   INFO("      \"sampleMask\":\"%d\"\n", info_out->io.sampleMask);
   INFO("      \"globalAccess\":\"%d\"\n", info_out->io.globalAccess);
   INFO("      \"fp64\":\"%s\"\n", info_out->io.fp64 ? "true" : "false");
   INFO("      \"layer_viewport_relative\":\"%s\"\n", info_out->io.layer_viewport_relative ? "true" : "false");
   INFO("   \"}\n");
   INFO("   \"numBarriers\":\"%d\"\n", info_out->numBarriers);
               }
