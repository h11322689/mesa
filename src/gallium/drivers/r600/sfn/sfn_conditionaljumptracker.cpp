   /* -*- mesa-c++  -*-
   *
   * Copyright (c) 2019 Collabora LTD
   *
   * Author: Gert Wollny <gert.wollny@collabora.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "sfn_conditionaljumptracker.h"
      #include "sfn_debug.h"
      #include <iostream>
   #include <memory>
   #include <stack>
   #include <vector>
      namespace r600 {
      using std::shared_ptr;
   using std::stack;
   using std::vector;
      struct StackFrame {
         StackFrame(r600_bytecode_cf *s, JumpType t):
      type(t),
      {
                     JumpType type;
   r600_bytecode_cf *start;
            virtual void fixup_mid(r600_bytecode_cf *cf) = 0;
      };
      using PStackFrame = shared_ptr<StackFrame>;
      struct IfFrame : public StackFrame {
      IfFrame(r600_bytecode_cf *s);
   void fixup_mid(r600_bytecode_cf *cf) override;
      };
      struct LoopFrame : public StackFrame {
      LoopFrame(r600_bytecode_cf *s);
   void fixup_mid(r600_bytecode_cf *cf) override;
      };
      struct ConditionalJumpTrackerImpl {
      ConditionalJumpTrackerImpl();
   stack<PStackFrame> m_jump_stack;
   stack<PStackFrame> m_loop_stack;
      };
      ConditionalJumpTrackerImpl::ConditionalJumpTrackerImpl():
         {
   }
      ConditionalJumpTracker::~ConditionalJumpTracker() { delete impl; }
      ConditionalJumpTracker::ConditionalJumpTracker()
   {
         }
      void
   ConditionalJumpTracker::push(r600_bytecode_cf *start, JumpType type)
   {
      PStackFrame f;
   switch (type) {
   case jt_if:
      f.reset(new IfFrame(start));
      case jt_loop:
      f.reset(new LoopFrame(start));
   impl->m_loop_stack.push(f);
      }
      }
      bool
   ConditionalJumpTracker::pop(r600_bytecode_cf *final, JumpType type)
   {
      if (impl->m_jump_stack.empty())
            auto& frame = *impl->m_jump_stack.top();
   if (frame.type != type)
            frame.fixup_pop(final);
   if (frame.type == jt_loop)
         impl->m_jump_stack.pop();
      }
      bool
   ConditionalJumpTracker::add_mid(r600_bytecode_cf *source, JumpType type)
   {
      if (impl->m_jump_stack.empty()) {
      sfn_log << "Jump stack empty\n";
               PStackFrame pframe;
   if (type == jt_loop) {
      if (impl->m_loop_stack.empty()) {
      sfn_log << "Loop jump stack empty\n";
      }
      } else {
                  pframe->mid.push_back(source);
   pframe->fixup_mid(source);
      }
      IfFrame::IfFrame(r600_bytecode_cf *s):
         {
   }
      StackFrame::~StackFrame() {}
      void
   IfFrame::fixup_mid(r600_bytecode_cf *source)
   {
      /* JUMP target is ELSE */
      }
      void
   IfFrame::fixup_pop(r600_bytecode_cf *final)
   {
      /* JUMP or ELSE target is one past last CF instruction */
   unsigned offset = final->eg_alu_extended ? 4 : 2;
   auto src = mid.empty() ? start : mid[0];
   src->cf_addr = final->id + offset;
      }
      LoopFrame::LoopFrame(r600_bytecode_cf *s):
         {
   }
      void
   LoopFrame::fixup_mid(UNUSED r600_bytecode_cf *mid)
   {
   }
      void
   LoopFrame::fixup_pop(r600_bytecode_cf *final)
   {
      /* LOOP END address is past LOOP START */
            /* LOOP START address is past LOOP END*/
            /* BREAK and CONTINUE point at LOOP END*/
   for (auto m : mid)
      }
      } // namespace r600
