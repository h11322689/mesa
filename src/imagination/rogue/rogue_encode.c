   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "rogue.h"
   #include "rogue_isa.h"
   #include "util/macros.h"
   #include "util/u_dynarray.h"
      #include <stdbool.h>
      /**
   * \file rogue_encode.c
   *
   * \brief Contains hardware encoding functions.
   */
      #define util_dynarray_append_mem(buf, size, mem) \
            static unsigned rogue_calc_da(const rogue_instr_group *group)
   {
               if (group->header.alu == ROGUE_ALU_MAIN) {
      for (unsigned u = ROGUE_INSTR_PHASE_COUNT; u > 0; --u) {
      enum rogue_instr_phase p = u - 1;
   if (p > ROGUE_INSTR_PHASE_1)
         } else if (group->header.alu == ROGUE_ALU_BITWISE) {
      for (unsigned u = ROGUE_INSTR_PHASE_COUNT; u > 0; --u) {
      enum rogue_instr_phase p = u - 1;
         } else if (group->header.alu == ROGUE_ALU_CONTROL) {
      const rogue_instr *instr = group->instrs[ROGUE_INSTR_PHASE_CTRL];
            if (!rogue_ctrl_op_has_srcs(ctrl->op) &&
      !rogue_ctrl_op_has_dsts(ctrl->op)) {
      } else {
               } else {
                     }
      #define P(type) BITFIELD64_BIT(ROGUE_INSTR_PHASE_##type)
   static enum oporg rogue_calc_oporg(uint64_t alu_phases)
   {
      bool P0 = !!(alu_phases & P(0));
   bool P1 = !!(alu_phases & P(1));
   bool P2 = !!(alu_phases & (P(2_PCK) | P(2_TST) | P(2_MOV)));
            if (P0 && P1 && P2 && PBE)
         else if (P0 && !P1 && P2 && PBE)
         else if (P0 && P1 && P2 && !PBE)
         else if (P0 && !P1 && P2 && !PBE)
         else if (P0 && P1 && !P2 && !PBE)
         else if (!P0 && !P1 && !P2 && PBE)
         else if (!P0 && !P1 && P2 && !PBE)
         else if (P0 && !P1 && !P2 && !PBE)
               }
      static enum opcnt rogue_calc_opcnt(uint64_t bitwise_phases)
   {
               if (bitwise_phases & P(0_BITMASK) || bitwise_phases & P(0_SHIFT1) ||
      bitwise_phases & P(0_COUNT)) {
               if (bitwise_phases & P(1_LOGICAL))
            if (bitwise_phases & P(2_SHIFT2) || bitwise_phases & P(2_TEST))
               }
      static void rogue_encode_instr_group_header(rogue_instr_group *group,
         {
               h.da = rogue_calc_da(group);
   h.length = (group->size.total / 2) % 16;
            rogue_ref *w0ref = rogue_instr_group_io_sel_ref(&group->io_sel, ROGUE_IO_W0);
            /* TODO: Update this - needs to be set for MOVMSK, and if instruction group
   * READS OR WRITES to/from pixout regs. */
   h.olchk = rogue_ref_is_pixout(w0ref) || rogue_ref_is_pixout(w1ref);
   h.w1p = !rogue_ref_is_null(w1ref);
            rogue_cc cc = { 0 };
   switch (group->header.exec_cond) {
   case ROGUE_EXEC_COND_PE_TRUE:
      cc._ = CC_PE_TRUE;
         case ROGUE_EXEC_COND_P0_TRUE:
      cc._ = CC_P0_TRUE;
         case ROGUE_EXEC_COND_PE_ANY:
      cc._ = CC_PE_ANY;
         case ROGUE_EXEC_COND_P0_FALSE:
      cc._ = CC_P0_FALSE;
         default:
                  h.cc = cc.cc;
            switch (group->header.alu) {
   case ROGUE_ALU_MAIN:
      h.alutype = ALUTYPE_MAIN;
   h.oporg = rogue_calc_oporg(group->header.phases);
         case ROGUE_ALU_BITWISE:
      h.alutype = ALUTYPE_BITWISE;
   h.opcnt = rogue_calc_opcnt(group->header.phases);
         case ROGUE_ALU_CONTROL:
      #define OM(op_mod) ROGUE_CTRL_OP_MOD_##op_mod
         const rogue_instr *instr = group->instrs[ROGUE_INSTR_PHASE_CTRL];
   const rogue_ctrl_instr *ctrl = rogue_instr_as_ctrl(instr);
   switch (ctrl->op) {
   case ROGUE_CTRL_OP_NOP:
      h.ctrlop = CTRLOP_NOP;
               case ROGUE_CTRL_OP_WOP:
                  case ROGUE_CTRL_OP_BR:
   case ROGUE_CTRL_OP_BA:
                  case ROGUE_CTRL_OP_WDF:
      h.ctrlop = CTRLOP_WDF;
               default:
         #undef OM
               default:
                  if (group->header.alu != ROGUE_ALU_CONTROL) {
      h.end = group->header.end;
   /* h.atom = ; */ /* Unused for now */
                  }
   #undef P
      typedef union rogue_instr_encoding {
      rogue_alu_instr_encoding alu;
   rogue_backend_instr_encoding backend;
   rogue_ctrl_instr_encoding ctrl;
      } PACKED rogue_instr_encoding;
      static unsigned rogue_alu_movc_ft(const rogue_ref *ref)
   {
      switch (rogue_ref_get_io(ref)) {
   case ROGUE_IO_NONE:
   case ROGUE_IO_FT0:
            case ROGUE_IO_FT1:
            case ROGUE_IO_FT2:
            case ROGUE_IO_FTE:
            default:
                     }
      #define SM(src_mod) ROGUE_ALU_SRC_MOD_##src_mod
   #define DM(dst_mod) ROGUE_ALU_DST_MOD_##dst_mod
   #define OM(op_mod) ROGUE_ALU_OP_MOD_##op_mod
   static void rogue_encode_alu_instr(const rogue_alu_instr *alu,
               {
      switch (alu->op) {
   case ROGUE_ALU_OP_MBYP:
      instr_encoding->alu.op = ALUOP_SNGL;
            if (instr_size == 2) {
      instr_encoding->alu.sngl.ext0 = 1;
   instr_encoding->alu.sngl.mbyp.s0neg =
         instr_encoding->alu.sngl.mbyp.s0abs =
      }
         case ROGUE_ALU_OP_FMUL:
      instr_encoding->alu.op = ALUOP_FMUL;
   instr_encoding->alu.fmul.lp = rogue_alu_op_mod_is_set(alu, OM(LP));
   instr_encoding->alu.fmul.sat = rogue_alu_op_mod_is_set(alu, OM(SAT));
   instr_encoding->alu.fmul.s0neg =
         instr_encoding->alu.fmul.s0abs =
         instr_encoding->alu.fmul.s1abs =
         instr_encoding->alu.fmul.s0flr =
               case ROGUE_ALU_OP_FMAD:
      instr_encoding->alu.op = ALUOP_FMAD;
   instr_encoding->alu.fmad.s0neg =
         instr_encoding->alu.fmad.s0abs =
         instr_encoding->alu.fmad.s2neg =
                  if (instr_size == 2) {
      instr_encoding->alu.fmad.ext = 1;
   instr_encoding->alu.fmad.lp = rogue_alu_op_mod_is_set(alu, OM(LP));
   instr_encoding->alu.fmad.s1abs =
         instr_encoding->alu.fmad.s1neg =
         instr_encoding->alu.fmad.s2flr =
         instr_encoding->alu.fmad.s2abs =
      }
         case ROGUE_ALU_OP_TST: {
      instr_encoding->alu.op = ALUOP_TST;
            rogue_tstop tstop = { 0 };
   if (rogue_alu_op_mod_is_set(alu, OM(Z)))
         else if (rogue_alu_op_mod_is_set(alu, OM(GZ)))
         else if (rogue_alu_op_mod_is_set(alu, OM(GEZ)))
         else if (rogue_alu_op_mod_is_set(alu, OM(C)))
         else if (rogue_alu_op_mod_is_set(alu, OM(E)))
         else if (rogue_alu_op_mod_is_set(alu, OM(G)))
         else if (rogue_alu_op_mod_is_set(alu, OM(GE)))
         else if (rogue_alu_op_mod_is_set(alu, OM(NE)))
         else if (rogue_alu_op_mod_is_set(alu, OM(L)))
         else if (rogue_alu_op_mod_is_set(alu, OM(LE)))
         else
                     if (instr_size == 2) {
                     if (rogue_alu_src_mod_is_set(alu, 0, SM(E0)))
         else if (rogue_alu_src_mod_is_set(alu, 0, SM(E1)))
         else if (rogue_alu_src_mod_is_set(alu, 0, SM(E2)))
                        instr_encoding->alu.tst.p2end =
                  if (rogue_alu_op_mod_is_set(alu, OM(F32)))
         else if (rogue_alu_op_mod_is_set(alu, OM(U16)))
         else if (rogue_alu_op_mod_is_set(alu, OM(S16)))
         else if (rogue_alu_op_mod_is_set(alu, OM(U8)))
         else if (rogue_alu_op_mod_is_set(alu, OM(S8)))
         else if (rogue_alu_op_mod_is_set(alu, OM(U32)))
         else if (rogue_alu_op_mod_is_set(alu, OM(S32)))
         else
      }
               case ROGUE_ALU_OP_MOVC: {
               bool e0 = rogue_alu_dst_mod_is_set(alu, 0, DM(E0));
   bool e1 = rogue_alu_dst_mod_is_set(alu, 0, DM(E1));
   bool e2 = rogue_alu_dst_mod_is_set(alu, 0, DM(E2));
   bool e3 = rogue_alu_dst_mod_is_set(alu, 0, DM(E3));
            instr_encoding->alu.movc.movw0 = rogue_alu_movc_ft(&alu->src[1].ref);
            if (instr_size == 2) {
      instr_encoding->alu.movc.ext = 1;
   instr_encoding->alu.movc.p2end =
      !rogue_phase_occupied(ROGUE_INSTR_PHASE_2_TST,
         !rogue_phase_occupied(ROGUE_INSTR_PHASE_2_PCK,
               if (e_none) {
         } else {
      instr_encoding->alu.movc.maskw0 |= e0 ? MASKW0_E0 : 0;
   instr_encoding->alu.movc.maskw0 |= e1 ? MASKW0_E1 : 0;
   instr_encoding->alu.movc.maskw0 |= e2 ? MASKW0_E2 : 0;
         }
               case ROGUE_ALU_OP_PCK_U8888:
      instr_encoding->alu.op = ALUOP_SNGL;
   instr_encoding->alu.sngl.snglop = SNGLOP_PCK;
            instr_encoding->alu.sngl.pck.pck.prog = 0;
   instr_encoding->alu.sngl.pck.pck.rtz =
         instr_encoding->alu.sngl.pck.pck.scale =
         instr_encoding->alu.sngl.pck.pck.format = PCK_FMT_U8888;
         case ROGUE_ALU_OP_ADD64:
               instr_encoding->alu.int32_64.int32_64_op = INT32_64_OP_ADD64_NMX;
   instr_encoding->alu.int32_64.s2neg =
                  if (instr_size == 2) {
      instr_encoding->alu.int32_64.ext = 1;
   instr_encoding->alu.int32_64.s2abs =
         instr_encoding->alu.int32_64.s1abs =
         instr_encoding->alu.int32_64.s0abs =
         instr_encoding->alu.int32_64.s0neg =
         instr_encoding->alu.int32_64.s1neg =
         instr_encoding->alu.int32_64.cin =
      }
         default:
            }
   #undef OM
   #undef DM
   #undef SM
      #define OM(op_mod) ROGUE_BACKEND_OP_MOD_##op_mod
   static unsigned rogue_backend_get_cachemode(const rogue_backend_instr *backend)
   {
      if (rogue_backend_op_mod_is_set(backend, OM(BYPASS)))
         else if (rogue_backend_op_mod_is_set(backend, OM(FORCELINEFILL)))
         else if (rogue_backend_op_mod_is_set(backend, OM(WRITETHROUGH)))
         else if (rogue_backend_op_mod_is_set(backend, OM(WRITEBACK)))
         else if (rogue_backend_op_mod_is_set(backend, OM(LAZYWRITEBACK)))
            /* Default cache mode. */
      }
      static unsigned
   rogue_backend_get_slccachemode(const rogue_backend_instr *backend)
   {
      if (rogue_backend_op_mod_is_set(backend, OM(SLCBYPASS)))
         else if (rogue_backend_op_mod_is_set(backend, OM(SLCWRITEBACK)))
         else if (rogue_backend_op_mod_is_set(backend, OM(SLCWRITETHROUGH)))
         else if (rogue_backend_op_mod_is_set(backend, OM(SLCNOALLOC)))
            /* Default SLC cache mode. */
      }
      static void rogue_encode_backend_instr(const rogue_backend_instr *backend,
               {
      switch (backend->op) {
   case ROGUE_BACKEND_OP_FITR_PIXEL:
      instr_encoding->backend.op = BACKENDOP_FITR;
   instr_encoding->backend.fitr.p = 0;
   instr_encoding->backend.fitr.drc =
         instr_encoding->backend.fitr.mode = FITR_MODE_PIXEL;
   instr_encoding->backend.fitr.sat =
         instr_encoding->backend.fitr.count =
               case ROGUE_BACKEND_OP_FITRP_PIXEL:
      instr_encoding->backend.op = BACKENDOP_FITR;
   instr_encoding->backend.fitr.p = 1;
   instr_encoding->backend.fitr.drc =
         instr_encoding->backend.fitr.mode = FITR_MODE_PIXEL;
   instr_encoding->backend.fitr.sat =
         instr_encoding->backend.fitr.count =
               case ROGUE_BACKEND_OP_UVSW_WRITE:
      instr_encoding->backend.op = BACKENDOP_UVSW;
   instr_encoding->backend.uvsw.writeop = UVSW_WRITEOP_WRITE;
   instr_encoding->backend.uvsw.imm = 1;
   instr_encoding->backend.uvsw.imm_src.imm_addr =
               case ROGUE_BACKEND_OP_UVSW_EMIT:
      instr_encoding->backend.op = BACKENDOP_UVSW;
   instr_encoding->backend.uvsw.writeop = UVSW_WRITEOP_EMIT;
         case ROGUE_BACKEND_OP_UVSW_ENDTASK:
      instr_encoding->backend.op = BACKENDOP_UVSW;
   instr_encoding->backend.uvsw.writeop = UVSW_WRITEOP_END;
         case ROGUE_BACKEND_OP_UVSW_EMITTHENENDTASK:
      instr_encoding->backend.op = BACKENDOP_UVSW;
   instr_encoding->backend.uvsw.writeop = UVSW_WRITEOP_EMIT_END;
         case ROGUE_BACKEND_OP_UVSW_WRITETHENEMITTHENENDTASK:
      instr_encoding->backend.op = BACKENDOP_UVSW;
   instr_encoding->backend.uvsw.writeop = UVSW_WRITEOP_WRITE_EMIT_END;
   instr_encoding->backend.uvsw.imm = 1;
   instr_encoding->backend.uvsw.imm_src.imm_addr =
               case ROGUE_BACKEND_OP_LD: {
      instr_encoding->backend.op = BACKENDOP_DMA;
   instr_encoding->backend.dma.dmaop = DMAOP_LD;
   instr_encoding->backend.dma.ld.drc =
         instr_encoding->backend.dma.ld.cachemode =
                     rogue_burstlen burstlen = {
                  if (imm_burstlen) {
         } else {
      instr_encoding->backend.dma.ld.srcselbl =
               instr_encoding->backend.dma.ld.srcseladd =
            if (instr_size == 3) {
      instr_encoding->backend.dma.ld.ext = 1;
                  instr_encoding->backend.dma.ld.slccachemode =
                                 case ROGUE_BACKEND_OP_ST: {
      instr_encoding->backend.op = BACKENDOP_DMA;
   instr_encoding->backend.dma.dmaop = DMAOP_ST;
   instr_encoding->backend.dma.st.drc =
                              if (imm_burstlen) {
      rogue_burstlen burstlen = { ._ = rogue_ref_get_val(
         instr_encoding->backend.dma.st.burstlen_2_0 = burstlen._2_0;
      } else {
      instr_encoding->backend.dma.st.srcselbl =
               instr_encoding->backend.dma.st.cachemode =
         instr_encoding->backend.dma.st.srcseladd =
            instr_encoding->backend.dma.st.dsize =
         instr_encoding->backend.dma.st.srcseldata =
            if (instr_size == 4) {
      instr_encoding->backend.dma.st.ext = 1;
   instr_encoding->backend.dma.st.srcmask =
         instr_encoding->backend.dma.st.slccachemode =
         instr_encoding->backend.dma.st.nottiled =
                           case ROGUE_BACKEND_OP_SMP1D:
   case ROGUE_BACKEND_OP_SMP2D:
   case ROGUE_BACKEND_OP_SMP3D:
      instr_encoding->backend.op = BACKENDOP_DMA;
            instr_encoding->backend.dma.smp.drc =
         instr_encoding->backend.dma.smp.fcnorm =
            if (rogue_backend_op_mod_is_set(backend, OM(BIAS)))
         else if (rogue_backend_op_mod_is_set(backend, OM(REPLACE)))
         else if (rogue_backend_op_mod_is_set(backend, OM(GRADIENT)))
         else
            switch (rogue_ref_get_val(&backend->src[5].ref)) {
   case 1:
                  case 2:
                  case 3:
                  case 4:
                  default:
                  switch (backend->op) {
   case ROGUE_BACKEND_OP_SMP1D:
                  case ROGUE_BACKEND_OP_SMP2D:
                  case ROGUE_BACKEND_OP_SMP3D:
                  default:
                  if (instr_size > 2) {
               instr_encoding->backend.dma.smp.tao =
         instr_encoding->backend.dma.smp.soo =
         instr_encoding->backend.dma.smp.sno =
                        if (rogue_backend_op_mod_is_set(backend, OM(DATA)))
         else if (rogue_backend_op_mod_is_set(backend, OM(INFO)))
         else if (rogue_backend_op_mod_is_set(backend, OM(BOTH)))
                        instr_encoding->backend.dma.smp.proj =
         instr_encoding->backend.dma.smp.pplod =
               if (instr_size > 3) {
                                             instr_encoding->backend.dma.smp.swap =
                        instr_encoding->backend.dma.smp.slccachemode =
               if (instr_size > 4) {
               instr_encoding->backend.dma.smp.array =
                     case ROGUE_BACKEND_OP_IDF:
      instr_encoding->backend.op = BACKENDOP_DMA;
   instr_encoding->backend.dma.dmaop = DMAOP_IDF;
   instr_encoding->backend.dma.idf.drc =
         instr_encoding->backend.dma.idf.srcseladd =
               case ROGUE_BACKEND_OP_EMITPIX:
      instr_encoding->backend.op = BACKENDOP_EMIT;
   instr_encoding->backend.emitpix.freep =
               default:
            }
   #undef OM
      #define OM(op_mod) ROGUE_CTRL_OP_MOD_##op_mod
   static void rogue_encode_ctrl_instr(const rogue_ctrl_instr *ctrl,
               {
      /* Only some control instructions have additional bytes. */
   switch (ctrl->op) {
   case ROGUE_CTRL_OP_NOP:
      memset(&instr_encoding->ctrl.nop, 0, sizeof(instr_encoding->ctrl.nop));
         case ROGUE_CTRL_OP_BR:
   case ROGUE_CTRL_OP_BA: {
      bool branch_abs = (ctrl->op == ROGUE_CTRL_OP_BA);
            instr_encoding->ctrl.ba.abs = branch_abs;
   instr_encoding->ctrl.ba.allp =
         instr_encoding->ctrl.ba.anyp =
                  if (branch_abs) {
         } else {
      rogue_instr_group *block_group =
      list_entry(ctrl->target_block->instrs.next,
            offset._ = block_group->size.offset - (ctrl->instr.group->size.offset +
               instr_encoding->ctrl.ba.offset_7_1 = offset._7_1;
   instr_encoding->ctrl.ba.offset_15_8 = offset._15_8;
   instr_encoding->ctrl.ba.offset_23_16 = offset._23_16;
                        default:
            }
   #undef OM
      static void rogue_encode_bitwise_instr(const rogue_bitwise_instr *bitwise,
               {
      switch (bitwise->op) {
   case ROGUE_BITWISE_OP_BYP0: {
      instr_encoding->bitwise.phase0 = 1;
   instr_encoding->bitwise.ph0.shft = SHFT1_BYP;
            rogue_imm32 imm32;
   if (rogue_ref_is_val(&bitwise->src[1].ref))
            if (instr_size > 1) {
      instr_encoding->bitwise.ph0.ext = 1;
   instr_encoding->bitwise.ph0.imm_7_0 = imm32._7_0;
               if (instr_size > 3) {
      instr_encoding->bitwise.ph0.bm = 1;
   instr_encoding->bitwise.ph0.imm_23_16 = imm32._23_16;
                           default:
            }
      static void rogue_encode_instr_group_instrs(rogue_instr_group *group,
         {
               /* Reverse order for encoding. */
   rogue_foreach_phase_in_set_rev (p, group->header.phases) {
      if (!group->size.instrs[p])
                     const rogue_instr *instr = group->instrs[p];
   switch (instr->type) {
   case ROGUE_INSTR_TYPE_ALU:
      rogue_encode_alu_instr(rogue_instr_as_alu(instr),
                     case ROGUE_INSTR_TYPE_BACKEND:
      rogue_encode_backend_instr(rogue_instr_as_backend(instr),
                     case ROGUE_INSTR_TYPE_CTRL:
      rogue_encode_ctrl_instr(rogue_instr_as_ctrl(instr),
                     case ROGUE_INSTR_TYPE_BITWISE:
      rogue_encode_bitwise_instr(rogue_instr_as_bitwise(instr),
                     default:
                        }
      static void rogue_encode_source_map(const rogue_instr_group *group,
               {
      unsigned base = upper_srcs ? 3 : 0;
   unsigned index = upper_srcs ? group->encode_info.upper_src_index
         const rogue_reg_src_info *info = upper_srcs
                                 if (!upper_srcs && rogue_ref_is_io(&io_sel->iss[0])) {
      switch (io_sel->iss[0].io) {
   case ROGUE_IO_S0:
      mux._ = IS0_S0;
      case ROGUE_IO_S3:
      mux._ = IS0_S3;
      case ROGUE_IO_S4:
      mux._ = IS0_S4;
      case ROGUE_IO_S5:
      mux._ = IS0_S5;
      case ROGUE_IO_S1:
      mux._ = IS0_S1;
      case ROGUE_IO_S2:
                  default:
                     rogue_sbA sbA = { 0 };
            if (!rogue_ref_is_null(&io_sel->srcs[base + 0])) {
      sbA._ = rogue_reg_bank_encoding(
                     rogue_sbB sbB = { 0 };
            if (!rogue_ref_is_null(&io_sel->srcs[base + 1])) {
      sbB._ = rogue_reg_bank_encoding(
                     rogue_sbC sbC = { 0 };
            if (!rogue_ref_is_null(&io_sel->srcs[base + 2])) {
      sbC._ = rogue_reg_bank_encoding(
                     /* Byte 0 is common for all encodings. */
   e->sbA_0 = sbA._0;
            switch (info->num_srcs) {
   case 1:
      switch (info->bytes) {
   case 3:
                     e->sA_1.mux_1_0 = mux._1_0;
                                             case 1:
            default:
         }
         case 2:
      e->ext0 = 1;
   e->sel = 1;
   switch (info->bytes) {
   case 4:
                     e->sB_3.sA_10_8 = sA._10_8;
   e->sB_3.mux_2 = mux._2;
   e->sB_3.sbA_2 = sbA._2;
                              case 3:
                     e->mux_1_0 = mux._1_0;
   e->sbA_1 = sbA._1;
   e->sbB_1 = sbB._1;
                              case 2:
      /* Byte 1 */
   e->sbB_0 = sbB._0;
               default:
         }
         case 3:
      e->ext0 = 1;
   e->ext1 = 1;
   switch (info->bytes) {
   case 6:
                                                case 5:
      /* Byte 4 */
                  e->sC_4.sbC_2 = sbC._2;
   e->sC_4.sC_7_6 = sC._7_6;
   e->sC_4.mux_2 = mux._2;
   e->sC_4.sbA_2 = sbA._2;
                              case 4:
      /* Byte 1 */
                                 e->mux_1_0 = mux._1_0;
   e->sbA_1 = sbA._1;
   e->sbB_1 = sbB._1;
                  /* Byte 3 */
   e->sbC_1_0 = sbC._1_0;
               default:
         }
         default:
            }
      static void rogue_encode_dest_map(const rogue_instr_group *group,
         {
      const rogue_reg_dst_info *info =
                  unsigned num_dsts = !rogue_ref_is_null(&io_sel->dsts[0]) +
            switch (num_dsts) {
   case 1: {
      const rogue_ref *dst_ref = !rogue_ref_is_null(&io_sel->dsts[0])
                  rogue_dbN dbN = { ._ = rogue_reg_bank_encoding(
                  switch (info->bytes) {
   case 2:
      e->dN_10_8 = dN._10_8;
                              case 1:
      e->dbN_0 = dbN._0;
               default:
         }
      }
   case 2: {
      rogue_db0 db0 = { ._ = rogue_reg_bank_encoding(
         rogue_d0 d0 = { ._ = rogue_ref_get_reg_index(&io_sel->dsts[0]) };
   rogue_db1 db1 = { ._ = rogue_reg_bank_encoding(
                  switch (info->bytes) {
   case 4:
                                 case 3:
      e->db1_2_1 = db1._2_1;
   e->d1_7_6 = d1._7_6;
                              case 2:
                     e->db1_0 = db1._0;
               default:
                     default:
            }
      static void rogue_encode_iss_map(const rogue_instr_group *group,
         {
               if (rogue_ref_is_io(&io_sel->iss[1]))
      switch (rogue_ref_get_io(&io_sel->iss[1])) {
   case ROGUE_IO_FT0:
      e->is1 = IS1_FT0;
      case ROGUE_IO_FTE:
                  default:
               if (rogue_ref_is_io(&io_sel->iss[2]))
      switch (rogue_ref_get_io(&io_sel->iss[2])) {
   case ROGUE_IO_FT1:
      e->is2 = IS2_FT1;
      case ROGUE_IO_FTE:
                  default:
               if (rogue_ref_is_io(&io_sel->iss[3]))
      switch (rogue_ref_get_io(&io_sel->iss[3])) {
   case ROGUE_IO_FT0:
      e->is3 = IS3_FT0;
      case ROGUE_IO_FT1:
      e->is3 = IS3_FT1;
      case ROGUE_IO_S2:
      e->is3 = IS3_S2;
      case ROGUE_IO_FTE:
                  default:
               if (rogue_ref_is_io(&io_sel->iss[4]))
      switch (rogue_ref_get_io(&io_sel->iss[4])) {
   case ROGUE_IO_FT0:
      e->is4 = IS4_FT0;
      case ROGUE_IO_FT1:
      e->is4 = IS4_FT1;
      case ROGUE_IO_FT2:
      e->is4 = IS4_FT2;
      case ROGUE_IO_FTE:
                  default:
               if (rogue_ref_is_io(&io_sel->iss[5]))
      switch (rogue_ref_get_io(&io_sel->iss[5])) {
   case ROGUE_IO_FT0:
      e->is5 = IS5_FT0;
      case ROGUE_IO_FT1:
      e->is5 = IS5_FT1;
      case ROGUE_IO_FT2:
      e->is5 = IS5_FT2;
      case ROGUE_IO_FTE:
                  default:
         }
      static void rogue_encode_instr_group_io(const rogue_instr_group *group,
         {
      if (group->size.lower_srcs) {
      rogue_source_map_encoding lower_srcs = { 0 };
   rogue_encode_source_map(group, false, &lower_srcs);
               if (group->size.upper_srcs) {
      rogue_source_map_encoding upper_srcs = { 0 };
   rogue_encode_source_map(group, true, &upper_srcs);
               if (group->size.iss) {
      rogue_iss_encoding internal_src_sel = { 0 };
   rogue_encode_iss_map(group, &internal_src_sel);
               if (group->size.dsts) {
      rogue_dest_map_encoding dests = { 0 };
   rogue_encode_dest_map(group, &dests);
         }
      static void rogue_encode_instr_group_padding(const rogue_instr_group *group,
         {
      if (group->size.word_padding)
            if (group->size.align_padding) {
      assert(!(group->size.align_padding % 2));
   unsigned align_words = group->size.align_padding / 2;
   util_dynarray_append(binary, uint8_t, 0xf0 | align_words);
   for (unsigned u = 0; u < group->size.align_padding - 1; ++u)
         }
      static void rogue_encode_instr_group(rogue_instr_group *group,
         {
      rogue_encode_instr_group_header(group, binary);
   rogue_encode_instr_group_instrs(group, binary);
   rogue_encode_instr_group_io(group, binary);
      }
      PUBLIC
   void rogue_encode_shader(rogue_build_ctx *ctx,
               {
      if (!shader->is_grouped)
                     rogue_foreach_instr_group_in_shader (group, shader)
      }
