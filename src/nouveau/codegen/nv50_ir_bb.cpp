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
      namespace nv50_ir {
      Function::Function(Program *p, const char *fnName, uint32_t label)
      : call(this),
   label(label),
   name(fnName),
      {
      cfgExit = NULL;
            bbArray = NULL;
   bbCount = 0;
   loopNestingBound = 0;
            binPos = 0;
            tlsBase = 0;
               }
      Function::~Function()
   {
               if (domTree)
         if (bbArray)
            // clear value refs and defs
   ins.clear();
            for (ArrayList::Iterator it = allInsns.iterator(); !it.end(); it.next())
            for (ArrayList::Iterator it = allLValues.iterator(); !it.end(); it.next())
            for (ArrayList::Iterator BBs = allBBlocks.iterator(); !BBs.end(); BBs.next())
      }
      BasicBlock::BasicBlock(Function *fn) : cfg(this), dom(this), func(fn)
   {
                        numInsns = 0;
   binPos = 0;
                        }
      BasicBlock::~BasicBlock()
   {
         }
      BasicBlock *
   BasicBlock::clone(ClonePolicy<Function>& pol) const
   {
                        for (Instruction *i = getFirst(); i; i = i->next)
                     for (Graph::EdgeIterator it = cfg.outgoing(); !it.end(); it.next()) {
      BasicBlock *obb = BasicBlock::get(it.getNode());
                  }
      BasicBlock *
   BasicBlock::idom() const
   {
      Graph::Node *dn = dom.parent();
      }
      void
   BasicBlock::insertHead(Instruction *inst)
   {
               if (inst->op == OP_PHI) {
      if (phi) {
         } else {
      if (entry) {
         } else {
      assert(!exit);
   phi = exit = inst;
   inst->bb = this;
            } else {
      if (entry) {
         } else {
      if (phi) {
         } else {
      assert(!exit);
   entry = exit = inst;
   inst->bb = this;
               }
      void
   BasicBlock::insertTail(Instruction *inst)
   {
               if (inst->op == OP_PHI) {
      if (entry) {
         } else
   if (exit) {
      assert(phi);
      } else {
      assert(!phi);
   phi = exit = inst;
   inst->bb = this;
         } else {
      if (exit) {
         } else {
      assert(!phi);
   entry = exit = inst;
   inst->bb = this;
            }
      void
   BasicBlock::insertBefore(Instruction *q, Instruction *p)
   {
                        if (q == entry) {
      if (p->op == OP_PHI) {
      if (!phi)
      } else {
            } else
   if (q == phi) {
      assert(p->op == OP_PHI);
               p->next = q;
   p->prev = q->prev;
   if (p->prev)
                  p->bb = this;
      }
      void
   BasicBlock::insertAfter(Instruction *p, Instruction *q)
   {
      assert(p && q);
                     if (p == exit)
         if (p->op == OP_PHI && q->op != OP_PHI)
            q->prev = p;
   q->next = p->next;
   if (q->next)
                  q->bb = this;
      }
      void
   BasicBlock::remove(Instruction *insn)
   {
               if (insn->prev)
            if (insn->next)
         else
            if (insn == entry) {
      if (insn->next)
         else
   if (insn->prev && insn->prev->op != OP_PHI)
         else
               if (insn == phi)
            --numInsns;
   insn->bb = NULL;
   insn->next =
      }
      void BasicBlock::permuteAdjacent(Instruction *a, Instruction *b)
   {
               if (a->next != b) {
      Instruction *i = a;
   a = b;
      }
   assert(a->next == b);
            if (b == exit)
         if (a == entry)
            b->prev = a->prev;
   a->next = b->next;
   b->next = a;
            if (b->prev)
         if (a->next)
      }
      void
   BasicBlock::splitCommon(Instruction *insn, BasicBlock *bb, bool attach)
   {
               if (insn) {
      exit = insn->prev;
               if (exit)
         else
            while (!cfg.outgoing(true).end()) {
      Graph::Edge *e = cfg.outgoing(true).getEdge();
   bb->cfg.attach(e->getTarget(), e->getType());
               for (; insn; insn = insn->next) {
      this->numInsns--;
   bb->numInsns++;
   insn->bb = bb;
      }
   if (attach)
      }
      BasicBlock *
   BasicBlock::splitBefore(Instruction *insn, bool attach)
   {
      BasicBlock *bb = new BasicBlock(func);
            bb->joinAt = joinAt;
            splitCommon(insn, bb, attach);
      }
      BasicBlock *
   BasicBlock::splitAfter(Instruction *insn, bool attach)
   {
      BasicBlock *bb = new BasicBlock(func);
            bb->joinAt = joinAt;
            splitCommon(insn ? insn->next : NULL, bb, attach);
      }
      bool
   BasicBlock::dominatedBy(BasicBlock *that)
   {
      Graph::Node *bn = &that->dom;
            while (dn && dn != bn)
               }
      unsigned int
   BasicBlock::initiatesSimpleConditional() const
   {
      Graph::Node *out[2];
   int n;
            if (cfg.outgoingCount() != 2) // -> if and -> else/endif
            n = 0;
   for (Graph::EdgeIterator ei = cfg.outgoing(); !ei.end(); ei.next())
                  // IF block is out edge to the right
   if (eR == Graph::Edge::CROSS || eR == Graph::Edge::BACK)
            if (out[1]->outgoingCount() != 1) // 0 is IF { RET; }, >1 is more divergence
         // do they reconverge immediately ?
   if (out[1]->outgoing().getNode() == out[0])
         if (out[0]->outgoingCount() == 1)
      if (out[0]->outgoing().getNode() == out[1]->outgoing().getNode())
            }
      bool
   Function::setEntry(BasicBlock *bb)
   {
      if (cfg.getRoot())
         cfg.insert(&bb->cfg);
      }
      bool
   Function::setExit(BasicBlock *bb)
   {
      if (cfgExit)
         cfgExit = &bb->cfg;
      }
      unsigned int
   Function::orderInstructions(ArrayList &result)
   {
               for (IteratorRef it = cfg.iteratorCFG(); !it->end(); it->next()) {
      BasicBlock *bb =
            for (Instruction *insn = bb->getFirst(); insn; insn = insn->next)
                  }
      void
   Function::buildLiveSets()
   {
      for (unsigned i = 0; i <= loopNestingBound; ++i)
            for (ArrayList::Iterator bi = allBBlocks.iterator(); !bi.end(); bi.next())
      }
      bool
   Pass::run(Program *prog, bool ordered, bool skipPhi)
   {
      this->prog = prog;
   err = false;
      }
      bool
   Pass::doRun(Program *prog, bool ordered, bool skipPhi)
   {
      for (IteratorRef it = prog->calls.iteratorDFS(false);
      !it->end(); it->next()) {
   Graph::Node *n = reinterpret_cast<Graph::Node *>(it->get());
   if (!doRun(Function::get(n), ordered, skipPhi))
      }
      }
      bool
   Pass::run(Function *func, bool ordered, bool skipPhi)
   {
      prog = func->getProgram();
   err = false;
      }
      bool
   Pass::doRun(Function *func, bool ordered, bool skipPhi)
   {
      IteratorRef bbIter;
   BasicBlock *bb;
            this->func = func;
   if (!visit(func))
                     for (; !bbIter->end(); bbIter->next()) {
      bb = BasicBlock::get(reinterpret_cast<Graph::Node *>(bbIter->get()));
   if (!visit(bb))
         for (insn = skipPhi ? bb->getEntry() : bb->getFirst(); insn != NULL;
      insn = next) {
   next = insn->next;
   if (!visit(insn))
                     }
      void
   Function::printCFGraph(const char *filePath)
   {
      FILE *out = fopen(filePath, "a");
   if (!out) {
      ERROR("failed to open file: %s\n", filePath);
      }
                     for (IteratorRef it = cfg.iteratorDFS(); !it->end(); it->next()) {
      BasicBlock *bb = BasicBlock::get(
         int idA = bb->getId();
   for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
      int idB = BasicBlock::get(ei.getNode())->getId();
   switch (ei.getType()) {
   case Graph::Edge::TREE:
      fprintf(out, "\t%i -> %i;\n", idA, idB);
      case Graph::Edge::FORWARD:
      fprintf(out, "\t%i -> %i [color=green];\n", idA, idB);
      case Graph::Edge::CROSS:
      fprintf(out, "\t%i -> %i [color=red];\n", idA, idB);
      case Graph::Edge::BACK:
      fprintf(out, "\t%i -> %i;\n", idA, idB);
      default:
      assert(0);
                     fprintf(out, "}\n");
      }
      } // namespace nv50_ir
