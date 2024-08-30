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
      #include "nv50_ir_graph.h"
   #include <limits>
   #include <list>
   #include <stack>
   #include "nv50_ir.h"
      namespace nv50_ir {
      Graph::Graph()
   {
      root = NULL;
   size = 0;
      }
      Graph::~Graph()
   {
      for (IteratorRef it = safeIteratorDFS(); !it->end(); it->next())
      }
      void Graph::insert(Node *node)
   {
      if (!root)
            node->graph = this;
      }
      void Graph::Edge::unlink()
   {
      if (origin) {
      prev[0]->next[0] = next[0];
   next[0]->prev[0] = prev[0];
   if (origin->out == this)
               }
   if (target) {
      prev[1]->next[1] = next[1];
   next[1]->prev[1] = prev[1];
   if (target->in == this)
                  }
      const char *Graph::Edge::typeStr() const
   {
      switch (type) {
   case TREE:    return "tree";
   case FORWARD: return "forward";
   case BACK:    return "back";
   case CROSS:   return "cross";
   case UNKNOWN:
   default:
            }
      Graph::Node::Node(void *priv) : data(priv),
                           {
         }
      void Graph::Node::attach(Node *node, Edge::Type kind)
   {
               // insert head
   if (this->out) {
      edge->next[0] = this->out;
   edge->prev[0] = this->out->prev[0];
   edge->prev[0]->next[0] = edge;
      }
            if (node->in) {
      edge->next[1] = node->in;
   edge->prev[1] = node->in->prev[1];
   edge->prev[1]->next[1] = edge;
      }
            ++this->outCount;
            assert(graph || node->graph);
   if (!node->graph)
         if (!graph)
            if (kind == Edge::UNKNOWN)
      }
      bool Graph::Node::detach(Graph::Node *node)
   {
      EdgeIterator ei = this->outgoing();
   for (; !ei.end(); ei.next())
      if (ei.getNode() == node)
      if (ei.end()) {
      ERROR("no such node attached\n");
      }
   delete ei.getEdge();
      }
      // Cut a node from the graph, deleting all attached edges.
   void Graph::Node::cut()
   {
      while (out)
         while (in)
            if (graph) {
      if (graph->root == this)
               }
      Graph::Edge::Edge(Node *org, Node *tgt, Type kind)
   {
      target = tgt;
   origin = org;
            next[0] = next[1] = this;
      }
      bool
   Graph::Node::reachableBy(const Node *node, const Node *term) const
   {
      std::stack<const Node *> stack;
   const Node *pos = NULL;
                     while (!stack.empty()) {
      pos = stack.top();
            if (pos == this)
         if (pos == term)
            for (EdgeIterator ei = pos->outgoing(); !ei.end(); ei.next()) {
      if (ei.getType() == Edge::BACK)
         if (ei.getNode()->visit(seq))
         }
      }
      class DFSIterator : public Iterator
   {
   public:
      DFSIterator(Graph *graph, const bool preorder)
   {
               nodes = new Graph::Node * [graph->getSize() + 1];
   count = 0;
   pos = 0;
            if (graph->getRoot()) {
      graph->getRoot()->visit(seq);
                  ~DFSIterator()
   {
      if (nodes)
               void search(Graph::Node *node, const bool preorder, const int sequence)
   {
      if (preorder)
            for (Graph::EdgeIterator ei = node->outgoing(); !ei.end(); ei.next())
                  if (!preorder)
               virtual bool end() const { return pos >= count; }
   virtual void next() { if (pos < count) ++pos; }
   virtual void *get() const { return nodes[pos]; }
         protected:
      Graph::Node **nodes;
   int count;
      };
      IteratorRef Graph::iteratorDFS(bool preorder)
   {
         }
      IteratorRef Graph::safeIteratorDFS(bool preorder)
   {
         }
      class CFGIterator : public Iterator
   {
   public:
      CFGIterator(Graph *graph)
   {
      nodes = new Graph::Node * [graph->getSize() + 1];
   count = 0;
   pos = 0;
            // TODO: argh, use graph->sequence instead of tag and just raise it by > 1
   for (IteratorRef it = graph->iteratorDFS(); !it->end(); it->next())
            if (graph->getRoot())
               ~CFGIterator()
   {
      if (nodes)
               virtual void *get() const { return nodes[pos]; }
   virtual bool end() const { return pos >= count; }
   virtual void next() { if (pos < count) ++pos; }
         private:
      void search(Graph::Node *node, const int sequence)
   {
                        while (bb.getSize() || cross.getSize()) {
                     node = reinterpret_cast<Graph::Node *>(bb.pop().u.p);
   assert(node);
   if (!node->visit(sequence))
                  for (Graph::EdgeIterator ei = node->outgoing(); !ei.end(); ei.next()) {
      switch (ei.getType()) {
   case Graph::Edge::TREE:
   case Graph::Edge::FORWARD:
      if (++(ei.getNode()->tag) == ei.getNode()->incidentCountFwd())
            case Graph::Edge::BACK:
         case Graph::Edge::CROSS:
      if (++(ei.getNode()->tag) == 1)
            default:
      assert(!"unknown edge kind in CFG");
         }
               private:
      Graph::Node **nodes;
   int count;
      };
      IteratorRef Graph::iteratorCFG()
   {
         }
      IteratorRef Graph::safeIteratorCFG()
   {
         }
      /**
   * Edge classification:
   *
   * We have a graph and want to classify the edges into one of four types:
   * - TREE:    edges that belong to a spanning tree of the graph
   * - FORWARD: edges from a node to a descendent in the spanning tree
   * - BACK:    edges from a node to a parent (or itself) in the spanning tree
   * - CROSS:   all other edges (because they cross between branches in the
   *            spanning tree)
   */
   void Graph::classifyEdges()
   {
               for (IteratorRef it = iteratorDFS(true); !it->end(); it->next()) {
      Node *node = reinterpret_cast<Node *>(it->get());
   node->visit(0);
                           }
      void Graph::classifyDFS(Node *curr, int& seq)
   {
      Graph::Edge *edge;
            curr->visit(++seq);
            for (edge = curr->out; edge; edge = edge->next[0]) {
               if (node->getSequence() == 0) {
      edge->type = Edge::TREE;
      } else
   if (node->getSequence() > curr->getSequence()) {
         } else {
                     for (edge = curr->in; edge; edge = edge->next[1]) {
               if (node->getSequence() == 0) {
      edge->type = Edge::TREE;
      } else
   if (node->getSequence() > curr->getSequence()) {
         } else {
                        }
      // @dist is indexed by Node::tag, returns -1 if no path found
   int
   Graph::findLightestPathWeight(Node *a, Node *b, const std::vector<int> &weight)
   {
      std::vector<int> path(weight.size(), std::numeric_limits<int>::max());
   std::list<Node *> nodeList;
            path[a->tag] = 0;
   for (Node *c = a; c && c != b;) {
      const int p = path[c->tag] + weight[c->tag];
   for (EdgeIterator ei = c->outgoing(); !ei.end(); ei.next()) {
      Node *t = ei.getNode();
   if (t->getSequence() < seq) {
      if (path[t->tag] == std::numeric_limits<int>::max())
         if (p < path[t->tag])
         }
   c->visit(seq);
   Node *next = NULL;
   for (std::list<Node *>::iterator n = nodeList.begin();
      n != nodeList.end(); ++n) {
   if (!next || path[(*n)->tag] < path[next->tag])
         if ((*n) == c) {
      // erase visited
   n = nodeList.erase(n);
         }
      }
   if (path[b->tag] == std::numeric_limits<int>::max())
            }
      } // namespace nv50_ir
