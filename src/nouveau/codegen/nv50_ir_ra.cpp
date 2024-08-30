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
      #include <algorithm>
   #include <stack>
   #include <limits>
   #include <unordered_map>
      namespace nv50_ir {
   namespace {
      #define MAX_REGISTER_FILE_SIZE 256
      class RegisterSet
   {
   public:
               void init(const Target *);
            bool assign(int32_t& reg, DataFile f, unsigned int size, unsigned int maxReg);
   void occupy(DataFile f, int32_t reg, unsigned int size);
   void occupyMask(DataFile f, int32_t reg, uint8_t mask);
   bool isOccupied(DataFile f, int32_t reg, unsigned int size) const;
                     inline unsigned int getFileSize(DataFile f) const
   {
                  inline unsigned int units(DataFile f, unsigned int size) const
   {
         }
   // for regs of size >= 4, id is counted in 4-byte words (like nv50/c0 binary)
   inline unsigned int idToBytes(const Value *v) const
   {
         }
   inline unsigned int idToUnits(const Value *v) const
   {
         }
   inline int bytesToId(Value *v, unsigned int bytes) const
   {
      if (v->reg.size < 4)
            }
   inline int unitsToId(DataFile f, int u, uint8_t size) const
   {
      if (u < 0)
                                    private:
                        int last[LAST_REGISTER_FILE + 1];
      };
      void
   RegisterSet::reset(DataFile f, bool resetMax)
   {
      bits[f].fill(0);
   if (resetMax)
      }
      void
   RegisterSet::init(const Target *targ)
   {
      for (unsigned int rf = 0; rf <= LAST_REGISTER_FILE; ++rf) {
      DataFile f = static_cast<DataFile>(rf);
   last[rf] = targ->getFileSize(f) - 1;
   unit[rf] = targ->getFileUnit(f);
   fill[rf] = -1;
   assert(last[rf] < MAX_REGISTER_FILE_SIZE);
         }
      RegisterSet::RegisterSet(const Target *targ)
   : restrictedGPR16Range(targ->getChipset() < 0xc0)
   {
      init(targ);
   for (unsigned int i = 0; i <= LAST_REGISTER_FILE; ++i)
      }
      void
   RegisterSet::print(DataFile f) const
   {
      INFO("GPR:");
   bits[f].print();
      }
      bool
   RegisterSet::assign(int32_t& reg, DataFile f, unsigned int size, unsigned int maxReg)
   {
      reg = bits[f].findFreeRange(size, maxReg);
   if (reg < 0)
         fill[f] = MAX2(fill[f], (int32_t)(reg + size - 1));
      }
      bool
   RegisterSet::isOccupied(DataFile f, int32_t reg, unsigned int size) const
   {
         }
      void
   RegisterSet::occupyMask(DataFile f, int32_t reg, uint8_t mask)
   {
         }
      void
   RegisterSet::occupy(DataFile f, int32_t reg, unsigned int size)
   {
                           }
      bool
   RegisterSet::testOccupy(DataFile f, int32_t reg, unsigned int size)
   {
      if (isOccupied(f, reg, size))
         occupy(f, reg, size);
      }
      class RegAlloc
   {
   public:
               bool exec();
         private:
      class PhiMovesPass : public Pass {
   private:
      virtual bool visit(BasicBlock *);
   inline bool needNewElseBlock(BasicBlock *b, BasicBlock *p);
               class BuildIntervalsPass : public Pass {
   private:
      virtual bool visit(BasicBlock *);
   void collectLiveValues(BasicBlock *);
               class InsertConstraintsPass : public Pass {
   public:
      InsertConstraintsPass() : targ(NULL) { }
      private:
               void insertConstraintMove(Instruction *, int s);
            void condenseDefs(Instruction *);
   void condenseDefs(Instruction *, const int first, const int last);
            void addHazard(Instruction *i, const ValueRef *src);
            // target specific functions, TODO: put in subclass or Target
   void texConstraintNV50(TexInstruction *);
   void texConstraintNVC0(TexInstruction *);
   void texConstraintNVE0(TexInstruction *);
            bool isScalarTexGM107(TexInstruction *);
                                       private:
      Program *prog;
            // instructions in control flow / chronological order
               };
      typedef std::pair<Value *, Value *> ValuePair;
      class MergedDefs
   {
   private:
      std::list<ValueDef *>& entry(Value *val) {
               if (it == defs.end()) {
      std::list<ValueDef *> &res = defs[val];
   res = val->defs;
      } else {
                           public:
      std::list<ValueDef *>& operator()(Value *val) {
                  void add(Value *val, const std::list<ValueDef *> &vals) {
      assert(val);
   std::list<ValueDef *> &valdefs = entry(val);
               void removeDefsOfInstruction(Instruction *insn) {
      for (int d = 0; insn->defExists(d); ++d) {
      ValueDef *def = &insn->def(d);
   defs.erase(def->get());
   for (auto &p : defs)
                  void merge() {
      for (auto &p : defs)
         };
      class SpillCodeInserter
   {
   public:
                        Symbol *assignSlot(const Interval&, const unsigned int size);
   Value *offsetSlot(Value *, const LValue *);
         private:
      Function *func;
            int32_t stackSize;
            LValue *unspill(Instruction *usei, LValue *, Value *slot);
      };
      void
   RegAlloc::BuildIntervalsPass::addLiveRange(Value *val,
               {
               if (!insn)
            assert(bb->getFirst()->serial <= bb->getExit()->serial);
            int begin = insn->serial;
   if (begin < bb->getEntry()->serial || begin > bb->getExit()->serial)
            INFO_DBG(prog->dbgFlags, REG_ALLOC, "%%%i <- live range [%i(%i), %i)\n",
            if (begin != end) // empty ranges are only added as hazards for fixed regs
      }
      bool
   RegAlloc::PhiMovesPass::needNewElseBlock(BasicBlock *b, BasicBlock *p)
   {
      if (b->cfg.incidentCount() <= 1)
            int n = 0;
   for (Graph::EdgeIterator ei = p->cfg.outgoing(); !ei.end(); ei.next())
      if (ei.getType() == Graph::Edge::TREE ||
      ei.getType() == Graph::Edge::FORWARD)
      }
      struct PhiMapHash {
      size_t operator()(const std::pair<Instruction *, BasicBlock *>& val) const {
      return std::hash<Instruction*>()(val.first) * 31 +
         };
      typedef std::unordered_map<
            // Critical edges need to be split up so that work can be inserted along
   // specific edge transitions. Unfortunately manipulating incident edges into a
   // BB invalidates all the PHI nodes since their sources are implicitly ordered
   // by incident edge order.
   //
   // TODO: Make it so that that is not the case, and PHI nodes store pointers to
   // the original BBs.
   void
   RegAlloc::PhiMovesPass::splitEdges(BasicBlock *bb)
   {
      BasicBlock *pb, *pn;
   Instruction *phi;
   Graph::EdgeIterator ei;
   std::stack<BasicBlock *> stack;
            for (ei = bb->cfg.incident(); !ei.end(); ei.next()) {
      pb = BasicBlock::get(ei.getNode());
   assert(pb);
   if (needNewElseBlock(bb, pb))
               // No critical edges were found, no need to perform any work.
   if (stack.empty())
            // We're about to, potentially, reorder the inbound edges. This means that
   // we need to hold on to the (phi, bb) -> src mapping, and fix up the phi
   // nodes after the graph has been modified.
            j = 0;
   for (ei = bb->cfg.incident(); !ei.end(); ei.next(), j++) {
      pb = BasicBlock::get(ei.getNode());
   for (phi = bb->getPhi(); phi && phi->op == OP_PHI; phi = phi->next)
               while (!stack.empty()) {
      pb = stack.top();
   pn = new BasicBlock(func);
            pb->cfg.detach(&bb->cfg);
   pb->cfg.attach(&pn->cfg, Graph::Edge::TREE);
            assert(pb->getExit()->op != OP_CALL);
   if (pb->getExit()->asFlow()->target.bb == bb)
            for (phi = bb->getPhi(); phi && phi->op == OP_PHI; phi = phi->next) {
      PhiMap::iterator it = phis.find(std::make_pair(phi, pb));
   assert(it != phis.end());
   phis.insert(std::make_pair(std::make_pair(phi, pn), it->second));
                  // Now go through and fix up all of the phi node sources.
   j = 0;
   for (ei = bb->cfg.incident(); !ei.end(); ei.next(), j++) {
      pb = BasicBlock::get(ei.getNode());
   for (phi = bb->getPhi(); phi && phi->op == OP_PHI; phi = phi->next) {
                              }
      // For each operand of each PHI in b, generate a new value by inserting a MOV
   // at the end of the block it is coming from and replace the operand with its
   // result. This eliminates liveness conflicts and enables us to let values be
   // copied to the right register if such a conflict exists nonetheless.
   //
   // These MOVs are also crucial in making sure the live intervals of phi srces
   // are extended until the end of the loop, since they are not included in the
   // live-in sets.
   bool
   RegAlloc::PhiMovesPass::visit(BasicBlock *bb)
   {
                        // insert MOVs (phi->src(j) should stem from j-th in-BB)
   int j = 0;
   for (Graph::EdgeIterator ei = bb->cfg.incident(); !ei.end(); ei.next()) {
      BasicBlock *pb = BasicBlock::get(ei.getNode());
   if (!pb->isTerminated())
            for (phi = bb->getPhi(); phi && phi->op == OP_PHI; phi = phi->next) {
                     mov->setSrc(0, phi->getSrc(j));
                     }
                  }
      // Build the set of live-in variables of bb.
   bool
   RegAlloc::buildLiveSets(BasicBlock *bb)
   {
      Function *f = bb->getFunction();
   BasicBlock *bn;
   Instruction *i;
                              int n = 0;
   for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
      bn = BasicBlock::get(ei.getNode());
   if (bn == bb)
         if (bn->cfg.visit(sequence))
      if (!buildLiveSets(bn))
      if (n++ || bb->liveSet.marker)
         else
      }
   if (!n && !bb->liveSet.marker)
                  if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC) {
      INFO("BB:%i live set of out blocks:\n", bb->getId());
               // if (!bb->getEntry())
            if (bb == BasicBlock::get(f->cfgExit)) {
      for (std::deque<ValueRef>::iterator it = f->outs.begin();
      it != f->outs.end(); ++it) {
   assert(it->get()->asLValue());
                  for (i = bb->getExit(); i && i != bb->getEntry()->prev; i = i->prev) {
      for (d = 0; i->defExists(d); ++d)
         for (s = 0; i->srcExists(s); ++s)
      if (i->getSrc(s)->asLValue())
   }
   for (i = bb->getPhi(); i && i->op == OP_PHI; i = i->next)
            if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC) {
      INFO("BB:%i live set after propagation:\n", bb->getId());
                  }
      void
   RegAlloc::BuildIntervalsPass::collectLiveValues(BasicBlock *bb)
   {
               if (bb->cfg.outgoingCount()) {
      // trickery to save a loop of OR'ing liveSets
   // aliasing works fine with BitSet::setOr
   for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
      if (bbA) {
      bb->liveSet.setOr(&bbA->liveSet, &bbB->liveSet);
      } else {
         }
      }
      } else
   if (bb->cfg.incidentCount()) {
            }
      bool
   RegAlloc::BuildIntervalsPass::visit(BasicBlock *bb)
   {
                        // go through out blocks and delete phi sources that do not originate from
   // the current block from the live set
   for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
               for (Instruction *i = out->getPhi(); i && i->op == OP_PHI; i = i->next) {
               for (int s = 0; i->srcExists(s); ++s) {
      assert(i->src(s).getInsn());
   if (i->getSrc(s)->getUniqueInsn()->bb == bb) // XXX: reachableBy ?
         else
                     // remaining live-outs are live until end
   if (bb->getExit()) {
      for (unsigned int j = 0; j < bb->liveSet.getSize(); ++j)
      if (bb->liveSet.test(j))
            for (Instruction *i = bb->getExit(); i && i->op != OP_PHI; i = i->prev) {
      for (int d = 0; i->defExists(d); ++d) {
      bb->liveSet.clr(i->getDef(d)->id);
   if (i->getDef(d)->reg.data.id >= 0) // add hazard for fixed regs
               for (int s = 0; i->srcExists(s); ++s) {
      if (!i->getSrc(s)->asLValue())
         if (!bb->liveSet.test(i->getSrc(s)->id)) {
      bb->liveSet.set(i->getSrc(s)->id);
                     if (bb == BasicBlock::get(func->cfg.getRoot())) {
      for (std::deque<ValueDef>::iterator it = func->ins.begin();
      it != func->ins.end(); ++it) {
   if (it->get()->reg.data.id >= 0) // add hazard for fixed regs
                     }
         #define JOIN_MASK_PHI        (1 << 0)
   #define JOIN_MASK_UNION      (1 << 1)
   #define JOIN_MASK_MOV        (1 << 2)
   #define JOIN_MASK_TEX        (1 << 3)
      class GCRA
   {
   public:
      GCRA(Function *, SpillCodeInserter&, MergedDefs&);
                           private:
      class RIG_Node : public Graph::Node
   {
   public:
                        void addInterference(RIG_Node *);
            inline LValue *getValue() const
   {
         }
            inline uint8_t getCompMask() const
   {
                  static inline RIG_Node *get(const Graph::EdgeIterator& ei)
   {
               public:
      uint32_t degree;
   uint16_t degreeLimit; // if deg < degLimit, node is trivially colourable
   uint16_t maxReg;
            DataFile f;
                     // list pointers for simplify() phase
   RIG_Node *next;
            // union of the live intervals of all coalesced values (we want to retain
   //  the separate intervals for testing interference of compound values)
                     private:
               void buildRIG(ArrayList&);
   bool coalesce(ArrayList&);
   bool doCoalesce(ArrayList&, unsigned int mask);
   void calculateSpillWeights();
   bool simplify();
   bool selectRegisters();
            void simplifyEdge(RIG_Node *, RIG_Node *);
            void copyCompound(Value *dst, Value *src);
   bool coalesceValues(Value *, Value *, bool force);
   void resolveSplitsAndMerges();
                     inline void insertOrderedTail(std::list<RIG_Node *>&, RIG_Node *);
         private:
               // list headers for simplify() phase
   RIG_Node lo[2];
            Graph RIG;
   RIG_Node *nodes;
            Function *func;
            struct RelDegree {
               RelDegree() {
      for (int i = 1; i <= 16; ++i)
      for (int j = 1; j <= 16; ++j)
            const uint8_t* operator[](std::size_t i) const {
                                       // need to fixup register id for participants of OP_MERGE/SPLIT
   std::list<Instruction *> merges;
            SpillCodeInserter& spill;
               };
      const GCRA::RelDegree GCRA::relDegree;
      GCRA::RIG_Node::RIG_Node() : Node(NULL), degree(0), degreeLimit(0), maxReg(0),
         {
   }
      void
   GCRA::printNodeInfo() const
   {
      for (unsigned int i = 0; i < nodeCount; ++i) {
      if (!nodes[i].colors)
         INFO("RIG_Node[%%%i]($[%u]%i): %u colors, weight %f, deg %u/%u\n X",
      i,
   nodes[i].f,nodes[i].reg,nodes[i].colors,
               for (Graph::EdgeIterator ei = nodes[i].outgoing(); !ei.end(); ei.next())
         for (Graph::EdgeIterator ei = nodes[i].incident(); !ei.end(); ei.next())
               }
      static bool
   isShortRegOp(Instruction *insn)
   {
      // Immediates are always in src1 (except zeroes, which end up getting
   // replaced with a zero reg). Every other situation can be resolved by
   // using a long encoding.
   return insn->srcExists(1) && insn->src(1).getFile() == FILE_IMMEDIATE &&
      }
      // Check if this LValue is ever used in an instruction that can't be encoded
   // with long registers (i.e. > r63)
   static bool
   isShortRegVal(LValue *lval)
   {
      if (lval->getInsn() == NULL)
         for (Value::DefCIterator def = lval->defs.begin();
      def != lval->defs.end(); ++def)
   if (isShortRegOp((*def)->getInsn()))
      for (Value::UseCIterator use = lval->uses.begin();
      use != lval->uses.end(); ++use)
   if (isShortRegOp((*use)->getInsn()))
         }
      void
   GCRA::RIG_Node::init(const RegisterSet& regs, LValue *lval)
   {
      setValue(lval);
   if (lval->reg.data.id >= 0)
            colors = regs.units(lval->reg.file, lval->reg.size);
   f = lval->reg.file;
   reg = -1;
   if (lval->reg.data.id >= 0)
            weight = std::numeric_limits<float>::infinity();
   degree = 0;
   maxReg = regs.getFileSize(f);
   // On nv50, we lose a bit of gpr encoding when there's an embedded
   // immediate.
   if (regs.restrictedGPR16Range && f == FILE_GPR && (lval->reg.size == 2 || isShortRegVal(lval)))
         degreeLimit = maxReg;
               }
      // Used when coalescing moves. The non-compound value will become one, e.g.:
   // mov b32 $r0 $r2            / merge b64 $r0d { $r0 $r1 }
   // split b64 { $r0 $r1 } $r0d / mov b64 $r0d f64 $r2d
   void
   GCRA::copyCompound(Value *dst, Value *src)
   {
      LValue *ldst = dst->asLValue();
            if (ldst->compound && !lsrc->compound) {
      LValue *swap = lsrc;
   lsrc = ldst;
                        if (lsrc->compound) {
      for (ValueDef *d : mergedDefs(ldst->join)) {
      LValue *ldst = d->get()->asLValue();
   if (!ldst->compound)
         ldst->compound = 1;
            }
      bool
   GCRA::coalesceValues(Value *dst, Value *src, bool force)
   {
      LValue *rep = dst->join->asLValue();
            if (!force && val->reg.data.id >= 0) {
      rep = src->join->asLValue();
      }
   RIG_Node *nRep = &nodes[rep->id];
            if (src->reg.file != dst->reg.file) {
      if (!force)
            }
   if (!force && dst->reg.size != src->reg.size)
            if ((rep->reg.data.id >= 0) && (rep->reg.data.id != val->reg.data.id)) {
      if (force) {
      if (val->reg.data.id >= 0)
      } else {
      if (val->reg.data.id >= 0)
         // make sure that there is no overlap with the fixed register of rep
   for (ArrayList::Iterator it = func->allLValues.iterator();
      !it.end(); it.next()) {
   Value *reg = reinterpret_cast<Value *>(it.get())->asLValue();
   assert(reg);
   if (reg->interfers(rep) && reg->livei.overlaps(nVal->livei))
                     if (!force && nRep->livei.overlaps(nVal->livei))
            // TODO: Handle this case properly.
   if (!force && rep->compound && val->compound)
            INFO_DBG(prog->dbgFlags, REG_ALLOC, "joining %%%i($%i) <- %%%i\n",
            if (!force)
            // set join pointer of all values joined with val
   const std::list<ValueDef *> &defs = mergedDefs(val);
   for (ValueDef *def : defs)
                  // add val's definitions to rep and extend the live interval of its RIG node
   mergedDefs.add(rep, defs);
   nRep->livei.unify(nVal->livei);
   nRep->degreeLimit = MIN2(nRep->degreeLimit, nVal->degreeLimit);
   nRep->maxReg = MIN2(nRep->maxReg, nVal->maxReg);
      }
      bool
   GCRA::coalesce(ArrayList& insns)
   {
      bool ret = doCoalesce(insns, JOIN_MASK_PHI);
   if (!ret)
         switch (func->getProgram()->getTarget()->getChipset() & ~0xf) {
   case 0x50:
   case 0x80:
   case 0x90:
   case 0xa0:
      ret = doCoalesce(insns, JOIN_MASK_UNION | JOIN_MASK_TEX);
      case 0xc0:
   case 0xd0:
   case 0xe0:
   case 0xf0:
   case 0x100:
   case 0x110:
   case 0x120:
   case 0x130:
   case 0x140:
   case 0x160:
   case 0x170:
   case 0x190:
      ret = doCoalesce(insns, JOIN_MASK_UNION);
      default:
         }
   if (!ret)
            }
      static inline uint8_t makeCompMask(int compSize, int base, int size)
   {
               switch (compSize) {
   case 1:
         case 2:
      m |= (m << 2);
      case 3:
   case 4:
         default:
      assert(compSize <= 8);
         }
      void
   GCRA::makeCompound(Instruction *insn, bool split)
   {
               if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC) {
      INFO("makeCompound(split = %i): ", split);
               const unsigned int size = getNode(rep)->colors;
            if (!rep->compound)
                  for (int c = 0; split ? insn->defExists(c) : insn->srcExists(c); ++c) {
               val->compound = 1;
   if (!val->compMask)
         val->compMask &= makeCompMask(size, base, getNode(val)->colors);
            INFO_DBG(prog->dbgFlags, REG_ALLOC, "compound: %%%i:%02x <- %%%i:%02x\n",
               }
      }
      bool
   GCRA::doCoalesce(ArrayList& insns, unsigned int mask)
   {
               for (unsigned int n = 0; n < insns.getSize(); ++n) {
      Instruction *i;
            switch (insn->op) {
   case OP_PHI:
      if (!(mask & JOIN_MASK_PHI))
         for (c = 0; insn->srcExists(c); ++c)
      if (!coalesceValues(insn->getDef(0), insn->getSrc(c), false)) {
      // this is bad
   ERROR("failed to coalesce phi operands\n");
            case OP_UNION:
   case OP_MERGE:
      if (!(mask & JOIN_MASK_UNION))
         for (c = 0; insn->srcExists(c); ++c)
         if (insn->op == OP_MERGE) {
      merges.push_back(insn);
   if (insn->srcExists(1))
      }
      case OP_SPLIT:
      if (!(mask & JOIN_MASK_UNION))
         splits.push_back(insn);
   for (c = 0; insn->defExists(c); ++c)
         makeCompound(insn, true);
      case OP_MOV:
      if (!(mask & JOIN_MASK_MOV))
         i = NULL;
   if (!insn->getDef(0)->uses.empty())
         // if this is a contraint-move there will only be a single use
   if (i && i->op == OP_MERGE) // do we really still need this ?
         i = insn->getSrc(0)->getUniqueInsn();
   if (i && !i->constrainedDefs()) {
         }
      case OP_TEX:
   case OP_TXB:
   case OP_TXL:
   case OP_TXF:
   case OP_TXQ:
   case OP_TXD:
   case OP_TXG:
   case OP_TXLQ:
   case OP_TEXCSAA:
   case OP_TEXPREP:
      if (!(mask & JOIN_MASK_TEX))
         for (c = 0; insn->srcExists(c) && c != insn->predSrc; ++c)
            default:
            }
      }
      void
   GCRA::RIG_Node::addInterference(RIG_Node *node)
   {
      this->degree += relDegree[node->colors][colors];
               }
      void
   GCRA::RIG_Node::addRegPreference(RIG_Node *node)
   {
         }
      GCRA::GCRA(Function *fn, SpillCodeInserter& spill, MergedDefs& mergedDefs) :
      nodes(NULL),
   nodeCount(0),
   func(fn),
   regs(fn->getProgram()->getTarget()),
   spill(spill),
      {
         }
      GCRA::~GCRA()
   {
      if (nodes)
      }
      void
   GCRA::checkList(std::list<RIG_Node *>& lst)
   {
               for (std::list<RIG_Node *>::iterator it = lst.begin();
      it != lst.end();
   ++it) {
   assert((*it)->getValue()->join == (*it)->getValue());
   if (prev)
               }
      void
   GCRA::insertOrderedTail(std::list<RIG_Node *>& list, RIG_Node *node)
   {
      if (node->livei.isEmpty())
         // only the intervals of joined values don't necessarily arrive in order
   std::list<RIG_Node *>::iterator prev, it;
   for (it = list.end(); it != list.begin(); it = prev) {
      prev = it;
   --prev;
   if ((*prev)->livei.begin() <= node->livei.begin())
      }
      }
      void
   GCRA::buildRIG(ArrayList& insns)
   {
               for (std::deque<ValueDef>::iterator it = func->ins.begin();
      it != func->ins.end(); ++it)
         for (unsigned int i = 0; i < insns.getSize(); ++i) {
      Instruction *insn = reinterpret_cast<Instruction *>(insns.get(i));
   for (int d = 0; insn->defExists(d); ++d)
      if (insn->getDef(d)->reg.file <= LAST_REGISTER_FILE &&
         }
            while (!values.empty()) {
               for (std::list<RIG_Node *>::iterator it = active.begin();
                     if (node->livei.end() <= cur->livei.begin()) {
         } else {
      if (node->f == cur->f && node->livei.overlaps(cur->livei))
               }
   values.pop_front();
         }
      void
   GCRA::calculateSpillWeights()
   {
      for (unsigned int i = 0; i < nodeCount; ++i) {
      RIG_Node *const n = &nodes[i];
   if (!nodes[i].colors || nodes[i].livei.isEmpty())
         if (nodes[i].reg >= 0) {
      // update max reg
   regs.occupy(n->f, n->reg, n->colors);
      }
            if (!val->noSpill) {
      int rc = 0;
                  nodes[i].weight =
               if (nodes[i].degree < nodes[i].degreeLimit) {
      int l = 0;
   if (val->reg.size > 4)
            } else {
            }
   if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC)
      }
      void
   GCRA::simplifyEdge(RIG_Node *a, RIG_Node *b)
   {
               INFO_DBG(prog->dbgFlags, REG_ALLOC,
            "edge: (%%%i, deg %u/%u) >-< (%%%i, deg %u/%u)\n",
                  move = move && b->degree < b->degreeLimit;
   if (move && !DLLIST_EMPTY(b)) {
      int l = (b->getValue()->reg.size > 4) ? 1 : 0;
   DLLIST_DEL(b);
         }
      void
   GCRA::simplifyNode(RIG_Node *node)
   {
      for (Graph::EdgeIterator ei = node->outgoing(); !ei.end(); ei.next())
            for (Graph::EdgeIterator ei = node->incident(); !ei.end(); ei.next())
            DLLIST_DEL(node);
            INFO_DBG(prog->dbgFlags, REG_ALLOC, "SIMPLIFY: pushed %%%i%s\n",
            }
      bool
   GCRA::simplify()
   {
      for (;;) {
      if (!DLLIST_EMPTY(&lo[0])) {
      do {
            } else
   if (!DLLIST_EMPTY(&lo[1])) {
         } else
   if (!DLLIST_EMPTY(&hi)) {
      RIG_Node *best = hi.next;
   unsigned bestMaxReg = best->maxReg;
   float bestScore = best->weight / (float)best->degree;
   // Spill candidate. First go through the ones with the highest max
   // register, then the ones with lower. That way the ones with the
   // lowest requirement will be allocated first, since it's a stack.
   for (RIG_Node *it = best->next; it != &hi; it = it->next) {
      float score = it->weight / (float)it->degree;
   if (score < bestScore || it->maxReg > bestMaxReg) {
      best = it;
   bestScore = score;
         }
   if (isinf(bestScore)) {
      ERROR("no viable spill candidates left\n");
      }
      } else {
               }
      void
   GCRA::checkInterference(const RIG_Node *node, Graph::EdgeIterator& ei)
   {
               if (intf->reg < 0)
         LValue *vA = node->getValue();
                     if (vA->compound | vB->compound) {
      // NOTE: this only works for >aligned< register tuples !
   for (const ValueDef *D : mergedDefs(vA)) {
   for (const ValueDef *d : mergedDefs(vB)) {
                     if (!vD->livei.overlaps(vd->livei)) {
      INFO_DBG(prog->dbgFlags, REG_ALLOC, "(%%%i) X (%%%i): no overlap\n",
                     uint8_t mask = vD->compound ? vD->compMask : ~0;
   if (vd->compound) {
      assert(vB->compound);
      } else {
                  INFO_DBG(prog->dbgFlags, REG_ALLOC,
            "(%%%i)%02x X (%%%i)%02x & %02x: $r%i.%02x\n",
   vD->id,
   vD->compound ? vD->compMask : 0xff,
   vd->id,
      if (mask)
      }
      } else {
      INFO_DBG(prog->dbgFlags, REG_ALLOC,
                     }
      bool
   GCRA::selectRegisters()
   {
               while (!stack.empty()) {
      RIG_Node *node = &nodes[stack.top()];
                     INFO_DBG(prog->dbgFlags, REG_ALLOC, "\nNODE[%%%i, %u colors]\n",
            for (Graph::EdgeIterator ei = node->outgoing(); !ei.end(); ei.next())
         for (Graph::EdgeIterator ei = node->incident(); !ei.end(); ei.next())
            if (!node->prefRegs.empty()) {
      for (std::list<RIG_Node *>::const_iterator it = node->prefRegs.begin();
      it != node->prefRegs.end();
   ++it) {
   if ((*it)->reg >= 0 &&
      regs.testOccupy(node->f, (*it)->reg, node->colors)) {
   node->reg = (*it)->reg;
            }
   if (node->reg >= 0)
         LValue *lval = node->getValue();
   if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC)
         bool ret = regs.assign(node->reg, node->f, node->colors, node->maxReg);
   if (ret) {
      INFO_DBG(prog->dbgFlags, REG_ALLOC, "assigned reg %i\n", node->reg);
      } else {
      INFO_DBG(prog->dbgFlags, REG_ALLOC, "must spill: %%%i (size %u)\n",
         Symbol *slot = NULL;
   if (lval->reg.file == FILE_GPR)
               }
   if (!mustSpill.empty())
         for (unsigned int i = 0; i < nodeCount; ++i) {
      LValue *lval = nodes[i].getValue();
   if (nodes[i].reg >= 0 && nodes[i].colors > 0)
      lval->reg.data.id =
   }
      }
      bool
   GCRA::allocateRegisters(ArrayList& insns)
   {
               INFO_DBG(prog->dbgFlags, REG_ALLOC,
            nodeCount = func->allLValues.getSize();
   nodes = new RIG_Node[nodeCount];
   if (!nodes)
         for (unsigned int i = 0; i < nodeCount; ++i) {
      LValue *lval = reinterpret_cast<LValue *>(func->allLValues.get(i));
   if (lval) {
                     if (lval->inFile(FILE_GPR) && lval->getInsn() != NULL) {
      Instruction *insn = lval->getInsn();
   if (insn->op != OP_MAD && insn->op != OP_FMA && insn->op != OP_SAD)
         // For both of the cases below, we only want to add the preference
   // if all arguments are in registers.
   if (insn->src(0).getFile() != FILE_GPR ||
      insn->src(1).getFile() != FILE_GPR ||
   insn->src(2).getFile() != FILE_GPR)
      if (prog->getTarget()->getChipset() < 0xc0) {
      // Outputting a flag is not supported with short encodings nor
   // with immediate arguments.
   // See handleMADforNV50.
   if (insn->flagsDef >= 0)
      } else {
      // We can only fold immediate arguments if dst == src2. This
   // only matters if one of the first two arguments is an
   // immediate. This form is also only supported for floats.
   // See handleMADforNVC0.
   ImmediateValue imm;
   if (insn->dType != TYPE_F32)
         if (!insn->src(0).getImmediate(imm) &&
                                       // coalesce first, we use only 1 RIG node for a group of joined values
   ret = coalesce(insns);
   if (!ret)
            if (func->getProgram()->dbgFlags & NV50_IR_DEBUG_REG_ALLOC)
            buildRIG(insns);
   calculateSpillWeights();
   ret = simplify();
   if (!ret)
            ret = selectRegisters();
   if (!ret) {
      INFO_DBG(prog->dbgFlags, REG_ALLOC,
         regs.reset(FILE_GPR, true);
   spill.run(mustSpill);
   if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC)
      } else {
      mergedDefs.merge();
            out:
      cleanup(ret);
      }
      void
   GCRA::cleanup(const bool success)
   {
               for (ArrayList::Iterator it = func->allLValues.iterator();
      !it.end(); it.next()) {
                     lval->compound = 0;
            if (lval->join == lval)
            if (success)
         else
               if (success)
         splits.clear(); // avoid duplicate entries on next coalesce pass
            delete[] nodes;
   nodes = NULL;
   hi.next = hi.prev = &hi;
   lo[0].next = lo[0].prev = &lo[0];
      }
      Symbol *
   SpillCodeInserter::assignSlot(const Interval &livei, const unsigned int size)
   {
               Symbol *sym = new_Symbol(func->getProgram(), FILE_MEMORY_LOCAL);
   sym->setAddress(NULL, address);
                        }
      Value *
   SpillCodeInserter::offsetSlot(Value *base, const LValue *lval)
   {
      if (!lval->compound || (lval->compMask & 0x1))
                  const unsigned int unit = func->getProgram()->getTarget()->getFileUnit(lval->reg.file);
   slot->reg.data.offset += (ffs(lval->compMask) - 1) << unit;
   assert((slot->reg.data.offset % lval->reg.size) == 0);
               }
      void
   SpillCodeInserter::spill(Instruction *defi, Value *slot, LValue *lval)
   {
                        Instruction *st;
   if (slot->reg.file == FILE_MEMORY_LOCAL) {
      lval->noSpill = 1;
   if (ty != TYPE_B96) {
      st = new_Instruction(func, OP_STORE, ty);
   st->setSrc(0, slot);
      } else {
      st = new_Instruction(func, OP_SPLIT, ty);
   st->setSrc(0, lval);
                  for (int d = lval->reg.size / 4 - 1; d >= 0; --d) {
      Value *tmp = cloneShallow(func, slot);
                  Instruction *s = new_Instruction(func, OP_STORE, TYPE_U32);
   s->setSrc(0, tmp);
   s->setSrc(1, st->getDef(d));
            } else {
      st = new_Instruction(func, OP_CVT, ty);
   st->setDef(0, slot);
   st->setSrc(0, lval);
   if (lval->reg.file == FILE_FLAGS)
      }
      }
      LValue *
   SpillCodeInserter::unspill(Instruction *usei, LValue *lval, Value *slot)
   {
               slot = offsetSlot(slot, lval);
            Instruction *ld;
   if (slot->reg.file == FILE_MEMORY_LOCAL) {
      lval->noSpill = 1;
   if (ty != TYPE_B96) {
         } else {
      ld = new_Instruction(func, OP_MERGE, ty);
   for (int d = 0; d < lval->reg.size / 4; ++d) {
      Value *tmp = cloneShallow(func, slot);
   LValue *val;
                  Instruction *l = new_Instruction(func, OP_LOAD, TYPE_U32);
   l->setDef(0, (val = new_LValue(func, FILE_GPR)));
   l->setSrc(0, tmp);
   usei->bb->insertBefore(usei, l);
   ld->setSrc(d, val);
      }
   ld->setDef(0, lval);
   usei->bb->insertBefore(usei, ld);
         } else {
         }
   ld->setDef(0, lval);
   ld->setSrc(0, slot);
   if (lval->reg.file == FILE_FLAGS)
            usei->bb->insertBefore(usei, ld);
      }
      static bool
   value_cmp(ValueRef *a, ValueRef *b) {
      Instruction *ai = a->getInsn(), *bi = b->getInsn();
   if (ai->bb != bi->bb)
            }
      // For each value that is to be spilled, go through all its definitions.
   // A value can have multiple definitions if it has been coalesced before.
   // For each definition, first go through all its uses and insert an unspill
   // instruction before it, then replace the use with the temporary register.
   // Unspill can be either a load from memory or simply a move to another
   // register file.
   // For "Pseudo" instructions (like PHI, SPLIT, MERGE) we can erase the use
   // if we have spilled to a memory location, or simply with the new register.
   // No load or conversion instruction should be needed.
   bool
   SpillCodeInserter::run(const std::list<ValuePair>& lst)
   {
      for (std::list<ValuePair>::const_iterator it = lst.begin(); it != lst.end();
      ++it) {
   LValue *lval = it->first->asLValue();
            // Keep track of which instructions to delete later. Deleting them
   // inside the loop is unsafe since a single instruction may have
   // multiple destinations that all need to be spilled (like OP_SPLIT).
            std::list<ValueDef *> &defs = mergedDefs(lval);
   for (Value::DefIterator d = defs.begin(); d != defs.end();
      ++d) {
   Value *slot = mem ?
                                       // Sort all the uses by BB/instruction so that we don't unspill
   // multiple times in a row, and also remove a source of
   // non-determinism.
                  // Unspill at each use *before* inserting spill instructions,
   // we don't want to have the spill instructions in the use list here.
   for (std::vector<ValueRef*>::const_iterator it = refs.begin();
      it != refs.end(); ++it) {
   ValueRef *u = *it;
   Instruction *usei = u->getInsn();
   assert(usei);
   if (usei->isPseudo()) {
      tmp = (slot->reg.file == FILE_MEMORY_LOCAL) ? NULL : slot;
      } else {
      if (!last || (usei != last->next && usei != last))
            }
               assert(defi);
   if (defi->isPseudo()) {
      d = defs.erase(d);
   --d;
   if (slot->reg.file == FILE_MEMORY_LOCAL)
         else
      } else {
                     for (std::unordered_set<Instruction *>::const_iterator it = to_del.begin();
      it != to_del.end(); ++it) {
   mergedDefs.removeDefsOfInstruction(*it);
                  stackBase = stackSize;
      }
      bool
   RegAlloc::exec()
   {
      for (IteratorRef it = prog->calls.iteratorDFS(false);
      !it->end(); it->next()) {
            func->tlsBase = prog->tlsSize;
   if (!execFunc())
            }
      }
      bool
   RegAlloc::execFunc()
   {
      MergedDefs mergedDefs;
   InsertConstraintsPass insertConstr;
   PhiMovesPass insertPhiMoves;
   BuildIntervalsPass buildIntervals;
                     unsigned int i, retries;
            if (!func->ins.empty()) {
      // Insert a nop at the entry so inputs only used by the first instruction
   // don't count as having an empty live range.
   Instruction *nop = new_Instruction(func, OP_NOP, TYPE_NONE);
               ret = insertConstr.exec(func);
   if (!ret)
            ret = insertPhiMoves.run(func);
   if (!ret)
            // TODO: need to fix up spill slot usage ranges to support > 1 retry
   for (retries = 0; retries < 3; ++retries) {
      if (retries && (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC))
         if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC)
            // spilling to registers may add live ranges, need to rebuild everything
   ret = true;
   for (sequence = func->cfg.nextSequence(), i = 0;
      ret && i <= func->loopNestingBound;
   sequence = func->cfg.nextSequence(), ++i)
      // reset marker
   for (ArrayList::Iterator bi = func->allBBlocks.iterator();
      !bi.end(); bi.next())
      if (!ret)
                  ret = buildIntervals.run(func);
   if (!ret)
         ret = gcra.allocateRegisters(insns);
   if (ret)
      }
               out:
         }
      // TODO: check if modifying Instruction::join here breaks anything
   void
   GCRA::resolveSplitsAndMerges()
   {
      for (std::list<Instruction *>::iterator it = splits.begin();
      it != splits.end();
   ++it) {
   Instruction *split = *it;
   unsigned int reg = regs.idToBytes(split->getSrc(0));
   for (int d = 0; split->defExists(d); ++d) {
      Value *v = split->getDef(d);
   v->reg.data.id = regs.bytesToId(v, reg);
   v->join = v;
         }
            for (std::list<Instruction *>::iterator it = merges.begin();
      it != merges.end();
   ++it) {
   Instruction *merge = *it;
   unsigned int reg = regs.idToBytes(merge->getDef(0));
   for (int s = 0; merge->srcExists(s); ++s) {
      Value *v = merge->getSrc(s);
   v->reg.data.id = regs.bytesToId(v, reg);
   v->join = v;
   // If the value is defined by a phi/union node, we also need to
   // perform the same fixup on that node's sources, since after RA
   // their registers should be identical.
   if (v->getInsn()->op == OP_PHI || v->getInsn()->op == OP_UNION) {
      Instruction *phi = v->getInsn();
   for (int phis = 0; phi->srcExists(phis); ++phis) {
      phi->getSrc(phis)->join = v;
         }
         }
      }
      bool
   RegAlloc::InsertConstraintsPass::exec(Function *ir)
   {
               bool ret = run(ir, true, true);
   if (ret)
            }
      // TODO: make part of texture insn
   void
   RegAlloc::InsertConstraintsPass::textureMask(TexInstruction *tex)
   {
      Value *def[4];
   int c, k, d;
            for (d = 0, k = 0, c = 0; c < 4; ++c) {
      if (!(tex->tex.mask & (1 << c)))
         if (tex->getDef(k)->refCount()) {
      mask |= 1 << c;
      }
      }
            for (c = 0; c < d; ++c)
         for (; c < 4; ++c)
      }
      // Add a dummy use of the pointer source of >= 8 byte loads after the load
   // to prevent it from being assigned a register which overlapping the load's
   // destination, which would produce random corruptions.
   void
   RegAlloc::InsertConstraintsPass::addHazard(Instruction *i, const ValueRef *src)
   {
      Instruction *hzd = new_Instruction(func, OP_NOP, TYPE_NONE);
   hzd->setSrc(0, src->get());
         }
      // b32 { %r0 %r1 %r2 %r3 } -> b128 %r0q
   void
   RegAlloc::InsertConstraintsPass::condenseDefs(Instruction *insn)
   {
      int n;
   for (n = 0; insn->defExists(n) && insn->def(n).getFile() == FILE_GPR; ++n);
      }
      void
   RegAlloc::InsertConstraintsPass::condenseDefs(Instruction *insn,
         {
      uint8_t size = 0;
   if (a >= b)
         for (int s = a; s <= b; ++s)
         if (!size)
            LValue *lval = new_LValue(func, FILE_GPR);
            Instruction *split = new_Instruction(func, OP_SPLIT, typeOfSize(size));
   split->setSrc(0, lval);
   for (int d = a; d <= b; ++d) {
      split->setDef(d - a, insn->getDef(d));
      }
            for (int k = a + 1, d = b + 1; insn->defExists(d); ++d, ++k) {
      insn->setDef(k, insn->getDef(d));
      }
   // carry over predicate if any (mainly for OP_UNION uses)
            insn->bb->insertAfter(insn, split);
      }
      void
   RegAlloc::InsertConstraintsPass::condenseSrcs(Instruction *insn,
         {
      uint8_t size = 0;
   if (a >= b)
         for (int s = a; s <= b; ++s)
         if (!size)
         LValue *lval = new_LValue(func, FILE_GPR);
            Value *save[3];
            Instruction *merge = new_Instruction(func, OP_MERGE, typeOfSize(size));
   merge->setDef(0, lval);
   for (int s = a, i = 0; s <= b; ++s, ++i) {
         }
   insn->moveSources(b + 1, a - b);
   insn->setSrc(a, lval);
                        }
      bool
   RegAlloc::InsertConstraintsPass::isScalarTexGM107(TexInstruction *tex)
   {
      if (tex->tex.sIndirectSrc >= 0 ||
      tex->tex.rIndirectSrc >= 0 ||
   tex->tex.derivAll)
         if (tex->tex.mask == 5 || tex->tex.mask == 6)
            switch (tex->op) {
   case OP_TEX:
   case OP_TXF:
   case OP_TXG:
   case OP_TXL:
         default:
                  // legal variants:
   // TEXS.1D.LZ
   // TEXS.2D
   // TEXS.2D.LZ
   // TEXS.2D.LL
   // TEXS.2D.DC
   // TEXS.2D.LL.DC
   // TEXS.2D.LZ.DC
   // TEXS.A2D
   // TEXS.A2D.LZ
   // TEXS.A2D.LZ.DC
   // TEXS.3D
   // TEXS.3D.LZ
   // TEXS.CUBE
            // TLDS.1D.LZ
   // TLDS.1D.LL
   // TLDS.2D.LZ
   // TLSD.2D.LZ.AOFFI
   // TLDS.2D.LZ.MZ
   // TLDS.2D.LL
   // TLDS.2D.LL.AOFFI
   // TLDS.A2D.LZ
                     switch (tex->op) {
   case OP_TEX:
      if (tex->tex.useOffsets)
            switch (tex->tex.target.getEnum()) {
   case TEX_TARGET_1D:
   case TEX_TARGET_2D_ARRAY_SHADOW:
         case TEX_TARGET_CUBE:
         case TEX_TARGET_2D:
   case TEX_TARGET_2D_ARRAY:
   case TEX_TARGET_2D_SHADOW:
   case TEX_TARGET_3D:
   case TEX_TARGET_RECT:
   case TEX_TARGET_RECT_SHADOW:
         default:
               case OP_TXL:
      if (tex->tex.useOffsets)
            switch (tex->tex.target.getEnum()) {
   case TEX_TARGET_2D:
   case TEX_TARGET_2D_SHADOW:
   case TEX_TARGET_RECT:
   case TEX_TARGET_RECT_SHADOW:
   case TEX_TARGET_CUBE:
         default:
               case OP_TXF:
      switch (tex->tex.target.getEnum()) {
   case TEX_TARGET_1D:
         case TEX_TARGET_2D:
   case TEX_TARGET_RECT:
         case TEX_TARGET_2D_ARRAY:
   case TEX_TARGET_2D_MS:
   case TEX_TARGET_3D:
         default:
               case OP_TXG:
      if (tex->tex.useOffsets > 1)
         if (tex->tex.mask != 0x3 && tex->tex.mask != 0xf)
            switch (tex->tex.target.getEnum()) {
   case TEX_TARGET_2D:
   case TEX_TARGET_2D_MS:
   case TEX_TARGET_2D_SHADOW:
   case TEX_TARGET_RECT:
   case TEX_TARGET_RECT_SHADOW:
         default:
               default:
            }
      void
   RegAlloc::InsertConstraintsPass::handleScalarTexGM107(TexInstruction *tex)
   {
      int defCount = tex->defCount(0xff);
                     // 1. handle defs
   if (defCount > 3)
         if (defCount > 1)
            // 2. handle srcs
   // special case for TXF.A2D
   if (tex->op == OP_TXF && tex->tex.target == TEX_TARGET_2D_ARRAY) {
      assert(srcCount >= 3);
      } else {
      if (srcCount > 3)
         // only if we have more than 2 sources
   if (srcCount > 2)
                  }
      void
   RegAlloc::InsertConstraintsPass::texConstraintGM107(TexInstruction *tex)
   {
               if (isTextureOp(tex->op))
            if (targ->getChipset() < NVISA_GV100_CHIPSET) {
      if (isScalarTexGM107(tex)) {
      handleScalarTexGM107(tex);
               assert(!tex->tex.scalar);
      } else {
      if (isTextureOp(tex->op)) {
      int defCount = tex->defCount(0xff);
   if (defCount > 3)
         if (defCount > 1)
      } else {
                     if (isSurfaceOp(tex->op)) {
      int s = tex->tex.target.getDim() +
                  switch (tex->op) {
   case OP_SUSTB:
   case OP_SUSTP:
      n = 4;
      case OP_SUREDB:
   case OP_SUREDP:
      if (tex->subOp == NV50_IR_SUBOP_ATOM_CAS)
            default:
                  if (s > 1)
         if (n > 1)
      } else
   if (isTextureOp(tex->op)) {
      if (tex->op != OP_TXQ) {
      s = tex->tex.target.getArgCount() - tex->tex.target.isMS();
   if (tex->op == OP_TXD) {
      // Indirect handle belongs in the first arg
   if (tex->tex.rIndirectSrc >= 0)
         if (!tex->tex.target.isArray() && tex->tex.useOffsets)
      }
   n = tex->srcCount(0xff, true) - s;
   // TODO: Is this necessary? Perhaps just has to be aligned to the
   // level that the first arg is, not necessarily to 4. This
   // requirement has not been rigorously verified, as it has been on
   // Kepler.
   if (n > 0 && n < 3) {
      if (tex->srcExists(n + s)) // move potential predicate out of the way
         while (n < 3)
         } else {
      s = tex->srcCount(0xff, true);
               if (s > 1)
         if (n > 1) // NOTE: first call modified positions already
         }
      void
   RegAlloc::InsertConstraintsPass::texConstraintNVE0(TexInstruction *tex)
   {
      if (isTextureOp(tex->op))
                  if (tex->op == OP_SUSTB || tex->op == OP_SUSTP) {
         } else
   if (isTextureOp(tex->op)) {
      int n = tex->srcCount(0xff, true);
   int s = n > 4 ? 4 : n;
   if (n > 4 && n < 7) {
                     while (n < 7)
      }
   if (s > 1)
         if (n > 4)
         }
      void
   RegAlloc::InsertConstraintsPass::texConstraintNVC0(TexInstruction *tex)
   {
               if (isTextureOp(tex->op))
            if (tex->op == OP_TXQ) {
      s = tex->srcCount(0xff);
      } else if (isSurfaceOp(tex->op)) {
      s = tex->tex.target.getDim() + (tex->tex.target.isArray() || tex->tex.target.isCube());
   if (tex->op == OP_SUSTB || tex->op == OP_SUSTP)
         else
      } else {
      s = tex->tex.target.getArgCount() - tex->tex.target.isMS();
   if (!tex->tex.target.isArray() &&
      (tex->tex.rIndirectSrc >= 0 || tex->tex.sIndirectSrc >= 0))
      if (tex->op == OP_TXD && tex->tex.useOffsets)
         n = tex->srcCount(0xff) - s;
               if (s > 1)
         if (n > 1) // NOTE: first call modified positions already
               }
      void
   RegAlloc::InsertConstraintsPass::texConstraintNV50(TexInstruction *tex)
   {
      Value *pred = tex->getPredicate();
   if (pred)
                     assert(tex->defExists(0) && tex->srcExists(0));
   // make src and def count match
   int c;
   for (c = 0; tex->srcExists(c) || tex->defExists(c); ++c) {
      if (!tex->srcExists(c))
         else
         if (!tex->defExists(c))
      }
   if (pred)
         condenseDefs(tex);
      }
      // Insert constraint markers for instructions whose multiple sources must be
   // located in consecutive registers.
   bool
   RegAlloc::InsertConstraintsPass::visit(BasicBlock *bb)
   {
      TexInstruction *tex;
   Instruction *next;
                     for (Instruction *i = bb->getEntry(); i; i = next) {
               if ((tex = i->asTex())) {
      switch (targ->getChipset() & ~0xf) {
   case 0x50:
   case 0x80:
   case 0x90:
   case 0xa0:
      texConstraintNV50(tex);
      case 0xc0:
   case 0xd0:
      texConstraintNVC0(tex);
      case 0xe0:
   case 0xf0:
   case 0x100:
      texConstraintNVE0(tex);
      case 0x110:
   case 0x120:
   case 0x130:
   case 0x140:
   case 0x160:
   case 0x170:
   case 0x190:
      texConstraintGM107(tex);
      default:
            } else
   if (i->op == OP_EXPORT || i->op == OP_STORE) {
      for (size = typeSizeof(i->dType), s = 1; size > 0; ++s) {
      assert(i->srcExists(s));
      }
      } else
   if (i->op == OP_LOAD || i->op == OP_VFETCH) {
      condenseDefs(i);
   if (i->src(0).isIndirect(0) && typeSizeof(i->dType) >= 8)
         if (i->src(0).isIndirect(1) && typeSizeof(i->dType) >= 8)
         if (i->op == OP_LOAD && i->fixed && targ->getChipset() < 0xc0) {
      // Add a hazard to make sure we keep the op around. These are used
   // for membars.
   Instruction *nop = new_Instruction(func, OP_NOP, i->dType);
   nop->setSrc(0, i->getDef(0));
         } else
   if (i->op == OP_UNION ||
      i->op == OP_MERGE ||
   i->op == OP_SPLIT) {
      } else
   if (i->op == OP_ATOM && i->subOp == NV50_IR_SUBOP_ATOM_CAS &&
      targ->getChipset() < 0xc0) {
   // Like a hazard, but for a def.
   Instruction *nop = new_Instruction(func, OP_NOP, i->dType);
   nop->setSrc(0, i->getDef(0));
         }
      }
      void
   RegAlloc::InsertConstraintsPass::insertConstraintMove(Instruction *cst, int s)
   {
                                 bool imm = defi->op == OP_MOV &&
         bool load = defi->op == OP_LOAD &&
      defi->src(0).getFile() == FILE_MEMORY_CONST &&
      // catch some cases where don't really need MOVs
   if (cst->getSrc(s)->refCount() == 1 && !defi->constrainedDefs()
      && defi->op != OP_MERGE && defi->op != OP_SPLIT) {
   if (imm || load) {
      // Move the defi right before the cst. No point in expanding
   // the range.
   defi->bb->remove(defi);
      }
               LValue *lval = new_LValue(func, cst->src(s).getFile());
            Instruction *mov = new_Instruction(func, OP_MOV, typeOfSize(size));
   mov->setDef(0, lval);
            if (load) {
      mov->op = OP_LOAD;
      } else if (imm) {
                  if (defi->getPredicate())
            cst->setSrc(s, mov->getDef(0));
               }
      // Insert extra moves so that, if multiple register constraints on a value are
   // in conflict, these conflicts can be resolved.
   bool
   RegAlloc::InsertConstraintsPass::insertConstraintMoves()
   {
      for (std::list<Instruction *>::iterator it = constrList.begin();
      it != constrList.end();
   ++it) {
   Instruction *cst = *it;
            if (cst->op == OP_SPLIT && false) {
      // spilling splits is annoying, just make sure they're separate
   for (int d = 0; cst->defExists(d); ++d) {
      if (!cst->getDef(d)->refCount())
         LValue *lval = new_LValue(func, cst->def(d).getFile());
                  mov = new_Instruction(func, OP_MOV, typeOfSize(size));
   mov->setSrc(0, lval);
   mov->setDef(0, cst->getDef(d));
                  cst->getSrc(0)->asLValue()->noSpill = 1;
         } else
   if (cst->op == OP_MERGE || cst->op == OP_UNION) {
                        if (!cst->getSrc(s)->defs.size()) {
      mov = new_Instruction(func, OP_NOP, typeOfSize(size));
   mov->setDef(0, cst->getSrc(s));
                                          }
      } // anonymous namespace
      bool Program::registerAllocation()
   {
      RegAlloc ra(this);
      }
      } // namespace nv50_ir
