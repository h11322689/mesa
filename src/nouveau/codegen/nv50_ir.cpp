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
      namespace nv50_ir {
      Modifier::Modifier(operation op)
   {
      switch (op) {
   case OP_NEG: bits = NV50_IR_MOD_NEG; break;
   case OP_ABS: bits = NV50_IR_MOD_ABS; break;
   case OP_SAT: bits = NV50_IR_MOD_SAT; break;
   case OP_NOT: bits = NV50_IR_MOD_NOT; break;
   default:
      bits = 0;
         }
      Modifier Modifier::operator*(const Modifier m) const
   {
               b = m.bits;
   if (this->bits & NV50_IR_MOD_ABS)
            a = (this->bits ^ b)      & (NV50_IR_MOD_NOT | NV50_IR_MOD_NEG);
               }
      ValueRef::ValueRef(Value *v) : value(NULL), insn(NULL)
   {
      indirect[0] = -1;
   indirect[1] = -1;
   usedAsPtr = false;
      }
      ValueRef::ValueRef(const ValueRef& ref) : value(NULL), insn(ref.insn)
   {
      set(ref);
      }
      ValueRef::~ValueRef()
   {
         }
      bool ValueRef::getImmediate(ImmediateValue &imm) const
   {
      const ValueRef *src = this;
   Modifier m;
            while (src) {
      if (src->mod) {
      if (src->insn->sType != type)
            }
   if (src->getFile() == FILE_IMMEDIATE) {
      imm = *(src->value->asImm());
   // The immediate's type isn't required to match its use, it's
   // more of a hint; applying a modifier makes use of that hint.
   imm.reg.type = type;
   m.applyTo(imm);
                        if (insn && insn->op == OP_MOV) {
      src = &insn->src(0);
   if (src->mod)
      } else {
            }
      }
      ValueDef::ValueDef(Value *v) : value(NULL), origin(NULL), insn(NULL)
   {
         }
      ValueDef::ValueDef(const ValueDef& def) : value(NULL), origin(NULL), insn(NULL)
   {
         }
      ValueDef::~ValueDef()
   {
         }
      void
   ValueRef::set(const ValueRef &ref)
   {
      this->set(ref.get());
   mod = ref.mod;
   indirect[0] = ref.indirect[0];
      }
      void
   ValueRef::set(Value *refVal)
   {
      if (value == refVal)
         if (value)
         if (refVal)
               }
      void
   ValueDef::set(Value *defVal)
   {
      if (value == defVal)
         if (value)
         if (defVal)
               }
      // Check if we can replace this definition's value by the value in @rep,
   // including the source modifiers, i.e. make sure that all uses support
   // @rep.mod.
   bool
   ValueDef::mayReplace(const ValueRef &rep)
   {
      if (!rep.mod)
            if (!insn || !insn->bb) // Unbound instruction ?
                     for (Value::UseIterator it = value->uses.begin(); it != value->uses.end();
      ++it) {
   Instruction *insn = (*it)->getInsn();
            for (int i = 0; insn->srcExists(i); ++i) {
      if (insn->src(i).get() == value) {
      // If there are multiple references to us we'd have to check if the
   // combination of mods is still supported, but just bail for now.
   if (&insn->src(i) != (*it))
               }
            if (!target->isModSupported(insn, s, rep.mod))
      }
      }
      void
   ValueDef::replace(const ValueRef &repVal, bool doSet)
   {
               if (value == repVal.get())
            while (!value->uses.empty()) {
      ValueRef *ref = *value->uses.begin();
   ref->set(repVal.get());
               if (doSet)
      }
      Value::Value() : id(-1)
   {
   join = this;
   memset(&reg, 0, sizeof(reg));
   reg.size = 4;
   }
      LValue::LValue(Function *fn, DataFile file)
   {
      reg.file = file;
   reg.size = (file != FILE_PREDICATE) ? 4 : 1;
            compMask = 0;
   compound = 0;
   ssa = 0;
   fixedReg = 0;
               }
      LValue::LValue(Function *fn, LValue *lval)
   {
               reg.file = lval->reg.file;
   reg.size = lval->reg.size;
            compMask = 0;
   compound = 0;
   ssa = 0;
   fixedReg = 0;
               }
      LValue *
   LValue::clone(ClonePolicy<Function>& pol) const
   {
                        that->reg.size = this->reg.size;
   that->reg.type = this->reg.type;
               }
      bool
   LValue::isUniform() const
   {
      if (defs.size() > 1)
         Instruction *insn = getInsn();
   if (!insn)
         // let's not try too hard here for now ...
      }
      Symbol::Symbol(Program *prog, DataFile f, uint8_t fidx)
   {
               reg.file = f;
   reg.fileIndex = fidx;
               }
      Symbol *
   Symbol::clone(ClonePolicy<Function>& pol) const
   {
                                 that->reg.size = this->reg.size;
   that->reg.type = this->reg.type;
                        }
      bool
   Symbol::isUniform() const
   {
      return
      reg.file != FILE_SYSTEM_VALUE &&
   reg.file != FILE_MEMORY_LOCAL &&
   }
      ImmediateValue::ImmediateValue(Program *prog, uint32_t uval)
   {
               reg.file = FILE_IMMEDIATE;
   reg.size = 4;
                        }
      ImmediateValue::ImmediateValue(Program *prog, float fval)
   {
               reg.file = FILE_IMMEDIATE;
   reg.size = 4;
                        }
      ImmediateValue::ImmediateValue(Program *prog, double dval)
   {
               reg.file = FILE_IMMEDIATE;
   reg.size = 8;
                        }
      ImmediateValue::ImmediateValue(const ImmediateValue *proto, DataType ty)
   {
               reg.type = ty;
      }
      ImmediateValue *
   ImmediateValue::clone(ClonePolicy<Function>& pol) const
   {
      Program *prog = pol.context()->getProgram();
                     that->reg.size = this->reg.size;
   that->reg.type = this->reg.type;
               }
      bool
   ImmediateValue::isInteger(const int i) const
   {
      switch (reg.type) {
   case TYPE_S8:
         case TYPE_U8:
         case TYPE_S16:
         case TYPE_U16:
         case TYPE_S32:
   case TYPE_U32:
         case TYPE_S64:
   case TYPE_U64:
         case TYPE_F32:
         case TYPE_F64:
         default:
            }
      bool
   ImmediateValue::isNegative() const
   {
      switch (reg.type) {
   case TYPE_S8:  return reg.data.s8 < 0;
   case TYPE_S16: return reg.data.s16 < 0;
   case TYPE_S32:
   case TYPE_U32: return reg.data.s32 < 0;
   case TYPE_F32: return reg.data.u32 & (1 << 31);
   case TYPE_F64: return reg.data.u64 & (1ULL << 63);
   default:
            }
      bool
   ImmediateValue::isPow2() const
   {
      if (reg.type == TYPE_U64 || reg.type == TYPE_S64)
         else
      }
      void
   ImmediateValue::applyLog2()
   {
      switch (reg.type) {
   case TYPE_S8:
   case TYPE_S16:
   case TYPE_S32:
      assert(!this->isNegative());
      case TYPE_U8:
   case TYPE_U16:
   case TYPE_U32:
      reg.data.u32 = util_logbase2(reg.data.u32);
      case TYPE_S64:
      assert(!this->isNegative());
      case TYPE_U64:
      reg.data.u64 = util_logbase2_64(reg.data.u64);
      case TYPE_F32:
      reg.data.f32 = log2f(reg.data.f32);
      case TYPE_F64:
      reg.data.f64 = log2(reg.data.f64);
      default:
      assert(0);
         }
      bool
   ImmediateValue::compare(CondCode cc, float fval) const
   {
      if (reg.type != TYPE_F32)
            switch (static_cast<CondCode>(cc & 7)) {
   case CC_TR: return true;
   case CC_FL: return false;
   case CC_LT: return reg.data.f32 <  fval;
   case CC_LE: return reg.data.f32 <= fval;
   case CC_GT: return reg.data.f32 >  fval;
   case CC_GE: return reg.data.f32 >= fval;
   case CC_EQ: return reg.data.f32 == fval;
   case CC_NE: return reg.data.f32 != fval;
   default:
      assert(0);
         }
      ImmediateValue&
   ImmediateValue::operator=(const ImmediateValue &that)
   {
      this->reg = that.reg;
      }
      bool
   Value::interfers(const Value *that) const
   {
               if (that->reg.file != reg.file || that->reg.fileIndex != reg.fileIndex)
         if (this->asImm())
            if (this->asSym()) {
      idA = this->join->reg.data.offset;
      } else {
      idA = this->join->reg.data.id * MIN2(this->reg.size, 4);
               if (idA < idB)
         else
   if (idA > idB)
         else
      }
      bool
   Value::equals(const Value *that, bool strict) const
   {
      if (strict)
            if (that->reg.file != reg.file || that->reg.fileIndex != reg.fileIndex)
         if (that->reg.size != this->reg.size)
            if (that->reg.data.id != this->reg.data.id)
               }
      bool
   ImmediateValue::equals(const Value *that, bool strict) const
   {
      const ImmediateValue *imm = that->asImm();
   if (!imm)
            }
      bool
   Symbol::equals(const Value *that, bool strict) const
   {
      if (reg.file != that->reg.file || reg.fileIndex != that->reg.fileIndex)
                  if (this->baseSym != that->asSym()->baseSym)
            if (reg.file == FILE_SYSTEM_VALUE)
      return (this->reg.data.sv.sv    == that->reg.data.sv.sv &&
         }
      void Instruction::init()
   {
      next = prev = 0;
            cc = CC_ALWAYS;
   rnd = ROUND_N;
   cache = CACHE_CA;
            saturate = 0;
   join = 0;
   exit = 0;
   terminator = 0;
   ftz = 0;
   dnz = 0;
   perPatch = 0;
   fixed = 0;
   encSize = 0;
   ipa = 0;
   mask = 0;
                              predSrc = -1;
   flagsDef = -1;
            sched = 0;
      }
      Instruction::Instruction()
   {
               op = OP_NOP;
               }
      Instruction::Instruction(Function *fn, operation opr, DataType ty)
   {
               op = opr;
               }
      Instruction::~Instruction()
   {
      if (bb) {
      Function *fn = bb->getFunction();
   bb->remove(this);
               for (int s = 0; srcExists(s); ++s)
         // must unlink defs too since the list pointers will get deallocated
   for (int d = 0; defExists(d); ++d)
      }
      void
   Instruction::setDef(int i, Value *val)
   {
      int size = defs.size();
   if (i >= size) {
      defs.resize(i + 1);
   while (size <= i)
      }
      }
      void
   Instruction::setSrc(int s, Value *val)
   {
      int size = srcs.size();
   if (s >= size) {
      srcs.resize(s + 1);
   while (size <= s)
      }
      }
      void
   Instruction::setSrc(int s, const ValueRef& ref)
   {
      setSrc(s, ref.get());
      }
      void
   Instruction::swapSources(int a, int b)
   {
      Value *value = srcs[a].get();
                     srcs[b].set(value);
      }
      static inline void moveSourcesAdjustIndex(int8_t &index, int s, int delta)
   {
      if (index >= s)
         else
   if ((delta < 0) && (index >= (s + delta)))
      }
      // Moves sources [@s,last_source] by @delta.
   // If @delta < 0, sources [@s - abs(@delta), @s) are erased.
   void
   Instruction::moveSources(const int s, const int delta)
   {
      if (delta == 0)
                           for (k = 0; srcExists(k); ++k) {
      for (int i = 0; i < 2; ++i)
      }
   moveSourcesAdjustIndex(predSrc, s, delta);
   moveSourcesAdjustIndex(flagsSrc, s, delta);
   if (asTex()) {
      TexInstruction *tex = asTex();
   moveSourcesAdjustIndex(tex->tex.rIndirectSrc, s, delta);
               if (delta > 0) {
      --k;
   for (int p = k + delta; k >= s; --k, --p)
      } else {
      int p;
   for (p = s; p < k; ++p)
         for (; (p + delta) < k; ++p)
         }
      void
   Instruction::takeExtraSources(int s, Value *values[3])
   {
      values[0] = getIndirect(s, 0);
   if (values[0])
            values[1] = getIndirect(s, 1);
   if (values[1])
            values[2] = getPredicate();
   if (values[2])
      }
      void
   Instruction::putExtraSources(int s, Value *values[3])
   {
      if (values[0])
         if (values[1])
         if (values[2])
      }
      Instruction *
   Instruction::clone(ClonePolicy<Function>& pol, Instruction *i) const
   {
      if (!i)
      #if !defined(NDEBUG) && defined(__cpp_rtti)
         #endif
                           i->rnd = rnd;
   i->cache = cache;
            i->saturate = saturate;
   i->join = join;
   i->exit = exit;
   i->mask = mask;
   i->ftz = ftz;
   i->dnz = dnz;
   i->ipa = ipa;
   i->lanes = lanes;
                     for (int d = 0; defExists(d); ++d)
            for (int s = 0; srcExists(s); ++s) {
      i->setSrc(s, pol.get(getSrc(s)));
               i->cc = cc;
   i->predSrc = predSrc;
   i->flagsDef = flagsDef;
               }
      unsigned int
   Instruction::defCount(unsigned int mask, bool singleFile) const
   {
               if (singleFile) {
      unsigned int d = ffs(mask);
   if (!d)
         for (i = d--; defExists(i); ++i)
      if (getDef(i)->reg.file != getDef(d)->reg.file)
            for (n = 0, i = 0; this->defExists(i); ++i, mask >>= 1)
            }
      unsigned int
   Instruction::srcCount(unsigned int mask, bool singleFile) const
   {
               if (singleFile) {
      unsigned int s = ffs(mask);
   if (!s)
         for (i = s--; srcExists(i); ++i)
      if (getSrc(i)->reg.file != getSrc(s)->reg.file)
            for (n = 0, i = 0; this->srcExists(i); ++i, mask >>= 1)
            }
      bool
   Instruction::setIndirect(int s, int dim, Value *value)
   {
               int p = srcs[s].indirect[dim];
   if (p < 0) {
      if (!value)
         p = srcs.size();
   while (p > 0 && !srcExists(p - 1))
      }
   setSrc(p, value);
   srcs[p].usedAsPtr = (value != 0);
   srcs[s].indirect[dim] = value ? p : -1;
      }
      bool
   Instruction::setPredicate(CondCode ccode, Value *value)
   {
               if (!value) {
      if (predSrc >= 0) {
      srcs[predSrc].set(NULL);
      }
               if (predSrc < 0) {
      predSrc = srcs.size();
   while (predSrc > 0 && !srcExists(predSrc - 1))
               setSrc(predSrc, value);
      }
      bool
   Instruction::writesPredicate() const
   {
      for (int d = 0; defExists(d); ++d)
      if (getDef(d)->inFile(FILE_PREDICATE) || getDef(d)->inFile(FILE_FLAGS))
         }
      bool
   Instruction::canCommuteDefSrc(const Instruction *i) const
   {
      for (int d = 0; defExists(d); ++d)
      for (int s = 0; i->srcExists(s); ++s)
      if (getDef(d)->interfers(i->getSrc(s)))
      }
      bool
   Instruction::canCommuteDefDef(const Instruction *i) const
   {
      for (int d = 0; defExists(d); ++d)
      for (int c = 0; i->defExists(c); ++c)
      if (getDef(d)->interfers(i->getDef(c)))
      }
      bool
   Instruction::isCommutationLegal(const Instruction *i) const
   {
      return canCommuteDefDef(i) &&
      canCommuteDefSrc(i) &&
   }
      TexInstruction::TexInstruction(Function *fn, operation op)
         {
      tex.rIndirectSrc = -1;
            if (op == OP_TXF)
      }
      TexInstruction::~TexInstruction()
   {
      for (int c = 0; c < 3; ++c) {
      dPdx[c].set(NULL);
      }
   for (int n = 0; n < 4; ++n)
      for (int c = 0; c < 3; ++c)
   }
      TexInstruction *
   TexInstruction::clone(ClonePolicy<Function>& pol, Instruction *i) const
   {
      TexInstruction *tex = (i ? static_cast<TexInstruction *>(i) :
                              if (op == OP_TXD) {
      for (unsigned int c = 0; c < tex->tex.target.getDim(); ++c) {
      tex->dPdx[c].set(dPdx[c]);
                  for (int n = 0; n < tex->tex.useOffsets; ++n)
      for (int c = 0; c < 3; ++c)
            }
      const struct TexInstruction::Target::Desc TexInstruction::Target::descTable[] =
   {
      { "1D",                1, 1, false, false, false },
   { "2D",                2, 2, false, false, false },
   { "2D_MS",             2, 3, false, false, false },
   { "3D",                3, 3, false, false, false },
   { "CUBE",              2, 3, false, true,  false },
   { "1D_SHADOW",         1, 1, false, false, true  },
   { "2D_SHADOW",         2, 2, false, false, true  },
   { "CUBE_SHADOW",       2, 3, false, true,  true  },
   { "1D_ARRAY",          1, 2, true,  false, false },
   { "2D_ARRAY",          2, 3, true,  false, false },
   { "2D_MS_ARRAY",       2, 4, true,  false, false },
   { "CUBE_ARRAY",        2, 4, true,  true,  false },
   { "1D_ARRAY_SHADOW",   1, 2, true,  false, true  },
   { "2D_ARRAY_SHADOW",   2, 3, true,  false, true  },
   { "RECT",              2, 2, false, false, false },
   { "RECT_SHADOW",       2, 2, false, false, true  },
   { "CUBE_ARRAY_SHADOW", 2, 4, true,  true,  true  },
      };
      const struct TexInstruction::ImgFormatDesc TexInstruction::formatTable[] =
   {
      { "RGBA32F",      4, { 32, 32, 32, 32 }, FLOAT },
   { "RGBA16F",      4, { 16, 16, 16, 16 }, FLOAT },
   { "RG32F",        2, { 32, 32,  0,  0 }, FLOAT },
   { "RG16F",        2, { 16, 16,  0,  0 }, FLOAT },
   { "R11G11B10F",   3, { 11, 11, 10,  0 }, FLOAT },
   { "R32F",         1, { 32,  0,  0,  0 }, FLOAT },
            { "RGBA32UI",     4, { 32, 32, 32, 32 },  UINT },
   { "RGBA16UI",     4, { 16, 16, 16, 16 },  UINT },
   { "RGB10A2UI",    4, { 10, 10, 10,  2 },  UINT },
   { "RGBA8UI",      4, {  8,  8,  8,  8 },  UINT },
   { "RG32UI",       2, { 32, 32,  0,  0 },  UINT },
   { "RG16UI",       2, { 16, 16,  0,  0 },  UINT },
   { "RG8UI",        2, {  8,  8,  0,  0 },  UINT },
   { "R32UI",        1, { 32,  0,  0,  0 },  UINT },
   { "R16UI",        1, { 16,  0,  0,  0 },  UINT },
            { "RGBA32I",      4, { 32, 32, 32, 32 },  SINT },
   { "RGBA16I",      4, { 16, 16, 16, 16 },  SINT },
   { "RGBA8I",       4, {  8,  8,  8,  8 },  SINT },
   { "RG32I",        2, { 32, 32,  0,  0 },  SINT },
   { "RG16I",        2, { 16, 16,  0,  0 },  SINT },
   { "RG8I",         2, {  8,  8,  0,  0 },  SINT },
   { "R32I",         1, { 32,  0,  0,  0 },  SINT },
   { "R16I",         1, { 16,  0,  0,  0 },  SINT },
            { "RGBA16",       4, { 16, 16, 16, 16 }, UNORM },
   { "RGB10A2",      4, { 10, 10, 10,  2 }, UNORM },
   { "RGBA8",        4, {  8,  8,  8,  8 }, UNORM },
   { "RG16",         2, { 16, 16,  0,  0 }, UNORM },
   { "RG8",          2, {  8,  8,  0,  0 }, UNORM },
   { "R16",          1, { 16,  0,  0,  0 }, UNORM },
            { "RGBA16_SNORM", 4, { 16, 16, 16, 16 }, SNORM },
   { "RGBA8_SNORM",  4, {  8,  8,  8,  8 }, SNORM },
   { "RG16_SNORM",   2, { 16, 16,  0,  0 }, SNORM },
   { "RG8_SNORM",    2, {  8,  8,  0,  0 }, SNORM },
   { "R16_SNORM",    1, { 16,  0,  0,  0 }, SNORM },
               };
      const struct TexInstruction::ImgFormatDesc *
   TexInstruction::translateImgFormat(enum pipe_format format)
   {
      #define FMT_CASE(a, b) \
   case PIPE_FORMAT_ ## a: return &formatTable[nv50_ir::FMT_ ## b]
         switch (format) {
            FMT_CASE(R32G32B32A32_FLOAT, RGBA32F);
   FMT_CASE(R16G16B16A16_FLOAT, RGBA16F);
   FMT_CASE(R32G32_FLOAT, RG32F);
   FMT_CASE(R16G16_FLOAT, RG16F);
   FMT_CASE(R11G11B10_FLOAT, R11G11B10F);
   FMT_CASE(R32_FLOAT, R32F);
            FMT_CASE(R32G32B32A32_UINT, RGBA32UI);
   FMT_CASE(R16G16B16A16_UINT, RGBA16UI);
   FMT_CASE(R10G10B10A2_UINT, RGB10A2UI);
   FMT_CASE(R8G8B8A8_UINT, RGBA8UI);
   FMT_CASE(R32G32_UINT, RG32UI);
   FMT_CASE(R16G16_UINT, RG16UI);
   FMT_CASE(R8G8_UINT, RG8UI);
   FMT_CASE(R32_UINT, R32UI);
   FMT_CASE(R16_UINT, R16UI);
            FMT_CASE(R32G32B32A32_SINT, RGBA32I);
   FMT_CASE(R16G16B16A16_SINT, RGBA16I);
   FMT_CASE(R8G8B8A8_SINT, RGBA8I);
   FMT_CASE(R32G32_SINT, RG32I);
   FMT_CASE(R16G16_SINT, RG16I);
   FMT_CASE(R8G8_SINT, RG8I);
   FMT_CASE(R32_SINT, R32I);
   FMT_CASE(R16_SINT, R16I);
            FMT_CASE(R16G16B16A16_UNORM, RGBA16);
   FMT_CASE(R10G10B10A2_UNORM, RGB10A2);
   FMT_CASE(R8G8B8A8_UNORM, RGBA8);
   FMT_CASE(R16G16_UNORM, RG16);
   FMT_CASE(R8G8_UNORM, RG8);
   FMT_CASE(R16_UNORM, R16);
            FMT_CASE(R16G16B16A16_SNORM, RGBA16_SNORM);
   FMT_CASE(R8G8B8A8_SNORM, RGBA8_SNORM);
   FMT_CASE(R16G16_SNORM, RG16_SNORM);
   FMT_CASE(R8G8_SNORM, RG8_SNORM);
   FMT_CASE(R16_SNORM, R16_SNORM);
                     default:
      assert(!"Unexpected format");
         }
      void
   TexInstruction::setIndirectR(Value *v)
   {
      int p = ((tex.rIndirectSrc < 0) && v) ? srcs.size() : tex.rIndirectSrc;
   if (p >= 0) {
      tex.rIndirectSrc = p;
   setSrc(p, v);
         }
      void
   TexInstruction::setIndirectS(Value *v)
   {
      int p = ((tex.sIndirectSrc < 0) && v) ? srcs.size() : tex.sIndirectSrc;
   if (p >= 0) {
      tex.sIndirectSrc = p;
   setSrc(p, v);
         }
      CmpInstruction::CmpInstruction(Function *fn, operation op)
         {
         }
      CmpInstruction *
   CmpInstruction::clone(ClonePolicy<Function>& pol, Instruction *i) const
   {
      CmpInstruction *cmp = (i ? static_cast<CmpInstruction *>(i) :
         cmp->dType = dType;
   Instruction::clone(pol, cmp);
   cmp->setCond = setCond;
      }
      FlowInstruction::FlowInstruction(Function *fn, operation op, void *targ)
         {
      if (op == OP_CALL)
         else
            if (op == OP_BRA ||
      op == OP_CONT || op == OP_BREAK ||
   op == OP_RET || op == OP_EXIT)
      else
   if (op == OP_JOIN)
               }
      FlowInstruction *
   FlowInstruction::clone(ClonePolicy<Function>& pol, Instruction *i) const
   {
      FlowInstruction *flow = (i ? static_cast<FlowInstruction *>(i) :
            Instruction::clone(pol, flow);
   flow->allWarp = allWarp;
   flow->absolute = absolute;
   flow->limit = limit;
            if (builtin)
         else
   if (op == OP_CALL)
         else
   if (target.bb)
               }
      Program::Program(Type type, Target *arch)
      : progType(type),
   target(arch),
   tlsSize(0),
   mem_Instruction(sizeof(Instruction), 6),
   mem_CmpInstruction(sizeof(CmpInstruction), 4),
   mem_TexInstruction(sizeof(TexInstruction), 4),
   mem_FlowInstruction(sizeof(FlowInstruction), 4),
   mem_LValue(sizeof(LValue), 8),
   mem_Symbol(sizeof(Symbol), 7),
   mem_ImmediateValue(sizeof(ImmediateValue), 7),
   driver(NULL),
      {
      code = NULL;
            maxGPR = -1;
   fp64 = false;
            main = new Function(this, "MAIN", ~0);
            dbgFlags = 0;
               }
      Program::~Program()
   {
      for (ArrayList::Iterator it = allFuncs.iterator(); !it.end(); it.next())
            for (ArrayList::Iterator it = allRValues.iterator(); !it.end(); it.next())
      }
      void Program::releaseInstruction(Instruction *insn)
   {
                        if (insn->asCmp())
         else
   if (insn->asTex())
         else
   if (insn->asFlow())
         else
      }
      void Program::releaseValue(Value *value)
   {
               if (value->asLValue())
         else
   if (value->asImm())
         else
   if (value->asSym())
      }
         } // namespace nv50_ir
      extern "C" {
      static void
   nv50_ir_init_prog_info(struct nv50_ir_prog_info *info,
         {
      info_out->target = info->target;
   info_out->type = info->type;
   if (info->type == PIPE_SHADER_TESS_CTRL || info->type == PIPE_SHADER_TESS_EVAL) {
      info_out->prop.tp.domain = MESA_PRIM_COUNT;
      }
   if (info->type == PIPE_SHADER_GEOMETRY) {
      info_out->prop.gp.instanceCount = 1;
      }
   if (info->type == PIPE_SHADER_COMPUTE) {
      info->prop.cp.numThreads[0] =
   info->prop.cp.numThreads[1] =
      }
   info_out->bin.smemSize = info->bin.smemSize;
   info_out->io.instanceId = 0xff;
   info_out->io.vertexId = 0xff;
   info_out->io.edgeFlagIn = 0xff;
   info_out->io.edgeFlagOut = 0xff;
   info_out->io.fragDepth = 0xff;
      }
      int
   nv50_ir_generate_code(struct nv50_ir_prog_info *info,
         {
                              #define PROG_TYPE_CASE(a, b)                                      \
               switch (info->type) {
   PROG_TYPE_CASE(VERTEX, VERTEX);
   PROG_TYPE_CASE(TESS_CTRL, TESSELLATION_CONTROL);
   PROG_TYPE_CASE(TESS_EVAL, TESSELLATION_EVAL);
   PROG_TYPE_CASE(GEOMETRY, GEOMETRY);
   PROG_TYPE_CASE(FRAGMENT, FRAGMENT);
   PROG_TYPE_CASE(COMPUTE, COMPUTE);
   default:
      INFO_DBG(info->dbgFlags, VERBOSE, "unsupported program type %u\n", info->type);
      }
            nv50_ir::Target *targ = nv50_ir::Target::create(info->target);
   if (!targ)
            nv50_ir::Program *prog = new nv50_ir::Program(type, targ);
   if (!prog) {
      nv50_ir::Target::destroy(targ);
      }
   prog->driver = info;
   prog->driver_out = info_out;
   prog->dbgFlags = info->dbgFlags;
            ret = prog->makeFromNIR(info, info_out) ? 0 : -2;
   if (ret < 0)
         if (prog->dbgFlags & NV50_IR_DEBUG_VERBOSE)
            targ->parseDriverInfo(info, info_out);
                     if (prog->dbgFlags & NV50_IR_DEBUG_VERBOSE)
            prog->optimizeSSA(info->optLevel);
            if (prog->dbgFlags & NV50_IR_DEBUG_BASIC)
            if (!prog->registerAllocation()) {
      ret = -4;
      }
                     if (!prog->emitBinary(info_out)) {
      ret = -5;
            out:
               info_out->bin.maxGPR = prog->maxGPR;
   info_out->bin.code = prog->code;
   info_out->bin.codeSize = prog->binSize;
            delete prog;
               }
      } // extern "C"
