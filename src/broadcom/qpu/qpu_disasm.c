   /*
   * Copyright © 2016 Broadcom
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <string.h>
   #include <stdio.h>
   #include "util/ralloc.h"
      #include "broadcom/common/v3d_device_info.h"
   #include "qpu_instr.h"
   #include "qpu_disasm.h"
      struct disasm_state {
         const struct v3d_device_info *devinfo;
   char *string;
   };
      static void
   append(struct disasm_state *disasm, const char *fmt, ...)
   {
         va_list args;
   va_start(args, fmt);
   ralloc_vasprintf_rewrite_tail(&disasm->string,
               }
      static void
   pad_to(struct disasm_state *disasm, int n)
   {
         /* FIXME: Do a single append somehow. */
   while (disasm->offset < n)
   }
         static void
   v3d33_qpu_disasm_raddr(struct disasm_state *disasm,
               {
         if (mux == V3D_QPU_MUX_A) {
         } else if (mux == V3D_QPU_MUX_B) {
            if (instr->sig.small_imm_b) {
            uint32_t val;
                              if ((int)val >= -16 && (int)val <= 15)
         else
      } else {
      } else {
         }
      enum v3d_qpu_input_class {
         V3D_QPU_ADD_A,
   V3D_QPU_ADD_B,
   V3D_QPU_MUL_A,
   };
      static void
   v3d71_qpu_disasm_raddr(struct disasm_state *disasm,
                     {
         bool is_small_imm = false;
   switch(input_class) {
   case V3D_QPU_ADD_A:
               case V3D_QPU_ADD_B:
               case V3D_QPU_MUL_A:
               case V3D_QPU_MUL_B:
                        if (is_small_imm) {
            uint32_t val;
   ASSERTED bool ok =
                        if ((int)val >= -16 && (int)val <= 15)
         else
      } else {
         }
      static void
   v3d_qpu_disasm_raddr(struct disasm_state *disasm,
                     {
         if (disasm->devinfo->ver < 71)
         else
   }
      static void
   v3d_qpu_disasm_waddr(struct disasm_state *disasm, uint32_t waddr, bool magic)
   {
         if (!magic) {
                        const char *name = v3d_qpu_magic_waddr_name(disasm->devinfo, waddr);
   if (name)
         else
   }
      static void
   v3d_qpu_disasm_add(struct disasm_state *disasm,
         {
         bool has_dst = v3d_qpu_add_op_has_dst(instr->alu.add.op);
            append(disasm, "%s", v3d_qpu_add_op_name(instr->alu.add.op));
   if (!v3d_qpu_sig_writes_address(disasm->devinfo, &instr->sig))
         append(disasm, "%s", v3d_qpu_pf_name(instr->flags.apf));
                     if (has_dst) {
            v3d_qpu_disasm_waddr(disasm, instr->alu.add.waddr,
               if (num_src >= 1) {
            if (has_dst)
         v3d_qpu_disasm_raddr(disasm, instr, &instr->alu.add.a, V3D_QPU_ADD_A);
               if (num_src >= 2) {
            append(disasm, ", ");
   v3d_qpu_disasm_raddr(disasm, instr, &instr->alu.add.b, V3D_QPU_ADD_B);
      }
      static void
   v3d_qpu_disasm_mul(struct disasm_state *disasm,
         {
         bool has_dst = v3d_qpu_mul_op_has_dst(instr->alu.mul.op);
            pad_to(disasm, 30);
            append(disasm, "%s", v3d_qpu_mul_op_name(instr->alu.mul.op));
   if (!v3d_qpu_sig_writes_address(disasm->devinfo, &instr->sig))
         append(disasm, "%s", v3d_qpu_pf_name(instr->flags.mpf));
            if (instr->alu.mul.op == V3D_QPU_M_NOP)
                     if (has_dst) {
            v3d_qpu_disasm_waddr(disasm, instr->alu.mul.waddr,
               if (num_src >= 1) {
            if (has_dst)
         v3d_qpu_disasm_raddr(disasm, instr, &instr->alu.mul.a, V3D_QPU_MUL_A);
               if (num_src >= 2) {
            append(disasm, ", ");
   v3d_qpu_disasm_raddr(disasm, instr, &instr->alu.mul.b, V3D_QPU_MUL_B);
      }
      static void
   v3d_qpu_disasm_sig_addr(struct disasm_state *disasm,
         {
         if (disasm->devinfo->ver < 41)
            if (!instr->sig_magic)
         else {
            const char *name =
               if (name)
            }
      static void
   v3d_qpu_disasm_sig(struct disasm_state *disasm,
         {
                  if (!sig->thrsw &&
         !sig->ldvary &&
   !sig->ldvpm &&
   !sig->ldtmu &&
   !sig->ldtlb &&
   !sig->ldtlbu &&
   !sig->ldunif &&
   !sig->ldunifrf &&
   !sig->ldunifa &&
   !sig->ldunifarf &&
   !sig->wrtmuc) {
                     if (sig->thrsw)
         if (sig->ldvary) {
               }
   if (sig->ldvpm)
         if (sig->ldtmu) {
               }
   if (sig->ldtlb) {
               }
   if (sig->ldtlbu) {
               }
   if (sig->ldunif)
         if (sig->ldunifrf) {
               }
   if (sig->ldunifa)
         if (sig->ldunifarf) {
               }
   if (sig->wrtmuc)
   }
      static void
   v3d_qpu_disasm_alu(struct disasm_state *disasm,
         {
         v3d_qpu_disasm_add(disasm, instr);
   v3d_qpu_disasm_mul(disasm, instr);
   }
      static void
   v3d_qpu_disasm_branch(struct disasm_state *disasm,
         {
         append(disasm, "b");
   if (instr->branch.ub)
         append(disasm, "%s", v3d_qpu_branch_cond_name(instr->branch.cond));
            switch (instr->branch.bdi) {
   case V3D_QPU_BRANCH_DEST_ABS:
                  case V3D_QPU_BRANCH_DEST_REL:
                  case V3D_QPU_BRANCH_DEST_LINK_REG:
                  case V3D_QPU_BRANCH_DEST_REGFILE:
                        if (instr->branch.ub) {
            switch (instr->branch.bdu) {
                                                                  case V3D_QPU_BRANCH_DEST_REGFILE:
            }
      const char *
   v3d_qpu_decode(const struct v3d_device_info *devinfo,
         {
         struct disasm_state disasm = {
            .string = rzalloc_size(NULL, 1),
               switch (instr->type) {
   case V3D_QPU_INSTR_TYPE_ALU:
                  case V3D_QPU_INSTR_TYPE_BRANCH:
                        }
      /**
   * Returns a string containing the disassembled representation of the QPU
   * instruction.  It is the caller's responsibility to free the return value
   * with ralloc_free().
   */
   const char *
   v3d_qpu_disasm(const struct v3d_device_info *devinfo, uint64_t inst)
   {
         struct v3d_qpu_instr instr;
   bool ok = v3d_qpu_instr_unpack(devinfo, inst, &instr);
            }
      void
   v3d_qpu_dump(const struct v3d_device_info *devinfo,
         {
         const char *decoded = v3d_qpu_decode(devinfo, instr);
   fprintf(stderr, "%s", decoded);
   }
