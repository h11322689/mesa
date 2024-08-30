   /* -*- mesa-c++  -*-
   *
   * Copyright (c) 2022 Collabora LTD
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
      #include "sfn_alu_readport_validation.h"
      #include <cstring>
      namespace r600 {
      class ReserveReadport : public ConstRegisterVisitor {
   public:
                        void visit(const LocalArray& value) override;
   void visit(const LiteralConstant& value) override;
                     AluReadportReservation& reserver;
   int cycle = -1;
   int isrc = -1;
   int src0_sel = -1;
   int src0_chan = -1;
               };
      class ReserveReadportVec : public ReserveReadport {
   public:
      using ReserveReadport::ReserveReadport;
            void visit(const Register& value) override;
   void visit(const LocalArrayValue& value) override;
      };
      class ReserveReadportTrans : public ReserveReadport {
   public:
                  };
      class ReserveReadportTransPass1 : public ReserveReadportTrans {
   public:
      using ReserveReadportTrans::ReserveReadportTrans;
            void visit(const Register& value) override;
   void visit(const LocalArrayValue& value) override;
   void visit(const UniformValue& value) override;
   void visit(const InlineConstant& value) override;
      };
      class ReserveReadportTransPass2 : public ReserveReadportTrans {
   public:
      using ReserveReadportTrans::ReserveReadportTrans;
            void visit(const Register& value) override;
   void visit(const LocalArrayValue& value) override;
      };
      bool
   AluReadportReservation::schedule_vec_src(PVirtualValue src[3],
               {
               if (src[0]->as_register()) {
      visitor.src0_sel = src[0]->sel();
      } else {
      visitor.src0_sel = 0xffff;
               for (int i = 0; i < nsrc; ++i) {
      visitor.cycle = cycle_vec(swz, i);
   visitor.isrc = i;
                  }
      bool
   AluReadportReservation::schedule_vec_instruction(const AluInstr& alu, AluBankSwizzle swz)
   {
               for (unsigned i = 0; i < alu.n_sources() && visitor.success; ++i) {
      visitor.cycle = cycle_vec(swz, i);
   visitor.isrc = i;
   if (i == 1 && alu.src(i).equal_to(alu.src(0)))
            }
      }
      bool
   AluReadportReservation::schedule_trans_instruction(const AluInstr& alu,
         {
                  for (unsigned i = 0; i < alu.n_sources(); ++i) {
      visitor1.cycle = cycle_trans(swz, i);
      }
   if (!visitor1.success)
            ReserveReadportTransPass2 visitor2(*this);
            for (unsigned i = 0; i < alu.n_sources(); ++i) {
                  }
      }
      void AluReadportReservation::print(std::ostream& os) const
   {
      os << "AluReadportReservation\n";
   for (int i = 0; i < max_chan_channels; ++i) {
      os << "  chan " << i << ":";
   for (int j = 0; j < max_gpr_readports; ++j) {
         }
      }
         }
      AluReadportReservation::AluReadportReservation()
   {
      for (int i = 0; i < max_chan_channels; ++i) {
      for (int j = 0; j < max_gpr_readports; ++j)
         m_hw_const_addr[i] = -1;
   m_hw_const_chan[i] = -1;
         }
      bool
   AluReadportReservation::reserve_gpr(int sel, int chan, int cycle)
   {
      if (m_hw_gpr[cycle][chan] == -1) {
         } else if (m_hw_gpr[cycle][chan] != sel) {
         }
      }
      bool
   AluReadportReservation::reserve_const(const UniformValue& value)
   {
      int match = -1;
            for (int res = 0; res < ReserveReadport::max_const_readports; ++res) {
      if (m_hw_const_addr[res] == -1)
         else if ((m_hw_const_addr[res] == value.sel()) &&
            (m_hw_const_bank[res] == value.kcache_bank()) &&
            if (match < 0) {
      if (empty >= 0) {
      m_hw_const_addr[empty] = value.sel();
   (m_hw_const_bank[empty] = value.kcache_bank());
      } else {
            }
      }
      bool
   AluReadportReservation::add_literal(uint32_t value)
   {
      for (unsigned i = 0; i < m_nliterals; ++i) {
      if (m_literals[i] == value)
      }
   if (m_nliterals < m_literals.size()) {
      m_literals[m_nliterals++] = value;
      }
      }
      int
   AluReadportReservation::cycle_vec(AluBankSwizzle swz, int src)
   {
      static const int mapping[AluBankSwizzle::alu_vec_unknown][max_gpr_readports] = {
      {0, 1, 2},
   {0, 2, 1},
   {1, 2, 0},
   {1, 0, 2},
   {2, 0, 1},
      };
      }
      int
   AluReadportReservation::cycle_trans(AluBankSwizzle swz, int src)
   {
      static const int mapping[AluBankSwizzle::sq_alu_scl_unknown][max_gpr_readports] = {
      {2, 1, 0},
   {1, 2, 2},
   {2, 1, 2},
      };
      }
      ReserveReadport::ReserveReadport(AluReadportReservation& reserv):
         {
   }
      void
   ReserveReadport::visit(const LocalArray& value)
   {
      (void)value;
      }
      void
   ReserveReadport::visit(const LiteralConstant& value)
   {
         }
      void
   ReserveReadport::visit(const InlineConstant& value)
   {
         }
      void
   ReserveReadportVec::visit(const Register& value)
   {
         }
      void
   ReserveReadportVec::visit(const LocalArrayValue& value)
   {
      // Set the highest non-sign bit to indicated that we use the
   // AR register
      }
      void
   ReserveReadport::reserve_gpr(int sel, int chan)
   {
      if (isrc == 1 && src0_sel == sel && src0_chan == chan)
            }
      void
   ReserveReadportVec::visit(const UniformValue& value)
   {
      // kcache bank?
      }
      ReserveReadportTrans::ReserveReadportTrans(AluReadportReservation& reserv):
      ReserveReadport(reserv),
      {
   }
      void
   ReserveReadportTransPass1::visit(const Register& value)
   {
         }
      void
   ReserveReadportTransPass1::visit(const LocalArrayValue& value)
   {
         }
      void
   ReserveReadportTransPass1::visit(const UniformValue& value)
   {
      if (n_consts >= max_const_readports) {
      success = false;
      }
   n_consts++;
      }
      void
   ReserveReadportTransPass1::visit(const InlineConstant& value)
   {
      (void)value;
   if (n_consts >= max_const_readports) {
      success = false;
      }
      }
      void
   ReserveReadportTransPass1::visit(const LiteralConstant& value)
   {
      if (n_consts >= max_const_readports) {
      success = false;
      }
   n_consts++;
      }
      void
   ReserveReadportTransPass2::visit(const Register& value)
   {
      if (cycle < n_consts) {
      success = false;
      }
      }
      void
   ReserveReadportTransPass2::visit(const LocalArrayValue& value)
   {
      if (cycle < n_consts) {
      success = false;
      }
      }
      void
   ReserveReadportTransPass2::visit(const UniformValue& value)
   {
         }
      } // namespace r600
