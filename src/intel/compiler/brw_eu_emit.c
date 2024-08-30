   /*
   Copyright (C) Intel Corp.  2006.  All Rights Reserved.
   Intel funded Tungsten Graphics to
   develop this 3D driver.
      Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:
      The above copyright notice and this permission notice (including the
   next paragraph) shall be included in all copies or substantial
   portions of the Software.
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
      **********************************************************************/
   /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   */
         #include "brw_eu_defines.h"
   #include "brw_eu.h"
      #include "util/ralloc.h"
      /**
   * Prior to Sandybridge, the SEND instruction accepted non-MRF source
   * registers, implicitly moving the operand to a message register.
   *
   * On Sandybridge, this is no longer the case.  This function performs the
   * explicit move; it should be called before emitting a SEND instruction.
   */
   void
   gfx6_resolve_implied_move(struct brw_codegen *p,
      struct brw_reg *src,
      {
      const struct intel_device_info *devinfo = p->devinfo;
   if (devinfo->ver < 6)
            if (src->file == BRW_MESSAGE_REGISTER_FILE)
            if (src->file != BRW_ARCHITECTURE_REGISTER_FILE || src->nr != BRW_ARF_NULL) {
      assert(devinfo->ver < 12);
   brw_push_insn_state(p);
   brw_set_default_exec_size(p, BRW_EXECUTE_8);
   brw_set_default_mask_control(p, BRW_MASK_DISABLE);
   brw_set_default_compression_control(p, BRW_COMPRESSION_NONE);
   brw_MOV(p, retype(brw_message_reg(msg_reg_nr), BRW_REGISTER_TYPE_UD),
   retype(*src, BRW_REGISTER_TYPE_UD));
      }
      }
      static void
   gfx7_convert_mrf_to_grf(struct brw_codegen *p, struct brw_reg *reg)
   {
      /* From the Ivybridge PRM, Volume 4 Part 3, page 218 ("send"):
   * "The send with EOT should use register space R112-R127 for <src>. This is
   *  to enable loading of a new thread into the same slot while the message
   *  with EOT for current thread is pending dispatch."
   *
   * Since we're pretending to have 16 MRFs anyway, we may as well use the
   * registers required for messages with EOT.
   */
   const struct intel_device_info *devinfo = p->devinfo;
   if (devinfo->ver >= 7 && reg->file == BRW_MESSAGE_REGISTER_FILE) {
      reg->file = BRW_GENERAL_REGISTER_FILE;
         }
      void
   brw_set_dest(struct brw_codegen *p, brw_inst *inst, struct brw_reg dest)
   {
               if (dest.file == BRW_MESSAGE_REGISTER_FILE)
         else if (dest.file == BRW_GENERAL_REGISTER_FILE)
            /* The hardware has a restriction where a destination of size Byte with
   * a stride of 1 is only allowed for a packed byte MOV. For any other
   * instruction, the stride must be at least 2, even when the destination
   * is the NULL register.
   */
   if (dest.file == BRW_ARCHITECTURE_REGISTER_FILE &&
      dest.nr == BRW_ARF_NULL &&
   type_sz(dest.type) == 1 &&
   dest.hstride == BRW_HORIZONTAL_STRIDE_1) {
                        if (devinfo->ver >= 12 &&
      (brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SEND ||
   brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SENDC)) {
   assert(dest.file == BRW_GENERAL_REGISTER_FILE ||
         assert(dest.address_mode == BRW_ADDRESS_DIRECT);
   assert(dest.subnr == 0);
   assert(brw_inst_exec_size(devinfo, inst) == BRW_EXECUTE_1 ||
         (dest.hstride == BRW_HORIZONTAL_STRIDE_1 &&
   assert(!dest.negate && !dest.abs);
   brw_inst_set_dst_reg_file(devinfo, inst, dest.file);
         } else if (brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SENDS ||
            assert(devinfo->ver < 12);
   assert(dest.file == BRW_GENERAL_REGISTER_FILE ||
         assert(dest.address_mode == BRW_ADDRESS_DIRECT);
   assert(dest.subnr % 16 == 0);
   assert(dest.hstride == BRW_HORIZONTAL_STRIDE_1 &&
         assert(!dest.negate && !dest.abs);
   brw_inst_set_dst_da_reg_nr(devinfo, inst, dest.nr);
   brw_inst_set_dst_da16_subreg_nr(devinfo, inst, dest.subnr / 16);
      } else {
      brw_inst_set_dst_file_type(devinfo, inst, dest.file, dest.type);
            if (dest.address_mode == BRW_ADDRESS_DIRECT) {
               if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      brw_inst_set_dst_da1_subreg_nr(devinfo, inst, dest.subnr);
   if (dest.hstride == BRW_HORIZONTAL_STRIDE_0)
            } else {
      brw_inst_set_dst_da16_subreg_nr(devinfo, inst, dest.subnr / 16);
   brw_inst_set_da16_writemask(devinfo, inst, dest.writemask);
   if (dest.file == BRW_GENERAL_REGISTER_FILE ||
      dest.file == BRW_MESSAGE_REGISTER_FILE) {
      }
   /* From the Ivybridge PRM, Vol 4, Part 3, Section 5.2.4.1:
   *    Although Dst.HorzStride is a don't care for Align16, HW needs
   *    this to be programmed as "01".
   */
         } else {
               /* These are different sizes in align1 vs align16:
   */
   if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      brw_inst_set_dst_ia1_addr_imm(devinfo, inst,
         if (dest.hstride == BRW_HORIZONTAL_STRIDE_0)
            } else {
      brw_inst_set_dst_ia16_addr_imm(devinfo, inst,
         /* even ignored in da16, still need to set as '01' */
                     /* Generators should set a default exec_size of either 8 (SIMD4x2 or SIMD8)
   * or 16 (SIMD16), as that's normally correct.  However, when dealing with
   * small registers, it can be useful for us to automatically reduce it to
   * match the register size.
   */
   if (p->automatic_exec_sizes) {
      /*
   * In platforms that support fp64 we can emit instructions with a width
   * of 4 that need two SIMD8 registers and an exec_size of 8 or 16. In
   * these cases we need to make sure that these instructions have their
   * exec sizes set properly when they are emitted and we can't rely on
   * this code to fix it.
   */
   bool fix_exec_size;
   if (devinfo->ver >= 6)
         else
            if (fix_exec_size)
         }
      void
   brw_set_src0(struct brw_codegen *p, brw_inst *inst, struct brw_reg reg)
   {
               if (reg.file == BRW_MESSAGE_REGISTER_FILE)
         else if (reg.file == BRW_GENERAL_REGISTER_FILE)
                     if (devinfo->ver >= 6 &&
      (brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SEND ||
   brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SENDC ||
   brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SENDS ||
   brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SENDSC)) {
   /* Any source modifiers or regions will be ignored, since this just
   * identifies the MRF/GRF to start reading the message contents from.
   * Check for some likely failures.
   */
   assert(!reg.negate);
   assert(!reg.abs);
               if (devinfo->ver >= 12 &&
      (brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SEND ||
   brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SENDC)) {
   assert(reg.file != BRW_IMMEDIATE_VALUE);
   assert(reg.address_mode == BRW_ADDRESS_DIRECT);
   assert(reg.subnr == 0);
   assert(has_scalar_region(reg) ||
         (reg.hstride == BRW_HORIZONTAL_STRIDE_1 &&
   assert(!reg.negate && !reg.abs);
   brw_inst_set_send_src0_reg_file(devinfo, inst, reg.file);
         } else if (brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SENDS ||
            assert(reg.file == BRW_GENERAL_REGISTER_FILE);
   assert(reg.address_mode == BRW_ADDRESS_DIRECT);
   assert(reg.subnr % 16 == 0);
   assert(has_scalar_region(reg) ||
         (reg.hstride == BRW_HORIZONTAL_STRIDE_1 &&
   assert(!reg.negate && !reg.abs);
   brw_inst_set_src0_da_reg_nr(devinfo, inst, reg.nr);
      } else {
      brw_inst_set_src0_file_type(devinfo, inst, reg.file, reg.type);
   brw_inst_set_src0_abs(devinfo, inst, reg.abs);
   brw_inst_set_src0_negate(devinfo, inst, reg.negate);
            if (reg.file == BRW_IMMEDIATE_VALUE) {
      if (reg.type == BRW_REGISTER_TYPE_DF ||
      brw_inst_opcode(p->isa, inst) == BRW_OPCODE_DIM)
      else if (reg.type == BRW_REGISTER_TYPE_UQ ||
                              if (devinfo->ver < 12 && type_sz(reg.type) < 8) {
      brw_inst_set_src1_reg_file(devinfo, inst,
         brw_inst_set_src1_reg_hw_type(devinfo, inst,
         } else {
      if (reg.address_mode == BRW_ADDRESS_DIRECT) {
      brw_inst_set_src0_da_reg_nr(devinfo, inst, reg.nr);
   if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
         } else {
                              if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
         } else {
                     if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      if (reg.width == BRW_WIDTH_1 &&
      brw_inst_exec_size(devinfo, inst) == BRW_EXECUTE_1) {
   brw_inst_set_src0_hstride(devinfo, inst, BRW_HORIZONTAL_STRIDE_0);
   brw_inst_set_src0_width(devinfo, inst, BRW_WIDTH_1);
      } else {
      brw_inst_set_src0_hstride(devinfo, inst, reg.hstride);
   brw_inst_set_src0_width(devinfo, inst, reg.width);
         } else {
      brw_inst_set_src0_da16_swiz_x(devinfo, inst,
         brw_inst_set_src0_da16_swiz_y(devinfo, inst,
         brw_inst_set_src0_da16_swiz_z(devinfo, inst,
                        if (reg.vstride == BRW_VERTICAL_STRIDE_8) {
      /* This is an oddity of the fact we're using the same
   * descriptions for registers in align_16 as align_1:
   */
      } else if (devinfo->verx10 == 70 &&
            reg.type == BRW_REGISTER_TYPE_DF &&
   /* From SNB PRM:
   *
   * "For Align16 access mode, only encodings of 0000 and 0011
   *  are allowed. Other codes are reserved."
   *
   * Presumably the DevSNB behavior applies to IVB as well.
   */
      } else {
                     }
         void
   brw_set_src1(struct brw_codegen *p, brw_inst *inst, struct brw_reg reg)
   {
               if (reg.file == BRW_GENERAL_REGISTER_FILE)
            if (brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SENDS ||
      brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SENDSC ||
   (devinfo->ver >= 12 &&
   (brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SEND ||
         assert(reg.file == BRW_GENERAL_REGISTER_FILE ||
         assert(reg.address_mode == BRW_ADDRESS_DIRECT);
   assert(reg.subnr == 0);
   assert(has_scalar_region(reg) ||
         (reg.hstride == BRW_HORIZONTAL_STRIDE_1 &&
   assert(!reg.negate && !reg.abs);
   brw_inst_set_send_src1_reg_nr(devinfo, inst, reg.nr);
      } else {
      /* From the IVB PRM Vol. 4, Pt. 3, Section 3.3.3.5:
   *
   *    "Accumulator registers may be accessed explicitly as src0
   *    operands only."
   */
   assert(reg.file != BRW_ARCHITECTURE_REGISTER_FILE ||
            gfx7_convert_mrf_to_grf(p, &reg);
            brw_inst_set_src1_file_type(devinfo, inst, reg.file, reg.type);
   brw_inst_set_src1_abs(devinfo, inst, reg.abs);
            /* Only src1 can be immediate in two-argument instructions.
   */
            if (reg.file == BRW_IMMEDIATE_VALUE) {
      /* two-argument instructions can only use 32-bit immediates */
   assert(type_sz(reg.type) < 8);
      } else {
      /* This is a hardware restriction, which may or may not be lifted
   * in the future:
   */
                  brw_inst_set_src1_da_reg_nr(devinfo, inst, reg.nr);
   if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
         } else {
                  if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      if (reg.width == BRW_WIDTH_1 &&
      brw_inst_exec_size(devinfo, inst) == BRW_EXECUTE_1) {
   brw_inst_set_src1_hstride(devinfo, inst, BRW_HORIZONTAL_STRIDE_0);
   brw_inst_set_src1_width(devinfo, inst, BRW_WIDTH_1);
      } else {
      brw_inst_set_src1_hstride(devinfo, inst, reg.hstride);
   brw_inst_set_src1_width(devinfo, inst, reg.width);
         } else {
      brw_inst_set_src1_da16_swiz_x(devinfo, inst,
         brw_inst_set_src1_da16_swiz_y(devinfo, inst,
         brw_inst_set_src1_da16_swiz_z(devinfo, inst,
                        if (reg.vstride == BRW_VERTICAL_STRIDE_8) {
      /* This is an oddity of the fact we're using the same
   * descriptions for registers in align_16 as align_1:
   */
      } else if (devinfo->verx10 == 70 &&
            reg.type == BRW_REGISTER_TYPE_DF &&
   /* From SNB PRM:
   *
   * "For Align16 access mode, only encodings of 0000 and 0011
   *  are allowed. Other codes are reserved."
   *
   * Presumably the DevSNB behavior applies to IVB as well.
   */
      } else {
                     }
      /**
   * Specify the descriptor and extended descriptor immediate for a SEND(C)
   * message instruction.
   */
   void
   brw_set_desc_ex(struct brw_codegen *p, brw_inst *inst,
         {
      const struct intel_device_info *devinfo = p->devinfo;
   assert(brw_inst_opcode(p->isa, inst) == BRW_OPCODE_SEND ||
         if (devinfo->ver < 12)
      brw_inst_set_src1_file_type(devinfo, inst,
      brw_inst_set_send_desc(devinfo, inst, desc);
   if (devinfo->ver >= 9)
      }
      static void brw_set_math_message( struct brw_codegen *p,
         brw_inst *inst,
   unsigned function,
   unsigned integer_type,
   bool low_precision,
   {
      const struct intel_device_info *devinfo = p->devinfo;
   unsigned msg_length;
            /* Infer message length from the function */
   switch (function) {
   case BRW_MATH_FUNCTION_POW:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT:
   case BRW_MATH_FUNCTION_INT_DIV_REMAINDER:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER:
      msg_length = 2;
      default:
      msg_length = 1;
               /* Infer response length from the function */
   switch (function) {
   case BRW_MATH_FUNCTION_SINCOS:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER:
      response_length = 2;
      default:
      response_length = 1;
               brw_set_desc(p, inst, brw_message_desc(
            brw_inst_set_sfid(devinfo, inst, BRW_SFID_MATH);
   brw_inst_set_math_msg_function(devinfo, inst, function);
   brw_inst_set_math_msg_signed_int(devinfo, inst, integer_type);
   brw_inst_set_math_msg_precision(devinfo, inst, low_precision);
   brw_inst_set_math_msg_saturate(devinfo, inst, brw_inst_saturate(devinfo, inst));
   brw_inst_set_math_msg_data_type(devinfo, inst, dataType);
      }
         static void brw_set_ff_sync_message(struct brw_codegen *p,
         brw_inst *insn,
   bool allocate,
   unsigned response_length,
   {
               brw_set_desc(p, insn, brw_message_desc(
            brw_inst_set_sfid(devinfo, insn, BRW_SFID_URB);
   brw_inst_set_eot(devinfo, insn, end_of_thread);
   brw_inst_set_urb_opcode(devinfo, insn, 1); /* FF_SYNC */
   brw_inst_set_urb_allocate(devinfo, insn, allocate);
   /* The following fields are not used by FF_SYNC: */
   brw_inst_set_urb_global_offset(devinfo, insn, 0);
   brw_inst_set_urb_swizzle_control(devinfo, insn, 0);
   brw_inst_set_urb_used(devinfo, insn, 0);
      }
      static void brw_set_urb_message( struct brw_codegen *p,
      brw_inst *insn,
         unsigned msg_length,
   unsigned response_length,
   unsigned offset,
      {
               assert(devinfo->ver < 7 || swizzle_control != BRW_URB_SWIZZLE_TRANSPOSE);
   assert(devinfo->ver < 7 || !(flags & BRW_URB_WRITE_ALLOCATE));
            brw_set_desc(p, insn, brw_message_desc(
            brw_inst_set_sfid(devinfo, insn, BRW_SFID_URB);
            if (flags & BRW_URB_WRITE_OWORD) {
      assert(msg_length == 2); /* header + one OWORD of data */
      } else {
                  brw_inst_set_urb_global_offset(devinfo, insn, offset);
            if (devinfo->ver < 8) {
                  if (devinfo->ver < 7) {
      brw_inst_set_urb_allocate(devinfo, insn, !!(flags & BRW_URB_WRITE_ALLOCATE));
      } else {
      brw_inst_set_urb_per_slot_offset(devinfo, insn,
         }
      static void
   gfx7_set_dp_scratch_message(struct brw_codegen *p,
                              brw_inst *inst,
   bool write,
   bool dword,
   bool invalidate_after_read,
      {
      const struct intel_device_info *devinfo = p->devinfo;
   assert(num_regs == 1 || num_regs == 2 || num_regs == 4 ||
         const unsigned block_size = (devinfo->ver >= 8 ? util_logbase2(num_regs) :
            brw_set_desc(p, inst, brw_message_desc(
            brw_inst_set_sfid(devinfo, inst, GFX7_SFID_DATAPORT_DATA_CACHE);
   brw_inst_set_dp_category(devinfo, inst, 1); /* Scratch Block Read/Write msgs */
   brw_inst_set_scratch_read_write(devinfo, inst, write);
   brw_inst_set_scratch_type(devinfo, inst, dword);
   brw_inst_set_scratch_invalidate_after_read(devinfo, inst, invalidate_after_read);
   brw_inst_set_scratch_block_size(devinfo, inst, block_size);
      }
      static void
   brw_inst_set_state(const struct brw_isa_info *isa,
               {
               brw_inst_set_exec_size(devinfo, insn, state->exec_size);
   brw_inst_set_group(devinfo, insn, state->group);
   brw_inst_set_compression(devinfo, insn, state->compressed);
   brw_inst_set_access_mode(devinfo, insn, state->access_mode);
   brw_inst_set_mask_control(devinfo, insn, state->mask_control);
   if (devinfo->ver >= 12)
         brw_inst_set_saturate(devinfo, insn, state->saturate);
   brw_inst_set_pred_control(devinfo, insn, state->predicate);
            if (is_3src(isa, brw_inst_opcode(isa, insn)) &&
      state->access_mode == BRW_ALIGN_16) {
   brw_inst_set_3src_a16_flag_subreg_nr(devinfo, insn, state->flag_subreg % 2);
   if (devinfo->ver >= 7)
      } else {
      brw_inst_set_flag_subreg_nr(devinfo, insn, state->flag_subreg % 2);
   if (devinfo->ver >= 7)
               if (devinfo->ver >= 6)
      }
      static brw_inst *
   brw_append_insns(struct brw_codegen *p, unsigned nr_insn, unsigned align)
   {
      assert(util_is_power_of_two_or_zero(sizeof(brw_inst)));
   assert(util_is_power_of_two_or_zero(align));
   const unsigned align_insn = MAX2(align / sizeof(brw_inst), 1);
   const unsigned start_insn = ALIGN(p->nr_insn, align_insn);
            if (p->store_size < new_nr_insn) {
      p->store_size = util_next_power_of_two(new_nr_insn * sizeof(brw_inst));
               /* Memset any padding due to alignment to 0.  We don't want to be hashing
   * or caching a bunch of random bits we got from a memory allocation.
   */
   if (p->nr_insn < start_insn) {
      memset(&p->store[p->nr_insn], 0,
               assert(p->next_insn_offset == p->nr_insn * sizeof(brw_inst));
   p->nr_insn = new_nr_insn;
               }
      void
   brw_realign(struct brw_codegen *p, unsigned align)
   {
         }
      int
   brw_append_data(struct brw_codegen *p, void *data,
         {
      unsigned nr_insn = DIV_ROUND_UP(size, sizeof(brw_inst));
   void *dst = brw_append_insns(p, nr_insn, align);
            /* If it's not a whole number of instructions, memset the end */
   if (size < nr_insn * sizeof(brw_inst))
               }
      #define next_insn brw_next_insn
   brw_inst *
   brw_next_insn(struct brw_codegen *p, unsigned opcode)
   {
               memset(insn, 0, sizeof(*insn));
            /* Apply the default instruction state */
               }
      void
   brw_add_reloc(struct brw_codegen *p, uint32_t id,
               {
      if (p->num_relocs + 1 > p->reloc_array_size) {
      p->reloc_array_size = MAX2(16, p->reloc_array_size * 2);
   p->relocs = reralloc(p->mem_ctx, p->relocs,
               p->relocs[p->num_relocs++] = (struct brw_shader_reloc) {
      .id = id,
   .type = type,
   .offset = offset,
         }
      static brw_inst *
   brw_alu1(struct brw_codegen *p, unsigned opcode,
         {
      brw_inst *insn = next_insn(p, opcode);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src);
      }
      static brw_inst *
   brw_alu2(struct brw_codegen *p, unsigned opcode,
         {
      /* 64-bit immediates are only supported on 1-src instructions */
   assert(src0.file != BRW_IMMEDIATE_VALUE || type_sz(src0.type) <= 4);
            brw_inst *insn = next_insn(p, opcode);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, src1);
      }
      static int
   get_3src_subreg_nr(struct brw_reg reg)
   {
      /* Normally, SubRegNum is in bytes (0..31).  However, 3-src instructions
   * use 32-bit units (components 0..7).  Since they only support F/D/UD
   * types, this doesn't lose any flexibility, but uses fewer bits.
   */
      }
      static enum gfx10_align1_3src_vertical_stride
   to_3src_align1_vstride(const struct intel_device_info *devinfo,
         {
      switch (vstride) {
   case BRW_VERTICAL_STRIDE_0:
         case BRW_VERTICAL_STRIDE_1:
      assert(devinfo->ver >= 12);
      case BRW_VERTICAL_STRIDE_2:
      assert(devinfo->ver < 12);
      case BRW_VERTICAL_STRIDE_4:
         case BRW_VERTICAL_STRIDE_8:
   case BRW_VERTICAL_STRIDE_16:
         default:
            }
         static enum gfx10_align1_3src_src_horizontal_stride
   to_3src_align1_hstride(enum brw_horizontal_stride hstride)
   {
      switch (hstride) {
   case BRW_HORIZONTAL_STRIDE_0:
         case BRW_HORIZONTAL_STRIDE_1:
         case BRW_HORIZONTAL_STRIDE_2:
         case BRW_HORIZONTAL_STRIDE_4:
         default:
            }
      static brw_inst *
   brw_alu3(struct brw_codegen *p, unsigned opcode, struct brw_reg dest,
         {
      const struct intel_device_info *devinfo = p->devinfo;
                              if (devinfo->ver >= 10)
      assert(!(src0.file == BRW_IMMEDIATE_VALUE &&
         assert(src0.file == BRW_IMMEDIATE_VALUE || src0.nr < 128);
   assert(src1.file != BRW_IMMEDIATE_VALUE && src1.nr < 128);
   assert(src2.file == BRW_IMMEDIATE_VALUE || src2.nr < 128);
   assert(dest.address_mode == BRW_ADDRESS_DIRECT);
   assert(src0.address_mode == BRW_ADDRESS_DIRECT);
   assert(src1.address_mode == BRW_ADDRESS_DIRECT);
            if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      assert(dest.file == BRW_GENERAL_REGISTER_FILE ||
                  if (devinfo->ver >= 12) {
      brw_inst_set_3src_a1_dst_reg_file(devinfo, inst, dest.file);
      } else {
      if (dest.file == BRW_ARCHITECTURE_REGISTER_FILE) {
      brw_inst_set_3src_a1_dst_reg_file(devinfo, inst,
            } else {
      brw_inst_set_3src_a1_dst_reg_file(devinfo, inst,
               }
                     if (brw_reg_type_is_floating_point(dest.type)) {
      brw_inst_set_3src_a1_exec_type(devinfo, inst,
      } else {
      brw_inst_set_3src_a1_exec_type(devinfo, inst,
               brw_inst_set_3src_a1_dst_type(devinfo, inst, dest.type);
   brw_inst_set_3src_a1_src0_type(devinfo, inst, src0.type);
   brw_inst_set_3src_a1_src1_type(devinfo, inst, src1.type);
            if (src0.file == BRW_IMMEDIATE_VALUE) {
         } else {
      brw_inst_set_3src_a1_src0_vstride(
         brw_inst_set_3src_a1_src0_hstride(devinfo, inst,
         brw_inst_set_3src_a1_src0_subreg_nr(devinfo, inst, src0.subnr);
   if (src0.type == BRW_REGISTER_TYPE_NF) {
         } else {
         }
   brw_inst_set_3src_src0_abs(devinfo, inst, src0.abs);
      }
   brw_inst_set_3src_a1_src1_vstride(
         brw_inst_set_3src_a1_src1_hstride(devinfo, inst,
            brw_inst_set_3src_a1_src1_subreg_nr(devinfo, inst, src1.subnr);
   if (src1.file == BRW_ARCHITECTURE_REGISTER_FILE) {
         } else {
         }
   brw_inst_set_3src_src1_abs(devinfo, inst, src1.abs);
            if (src2.file == BRW_IMMEDIATE_VALUE) {
         } else {
      brw_inst_set_3src_a1_src2_hstride(devinfo, inst,
         /* no vstride on src2 */
   brw_inst_set_3src_a1_src2_subreg_nr(devinfo, inst, src2.subnr);
   brw_inst_set_3src_src2_reg_nr(devinfo, inst, src2.nr);
   brw_inst_set_3src_src2_abs(devinfo, inst, src2.abs);
               assert(src0.file == BRW_GENERAL_REGISTER_FILE ||
         src0.file == BRW_IMMEDIATE_VALUE ||
   (src0.file == BRW_ARCHITECTURE_REGISTER_FILE &&
   assert(src1.file == BRW_GENERAL_REGISTER_FILE ||
         (src1.file == BRW_ARCHITECTURE_REGISTER_FILE &&
   assert(src2.file == BRW_GENERAL_REGISTER_FILE ||
            if (devinfo->ver >= 12) {
      if (src0.file == BRW_IMMEDIATE_VALUE) {
         } else {
                           if (src2.file == BRW_IMMEDIATE_VALUE) {
         } else {
            } else {
      brw_inst_set_3src_a1_src0_reg_file(devinfo, inst,
                     brw_inst_set_3src_a1_src1_reg_file(devinfo, inst,
                     brw_inst_set_3src_a1_src2_reg_file(devinfo, inst,
                        } else {
      assert(dest.file == BRW_GENERAL_REGISTER_FILE ||
         assert(dest.type == BRW_REGISTER_TYPE_F  ||
         dest.type == BRW_REGISTER_TYPE_DF ||
   dest.type == BRW_REGISTER_TYPE_D  ||
   dest.type == BRW_REGISTER_TYPE_UD ||
   if (devinfo->ver == 6) {
      brw_inst_set_3src_a16_dst_reg_file(devinfo, inst,
      }
   brw_inst_set_3src_dst_reg_nr(devinfo, inst, dest.nr);
   brw_inst_set_3src_a16_dst_subreg_nr(devinfo, inst, dest.subnr / 4);
            assert(src0.file == BRW_GENERAL_REGISTER_FILE);
   brw_inst_set_3src_a16_src0_swizzle(devinfo, inst, src0.swizzle);
   brw_inst_set_3src_a16_src0_subreg_nr(devinfo, inst, get_3src_subreg_nr(src0));
   brw_inst_set_3src_src0_reg_nr(devinfo, inst, src0.nr);
   brw_inst_set_3src_src0_abs(devinfo, inst, src0.abs);
   brw_inst_set_3src_src0_negate(devinfo, inst, src0.negate);
   brw_inst_set_3src_a16_src0_rep_ctrl(devinfo, inst,
            assert(src1.file == BRW_GENERAL_REGISTER_FILE);
   brw_inst_set_3src_a16_src1_swizzle(devinfo, inst, src1.swizzle);
   brw_inst_set_3src_a16_src1_subreg_nr(devinfo, inst, get_3src_subreg_nr(src1));
   brw_inst_set_3src_src1_reg_nr(devinfo, inst, src1.nr);
   brw_inst_set_3src_src1_abs(devinfo, inst, src1.abs);
   brw_inst_set_3src_src1_negate(devinfo, inst, src1.negate);
   brw_inst_set_3src_a16_src1_rep_ctrl(devinfo, inst,
            assert(src2.file == BRW_GENERAL_REGISTER_FILE);
   brw_inst_set_3src_a16_src2_swizzle(devinfo, inst, src2.swizzle);
   brw_inst_set_3src_a16_src2_subreg_nr(devinfo, inst, get_3src_subreg_nr(src2));
   brw_inst_set_3src_src2_reg_nr(devinfo, inst, src2.nr);
   brw_inst_set_3src_src2_abs(devinfo, inst, src2.abs);
   brw_inst_set_3src_src2_negate(devinfo, inst, src2.negate);
   brw_inst_set_3src_a16_src2_rep_ctrl(devinfo, inst,
            if (devinfo->ver >= 7) {
      /* Set both the source and destination types based on dest.type,
   * ignoring the source register types.  The MAD and LRP emitters ensure
   * that all four types are float.  The BFE and BFI2 emitters, however,
   * may send us mixed D and UD types and want us to ignore that and use
   * the destination type.
   */
                  /* From the Bspec, 3D Media GPGPU, Instruction fields, srcType:
   *
   *    "Three source instructions can use operands with mixed-mode
   *     precision. When SrcType field is set to :f or :hf it defines
   *     precision for source 0 only, and fields Src1Type and Src2Type
   *     define precision for other source operands:
   *
   *     0b = :f. Single precision Float (32-bit).
   *     1b = :hf. Half precision Float (16-bit)."
   */
                  if (src2.type == BRW_REGISTER_TYPE_HF)
                     }
         /***********************************************************************
   * Convenience routines.
   */
   #define ALU1(OP)					\
   brw_inst *brw_##OP(struct brw_codegen *p,		\
         struct brw_reg dest,			\
   {							\
         }
      #define ALU2(OP)					\
   brw_inst *brw_##OP(struct brw_codegen *p,		\
         struct brw_reg dest,			\
   struct brw_reg src0,			\
   {							\
         }
      #define ALU3(OP)					\
   brw_inst *brw_##OP(struct brw_codegen *p,		\
         struct brw_reg dest,			\
   struct brw_reg src0,			\
   struct brw_reg src1,			\
   {                                                       \
      if (p->current->access_mode == BRW_ALIGN_16) {       \
      if (src0.vstride == BRW_VERTICAL_STRIDE_0)        \
         if (src1.vstride == BRW_VERTICAL_STRIDE_0)        \
         if (src2.vstride == BRW_VERTICAL_STRIDE_0)        \
      }                                                    \
      }
      #define ALU3F(OP)                                               \
   brw_inst *brw_##OP(struct brw_codegen *p,         \
                           {                                                               \
      assert(dest.type == BRW_REGISTER_TYPE_F ||                   \
         if (dest.type == BRW_REGISTER_TYPE_F) {                      \
      assert(src0.type == BRW_REGISTER_TYPE_F);                 \
   assert(src1.type == BRW_REGISTER_TYPE_F);                 \
      } else if (dest.type == BRW_REGISTER_TYPE_DF) {              \
      assert(src0.type == BRW_REGISTER_TYPE_DF);                \
   assert(src1.type == BRW_REGISTER_TYPE_DF);                \
      }                                                            \
         if (p->current->access_mode == BRW_ALIGN_16) {               \
      if (src0.vstride == BRW_VERTICAL_STRIDE_0)                \
         if (src1.vstride == BRW_VERTICAL_STRIDE_0)                \
         if (src2.vstride == BRW_VERTICAL_STRIDE_0)                \
      }                                                            \
      }
      ALU2(SEL)
   ALU1(NOT)
   ALU2(AND)
   ALU2(OR)
   ALU2(XOR)
   ALU2(SHR)
   ALU2(SHL)
   ALU1(DIM)
   ALU2(ASR)
   ALU2(ROL)
   ALU2(ROR)
   ALU3(CSEL)
   ALU1(FRC)
   ALU1(RNDD)
   ALU1(RNDE)
   ALU1(RNDU)
   ALU1(RNDZ)
   ALU2(MAC)
   ALU2(MACH)
   ALU1(LZD)
   ALU2(DP4)
   ALU2(DPH)
   ALU2(DP3)
   ALU2(DP2)
   ALU3(DP4A)
   ALU3(MAD)
   ALU3F(LRP)
   ALU1(BFREV)
   ALU3(BFE)
   ALU2(BFI1)
   ALU3(BFI2)
   ALU1(FBH)
   ALU1(FBL)
   ALU1(CBIT)
   ALU2(ADDC)
   ALU2(SUBB)
   ALU3(ADD3)
      brw_inst *
   brw_MOV(struct brw_codegen *p, struct brw_reg dest, struct brw_reg src0)
   {
               /* When converting F->DF on IVB/BYT, every odd source channel is ignored.
   * To avoid the problems that causes, we use an <X,2,0> source region to
   * read each element twice.
   */
   if (devinfo->verx10 == 70 &&
      brw_get_default_access_mode(p) == BRW_ALIGN_1 &&
   dest.type == BRW_REGISTER_TYPE_DF &&
   (src0.type == BRW_REGISTER_TYPE_F ||
   src0.type == BRW_REGISTER_TYPE_D ||
   src0.type == BRW_REGISTER_TYPE_UD) &&
   !has_scalar_region(src0)) {
   assert(src0.vstride == src0.width + src0.hstride);
   src0.vstride = src0.hstride;
   src0.width = BRW_WIDTH_2;
                  }
      brw_inst *
   brw_ADD(struct brw_codegen *p, struct brw_reg dest,
         {
      /* 6.2.2: add */
   if (src0.type == BRW_REGISTER_TYPE_F ||
      src0.type == BRW_REGISTER_TYPE_VF)) {
         assert(src1.type != BRW_REGISTER_TYPE_UD);
               if (src1.type == BRW_REGISTER_TYPE_F ||
      src1.type == BRW_REGISTER_TYPE_VF)) {
         assert(src0.type != BRW_REGISTER_TYPE_UD);
                  }
      brw_inst *
   brw_AVG(struct brw_codegen *p, struct brw_reg dest,
         {
      assert(dest.type == src0.type);
   assert(src0.type == src1.type);
   switch (src0.type) {
   case BRW_REGISTER_TYPE_B:
   case BRW_REGISTER_TYPE_UB:
   case BRW_REGISTER_TYPE_W:
   case BRW_REGISTER_TYPE_UW:
   case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_UD:
         default:
                     }
      brw_inst *
   brw_MUL(struct brw_codegen *p, struct brw_reg dest,
         {
      /* 6.32.38: mul */
   if (src0.type == BRW_REGISTER_TYPE_D ||
      src0.type == BRW_REGISTER_TYPE_UD ||
   src1.type == BRW_REGISTER_TYPE_D ||
   src1.type == BRW_REGISTER_TYPE_UD) {
               if (src0.type == BRW_REGISTER_TYPE_F ||
      src0.type == BRW_REGISTER_TYPE_VF)) {
         assert(src1.type != BRW_REGISTER_TYPE_UD);
               if (src1.type == BRW_REGISTER_TYPE_F ||
      src1.type == BRW_REGISTER_TYPE_VF)) {
         assert(src0.type != BRW_REGISTER_TYPE_UD);
               assert(src0.file != BRW_ARCHITECTURE_REGISTER_FILE ||
   src0.nr != BRW_ARF_ACCUMULATOR);
   assert(src1.file != BRW_ARCHITECTURE_REGISTER_FILE ||
               }
      brw_inst *
   brw_LINE(struct brw_codegen *p, struct brw_reg dest,
         {
      src0.vstride = BRW_VERTICAL_STRIDE_0;
   src0.width = BRW_WIDTH_1;
   src0.hstride = BRW_HORIZONTAL_STRIDE_0;
      }
      brw_inst *
   brw_PLN(struct brw_codegen *p, struct brw_reg dest,
         {
      src0.vstride = BRW_VERTICAL_STRIDE_0;
   src0.width = BRW_WIDTH_1;
   src0.hstride = BRW_HORIZONTAL_STRIDE_0;
   src1.vstride = BRW_VERTICAL_STRIDE_8;
   src1.width = BRW_WIDTH_8;
   src1.hstride = BRW_HORIZONTAL_STRIDE_1;
      }
      brw_inst *
   brw_F32TO16(struct brw_codegen *p, struct brw_reg dst, struct brw_reg src)
   {
               /* The F32TO16 instruction doesn't support 32-bit destination types in
   * Align1 mode.  Gfx7 (only) does zero out the high 16 bits in Align16
   * mode as an undocumented feature.
   */
   if (BRW_ALIGN_16 == brw_get_default_access_mode(p)) {
         } else {
      assert(dst.type == BRW_REGISTER_TYPE_W ||
                  }
      brw_inst *
   brw_F16TO32(struct brw_codegen *p, struct brw_reg dst, struct brw_reg src)
   {
               if (BRW_ALIGN_16 == brw_get_default_access_mode(p)) {
         } else {
      /* From the Ivybridge PRM, Vol4, Part3, Section 6.26 f16to32:
   *
   *   Because this instruction does not have a 16-bit floating-point
   *   type, the source data type must be Word (W). The destination type
   *   must be F (Float).
   */
   assert(src.type == BRW_REGISTER_TYPE_W ||
                  }
         void brw_NOP(struct brw_codegen *p)
   {
      brw_inst *insn = next_insn(p, BRW_OPCODE_NOP);
   memset(insn, 0, sizeof(*insn));
      }
      void brw_SYNC(struct brw_codegen *p, enum tgl_sync_function func)
   {
      brw_inst *insn = next_insn(p, BRW_OPCODE_SYNC);
      }
      /***********************************************************************
   * Comparisons, if/else/endif
   */
      brw_inst *
   brw_JMPI(struct brw_codegen *p, struct brw_reg index,
         {
      const struct intel_device_info *devinfo = p->devinfo;
   struct brw_reg ip = brw_ip_reg();
            brw_inst_set_exec_size(devinfo, inst, BRW_EXECUTE_1);
   brw_inst_set_qtr_control(devinfo, inst, BRW_COMPRESSION_NONE);
   brw_inst_set_mask_control(devinfo, inst, BRW_MASK_DISABLE);
               }
      static void
   push_if_stack(struct brw_codegen *p, brw_inst *inst)
   {
               p->if_stack_depth++;
   if (p->if_stack_array_size <= p->if_stack_depth) {
      p->if_stack_array_size *= 2;
   p->if_stack = reralloc(p->mem_ctx, p->if_stack, int,
         }
      static brw_inst *
   pop_if_stack(struct brw_codegen *p)
   {
      p->if_stack_depth--;
      }
      static void
   push_loop_stack(struct brw_codegen *p, brw_inst *inst)
   {
      if (p->loop_stack_array_size <= (p->loop_stack_depth + 1)) {
      p->loop_stack_array_size *= 2;
   p->loop_stack = reralloc(p->mem_ctx, p->loop_stack, int,
         p->if_depth_in_loop = reralloc(p->mem_ctx, p->if_depth_in_loop, int,
               p->loop_stack[p->loop_stack_depth] = inst - p->store;
   p->loop_stack_depth++;
      }
      static brw_inst *
   get_inner_do_insn(struct brw_codegen *p)
   {
         }
      /* EU takes the value from the flag register and pushes it onto some
   * sort of a stack (presumably merging with any flag value already on
   * the stack).  Within an if block, the flags at the top of the stack
   * control execution on each channel of the unit, eg. on each of the
   * 16 pixel values in our wm programs.
   *
   * When the matching 'else' instruction is reached (presumably by
   * countdown of the instruction count patched in by our ELSE/ENDIF
   * functions), the relevant flags are inverted.
   *
   * When the matching 'endif' instruction is reached, the flags are
   * popped off.  If the stack is now empty, normal execution resumes.
   */
   brw_inst *
   brw_IF(struct brw_codegen *p, unsigned execute_size)
   {
      const struct intel_device_info *devinfo = p->devinfo;
                     /* Override the defaults for this instruction:
   */
   if (devinfo->ver < 6) {
      brw_set_dest(p, insn, brw_ip_reg());
   brw_set_src0(p, insn, brw_ip_reg());
      } else if (devinfo->ver == 6) {
      brw_set_dest(p, insn, brw_imm_w(0));
   brw_inst_set_gfx6_jump_count(devinfo, insn, 0);
   brw_set_src0(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
      } else if (devinfo->ver == 7) {
      brw_set_dest(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
   brw_set_src0(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
   brw_set_src1(p, insn, brw_imm_w(0));
   brw_inst_set_jip(devinfo, insn, 0);
      } else {
      brw_set_dest(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
   if (devinfo->ver < 12)
         brw_inst_set_jip(devinfo, insn, 0);
               brw_inst_set_exec_size(devinfo, insn, execute_size);
   brw_inst_set_qtr_control(devinfo, insn, BRW_COMPRESSION_NONE);
   brw_inst_set_pred_control(devinfo, insn, BRW_PREDICATE_NORMAL);
   brw_inst_set_mask_control(devinfo, insn, BRW_MASK_ENABLE);
   if (!p->single_program_flow && devinfo->ver < 6)
            push_if_stack(p, insn);
   p->if_depth_in_loop[p->loop_stack_depth]++;
      }
      /* This function is only used for gfx6-style IF instructions with an
   * embedded comparison (conditional modifier).  It is not used on gfx7.
   */
   brw_inst *
   gfx6_IF(struct brw_codegen *p, enum brw_conditional_mod conditional,
   struct brw_reg src0, struct brw_reg src1)
   {
      const struct intel_device_info *devinfo = p->devinfo;
                     brw_set_dest(p, insn, brw_imm_w(0));
   brw_inst_set_exec_size(devinfo, insn, brw_get_default_exec_size(p));
   brw_inst_set_gfx6_jump_count(devinfo, insn, 0);
   brw_set_src0(p, insn, src0);
            assert(brw_inst_qtr_control(devinfo, insn) == BRW_COMPRESSION_NONE);
   assert(brw_inst_pred_control(devinfo, insn) == BRW_PREDICATE_NONE);
            push_if_stack(p, insn);
      }
      /**
   * In single-program-flow (SPF) mode, convert IF and ELSE into ADDs.
   */
   static void
   convert_IF_ELSE_to_ADD(struct brw_codegen *p,
         {
               /* The next instruction (where the ENDIF would be, if it existed) */
            assert(p->single_program_flow);
   assert(if_inst != NULL && brw_inst_opcode(p->isa, if_inst) == BRW_OPCODE_IF);
   assert(else_inst == NULL || brw_inst_opcode(p->isa, else_inst) == BRW_OPCODE_ELSE);
            /* Convert IF to an ADD instruction that moves the instruction pointer
   * to the first instruction of the ELSE block.  If there is no ELSE
   * block, point to where ENDIF would be.  Reverse the predicate.
   *
   * There's no need to execute an ENDIF since we don't need to do any
   * stack operations, and if we're currently executing, we just want to
   * continue normally.
   */
   brw_inst_set_opcode(p->isa, if_inst, BRW_OPCODE_ADD);
            if (else_inst != NULL) {
      /* Convert ELSE to an ADD instruction that points where the ENDIF
   * would be.
   */
            brw_inst_set_imm_ud(devinfo, if_inst, (else_inst - if_inst + 1) * 16);
      } else {
            }
      /**
   * Patch IF and ELSE instructions with appropriate jump targets.
   */
   static void
   patch_IF_ELSE(struct brw_codegen *p,
         {
               /* We shouldn't be patching IF and ELSE instructions in single program flow
   * mode when gen < 6, because in single program flow mode on those
   * platforms, we convert flow control instructions to conditional ADDs that
   * operate on IP (see brw_ENDIF).
   *
   * However, on Gfx6, writing to IP doesn't work in single program flow mode
   * (see the SandyBridge PRM, Volume 4 part 2, p79: "When SPF is ON, IP may
   * not be updated by non-flow control instructions.").  And on later
   * platforms, there is no significant benefit to converting control flow
   * instructions to conditional ADDs.  So we do patch IF and ELSE
   * instructions in single program flow mode on those platforms.
   */
   if (devinfo->ver < 6)
            assert(if_inst != NULL && brw_inst_opcode(p->isa, if_inst) == BRW_OPCODE_IF);
   assert(endif_inst != NULL);
                     assert(brw_inst_opcode(p->isa, endif_inst) == BRW_OPCODE_ENDIF);
            if (else_inst == NULL) {
      /* Patch IF -> ENDIF */
   /* Turn it into an IFF, which means no mask stack operations for
      * all-false and jumping past the ENDIF.
   */
         brw_inst_set_opcode(p->isa, if_inst, BRW_OPCODE_IFF);
   brw_inst_set_gfx4_jump_count(devinfo, if_inst,
            /* As of gfx6, there is no IFF and IF must point to the ENDIF. */
               } else {
      brw_inst_set_uip(devinfo, if_inst, br * (endif_inst - if_inst));
         } else {
               /* Patch IF -> ELSE */
   if (devinfo->ver < 6) {
      brw_inst_set_gfx4_jump_count(devinfo, if_inst,
            } else if (devinfo->ver == 6) {
      brw_inst_set_gfx6_jump_count(devinfo, if_inst,
               /* Patch ELSE -> ENDIF */
   /* BRW_OPCODE_ELSE pre-gfx6 should point just past the
      * matching ENDIF.
   */
         brw_inst_set_gfx4_jump_count(devinfo, else_inst,
            /* BRW_OPCODE_ELSE on gfx6 should point to the matching ENDIF. */
            brw_inst_set_gfx6_jump_count(devinfo, else_inst,
      /* The IF instruction's JIP should point just past the ELSE */
         /* The IF instruction's UIP and ELSE's JIP should point to ENDIF */
                     if (devinfo->ver >= 8 && devinfo->ver < 11) {
      /* Set the ELSE instruction to use branch_ctrl with a join
   * jump target pointing at the NOP inserted right before
   * the ENDIF instruction in order to make sure it is
   * executed in all cases, since attempting to do the same
   * as on other generations could cause the EU to jump at
   * the instruction immediately after the ENDIF due to
   * Wa_220160235, which could cause the program to continue
   * running with all channels disabled.
   */
   brw_inst_set_jip(devinfo, else_inst, br * (endif_inst - else_inst - 1));
      } else {
                  if (devinfo->ver >= 8) {
      /* Since we don't set branch_ctrl on Gfx11+, the ELSE's
   * JIP and UIP both should point to ENDIF on those
   * platforms.
   */
               }
      void
   brw_ELSE(struct brw_codegen *p)
   {
      const struct intel_device_info *devinfo = p->devinfo;
                     if (devinfo->ver < 6) {
      brw_set_dest(p, insn, brw_ip_reg());
   brw_set_src0(p, insn, brw_ip_reg());
      } else if (devinfo->ver == 6) {
      brw_set_dest(p, insn, brw_imm_w(0));
   brw_inst_set_gfx6_jump_count(devinfo, insn, 0);
   brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      } else if (devinfo->ver == 7) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   brw_set_src1(p, insn, brw_imm_w(0));
   brw_inst_set_jip(devinfo, insn, 0);
      } else {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   if (devinfo->ver < 12)
         brw_inst_set_jip(devinfo, insn, 0);
               brw_inst_set_qtr_control(devinfo, insn, BRW_COMPRESSION_NONE);
   brw_inst_set_mask_control(devinfo, insn, BRW_MASK_ENABLE);
   if (!p->single_program_flow && devinfo->ver < 6)
               }
      void
   brw_ENDIF(struct brw_codegen *p)
   {
      const struct intel_device_info *devinfo = p->devinfo;
   brw_inst *insn = NULL;
   brw_inst *else_inst = NULL;
   brw_inst *if_inst = NULL;
   brw_inst *tmp;
                     if (devinfo->ver >= 8 && devinfo->ver < 11 &&
      brw_inst_opcode(p->isa, &p->store[p->if_stack[
         /* Insert a NOP to be specified as join instruction within the
   * ELSE block, which is valid for an ELSE instruction with
   * branch_ctrl on.  The ELSE instruction will be set to jump
   * here instead of to the ENDIF instruction, since attempting to
   * do the latter would prevent the ENDIF from being executed in
   * some cases due to Wa_220160235, which could cause the program
   * to continue running with all channels disabled.
   */
               /* In single program flow mode, we can express IF and ELSE instructions
   * equivalently as ADD instructions that operate on IP.  On platforms prior
   * to Gfx6, flow control instructions cause an implied thread switch, so
   * this is a significant savings.
   *
   * However, on Gfx6, writing to IP doesn't work in single program flow mode
   * (see the SandyBridge PRM, Volume 4 part 2, p79: "When SPF is ON, IP may
   * not be updated by non-flow control instructions.").  And on later
   * platforms, there is no significant benefit to converting control flow
   * instructions to conditional ADDs.  So we only do this trick on Gfx4 and
   * Gfx5.
   */
   if (devinfo->ver < 6 && p->single_program_flow)
            /*
   * A single next_insn() may change the base address of instruction store
   * memory(p->store), so call it first before referencing the instruction
   * store pointer from an index
   */
   if (emit_endif)
            /* Pop the IF and (optional) ELSE instructions from the stack */
   p->if_depth_in_loop[p->loop_stack_depth]--;
   tmp = pop_if_stack(p);
   if (brw_inst_opcode(p->isa, tmp) == BRW_OPCODE_ELSE) {
      else_inst = tmp;
      }
            if (!emit_endif) {
      /* ENDIF is useless; don't bother emitting it. */
   convert_IF_ELSE_to_ADD(p, if_inst, else_inst);
               if (devinfo->ver < 6) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      } else if (devinfo->ver == 6) {
      brw_set_dest(p, insn, brw_imm_w(0));
   brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      } else if (devinfo->ver == 7) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      } else {
                  brw_inst_set_qtr_control(devinfo, insn, BRW_COMPRESSION_NONE);
   brw_inst_set_mask_control(devinfo, insn, BRW_MASK_ENABLE);
   if (devinfo->ver < 6)
            /* Also pop item off the stack in the endif instruction: */
   if (devinfo->ver < 6) {
      brw_inst_set_gfx4_jump_count(devinfo, insn, 0);
      } else if (devinfo->ver == 6) {
         } else {
         }
      }
      brw_inst *
   brw_BREAK(struct brw_codegen *p)
   {
      const struct intel_device_info *devinfo = p->devinfo;
            insn = next_insn(p, BRW_OPCODE_BREAK);
   if (devinfo->ver >= 8) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      } else if (devinfo->ver >= 6) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      } else {
      brw_set_dest(p, insn, brw_ip_reg());
   brw_set_src0(p, insn, brw_ip_reg());
   brw_set_src1(p, insn, brw_imm_d(0x0));
   brw_inst_set_gfx4_pop_count(devinfo, insn,
      }
   brw_inst_set_qtr_control(devinfo, insn, BRW_COMPRESSION_NONE);
               }
      brw_inst *
   brw_CONT(struct brw_codegen *p)
   {
      const struct intel_device_info *devinfo = p->devinfo;
            insn = next_insn(p, BRW_OPCODE_CONTINUE);
   brw_set_dest(p, insn, brw_ip_reg());
   if (devinfo->ver >= 8) {
         } else {
      brw_set_src0(p, insn, brw_ip_reg());
               if (devinfo->ver < 6) {
      brw_inst_set_gfx4_pop_count(devinfo, insn,
      }
   brw_inst_set_qtr_control(devinfo, insn, BRW_COMPRESSION_NONE);
   brw_inst_set_exec_size(devinfo, insn, brw_get_default_exec_size(p));
      }
      brw_inst *
   brw_HALT(struct brw_codegen *p)
   {
      const struct intel_device_info *devinfo = p->devinfo;
            insn = next_insn(p, BRW_OPCODE_HALT);
   brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   if (devinfo->ver < 6) {
      /* From the Gfx4 PRM:
   *
   *    "IP register must be put (for example, by the assembler) at <dst>
   *    and <src0> locations.
   */
   brw_set_dest(p, insn, brw_ip_reg());
   brw_set_src0(p, insn, brw_ip_reg());
      } else if (devinfo->ver < 8) {
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      } else if (devinfo->ver < 12) {
                  brw_inst_set_qtr_control(devinfo, insn, BRW_COMPRESSION_NONE);
   brw_inst_set_exec_size(devinfo, insn, brw_get_default_exec_size(p));
      }
      /* DO/WHILE loop:
   *
   * The DO/WHILE is just an unterminated loop -- break or continue are
   * used for control within the loop.  We have a few ways they can be
   * done.
   *
   * For uniform control flow, the WHILE is just a jump, so ADD ip, ip,
   * jip and no DO instruction.
   *
   * For non-uniform control flow pre-gfx6, there's a DO instruction to
   * push the mask, and a WHILE to jump back, and BREAK to get out and
   * pop the mask.
   *
   * For gfx6, there's no more mask stack, so no need for DO.  WHILE
   * just points back to the first instruction of the loop.
   */
   brw_inst *
   brw_DO(struct brw_codegen *p, unsigned execute_size)
   {
               if (devinfo->ver >= 6 || p->single_program_flow) {
      push_loop_stack(p, &p->store[p->nr_insn]);
      } else {
                        /* Override the defaults for this instruction:
   */
   brw_set_dest(p, insn, brw_null_reg());
   brw_set_src0(p, insn, brw_null_reg());
            brw_inst_set_qtr_control(devinfo, insn, BRW_COMPRESSION_NONE);
   brw_inst_set_exec_size(devinfo, insn, execute_size);
                  }
      /**
   * For pre-gfx6, we patch BREAK/CONT instructions to point at the WHILE
   * instruction here.
   *
   * For gfx6+, see brw_set_uip_jip(), which doesn't care so much about the loop
   * nesting, since it can always just point to the end of the block/current loop.
   */
   static void
   brw_patch_break_cont(struct brw_codegen *p, brw_inst *while_inst)
   {
      const struct intel_device_info *devinfo = p->devinfo;
   brw_inst *do_inst = get_inner_do_insn(p);
   brw_inst *inst;
                     for (inst = while_inst - 1; inst != do_inst; inst--) {
      /* If the jump count is != 0, that means that this instruction has already
   * been patched because it's part of a loop inside of the one we're
   * patching.
   */
   if (brw_inst_opcode(p->isa, inst) == BRW_OPCODE_BREAK &&
      brw_inst_gfx4_jump_count(devinfo, inst) == 0) {
      } else if (brw_inst_opcode(p->isa, inst) == BRW_OPCODE_CONTINUE &&
                     }
      brw_inst *
   brw_WHILE(struct brw_codegen *p)
   {
      const struct intel_device_info *devinfo = p->devinfo;
   brw_inst *insn, *do_insn;
            if (devinfo->ver >= 6) {
      insn = next_insn(p, BRW_OPCODE_WHILE);
            if (devinfo->ver >= 8) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   if (devinfo->ver < 12)
            } else if (devinfo->ver == 7) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   brw_set_src1(p, insn, brw_imm_w(0));
      } else {
      brw_set_dest(p, insn, brw_imm_w(0));
   brw_inst_set_gfx6_jump_count(devinfo, insn, br * (do_insn - insn));
   brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
                     } else {
      insn = next_insn(p, BRW_OPCODE_ADD);
            brw_set_dest(p, insn, brw_ip_reg());
   brw_set_src0(p, insn, brw_ip_reg());
   brw_set_src1(p, insn, brw_imm_d((do_insn - insn) * 16));
               insn = next_insn(p, BRW_OPCODE_WHILE);
                     brw_set_dest(p, insn, brw_ip_reg());
   brw_set_src0(p, insn, brw_ip_reg());
   brw_set_src1(p, insn, brw_imm_d(0));
               brw_inst_set_exec_size(devinfo, insn, brw_inst_exec_size(devinfo, do_insn));
         brw_patch_break_cont(p, insn);
            }
                        }
      /* FORWARD JUMPS:
   */
   void brw_land_fwd_jump(struct brw_codegen *p, int jmp_insn_idx)
   {
      const struct intel_device_info *devinfo = p->devinfo;
   brw_inst *jmp_insn = &p->store[jmp_insn_idx];
            if (devinfo->ver >= 5)
            assert(brw_inst_opcode(p->isa, jmp_insn) == BRW_OPCODE_JMPI);
            brw_inst_set_gfx4_jump_count(devinfo, jmp_insn,
      }
      /* To integrate with the above, it makes sense that the comparison
   * instruction should populate the flag register.  It might be simpler
   * just to use the flag reg for most WM tasks?
   */
   void brw_CMP(struct brw_codegen *p,
         struct brw_reg dest,
   unsigned conditional,
   struct brw_reg src0,
   {
      const struct intel_device_info *devinfo = p->devinfo;
            brw_inst_set_cond_modifier(devinfo, insn, conditional);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
            /* Item WaCMPInstNullDstForcesThreadSwitch in the Haswell Bspec workarounds
   * page says:
   *    "Any CMP instruction with a null destination must use a {switch}."
   *
   * It also applies to other Gfx7 platforms (IVB, BYT) even though it isn't
   * mentioned on their work-arounds pages.
   */
   if (devinfo->ver == 7) {
      if (dest.file == BRW_ARCHITECTURE_REGISTER_FILE &&
      dest.nr == BRW_ARF_NULL) {
            }
      void brw_CMPN(struct brw_codegen *p,
               struct brw_reg dest,
   unsigned conditional,
   {
      const struct intel_device_info *devinfo = p->devinfo;
            brw_inst_set_cond_modifier(devinfo, insn, conditional);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
            /* Page 166 of the Ivy Bridge PRM Volume 4 part 3 (Execution Unit ISA)
   * says:
   *
   *    If the destination is the null register, the {Switch} instruction
   *    option must be used.
   *
   * Page 77 of the Haswell PRM Volume 2b contains the same text.
   */
   if (devinfo->ver == 7) {
      if (dest.file == BRW_ARCHITECTURE_REGISTER_FILE &&
      dest.nr == BRW_ARF_NULL) {
            }
      /***********************************************************************
   * Helpers for the various SEND message types:
   */
      /** Extended math function, float[8].
   */
   void gfx4_math(struct brw_codegen *p,
         struct brw_reg dest,
   unsigned function,
   unsigned msg_reg_nr,
   struct brw_reg src,
   {
      const struct intel_device_info *devinfo = p->devinfo;
   brw_inst *insn = next_insn(p, BRW_OPCODE_SEND);
   unsigned data_type;
   if (has_scalar_region(src)) {
         } else {
                           /* Example code doesn't set predicate_control for send
   * instructions.
   */
   brw_inst_set_pred_control(devinfo, insn, 0);
            brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src);
   brw_set_math_message(p,
                        insn,
   }
      void gfx6_math(struct brw_codegen *p,
         struct brw_reg dest,
   unsigned function,
   struct brw_reg src0,
   {
      const struct intel_device_info *devinfo = p->devinfo;
                     assert(dest.file == BRW_GENERAL_REGISTER_FILE ||
            assert(dest.hstride == BRW_HORIZONTAL_STRIDE_1);
   if (devinfo->ver == 6) {
      assert(src0.hstride == BRW_HORIZONTAL_STRIDE_1);
               if (function == BRW_MATH_FUNCTION_INT_DIV_QUOTIENT ||
      function == BRW_MATH_FUNCTION_INT_DIV_REMAINDER ||
   function == BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER) {
   assert(src0.type != BRW_REGISTER_TYPE_F);
   assert(src1.type != BRW_REGISTER_TYPE_F);
   assert(src1.file == BRW_GENERAL_REGISTER_FILE ||
         /* From BSpec 6647/47428 "[Instruction] Extended Math Function":
   *     INT DIV function does not support source modifiers.
   */
   assert(!src0.negate);
   assert(!src0.abs);
   assert(!src1.negate);
      } else {
      assert(src0.type == BRW_REGISTER_TYPE_F ||
         assert(src1.type == BRW_REGISTER_TYPE_F ||
               /* Source modifiers are ignored for extended math instructions on Gfx6. */
   if (devinfo->ver == 6) {
      assert(!src0.negate);
   assert(!src0.abs);
   assert(!src1.negate);
                        brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
      }
      /**
   * Return the right surface index to access the thread scratch space using
   * stateless dataport messages.
   */
   unsigned
   brw_scratch_surface_idx(const struct brw_codegen *p)
   {
      /* The scratch space is thread-local so IA coherency is unnecessary. */
   if (p->devinfo->ver >= 8)
         else
      }
      /**
   * Write a block of OWORDs (half a GRF each) from the scratch buffer,
   * using a constant offset per channel.
   *
   * The offset must be aligned to oword size (16 bytes).  Used for
   * register spilling.
   */
   void brw_oword_block_write_scratch(struct brw_codegen *p,
         struct brw_reg mrf,
   int num_regs,
   {
      const struct intel_device_info *devinfo = p->devinfo;
   const unsigned target_cache =
      (devinfo->ver >= 7 ? GFX7_SFID_DATAPORT_DATA_CACHE :
   devinfo->ver >= 6 ? GFX6_SFID_DATAPORT_RENDER_CACHE :
      const struct tgl_swsb swsb = brw_get_default_swsb(p);
            if (devinfo->ver >= 6)
                              /* Set up the message header.  This is g0, with g0.2 filled with
   * the offset.  We don't want to leave our offset around in g0 or
   * it'll screw up texture samples, so set it up inside the message
   * reg.
   */
   {
      brw_push_insn_state(p);
   brw_set_default_exec_size(p, BRW_EXECUTE_8);
   brw_set_default_mask_control(p, BRW_MASK_DISABLE);
   brw_set_default_compression_control(p, BRW_COMPRESSION_NONE);
                     /* set message header global offset field (reg 0, element 2) */
   brw_set_default_exec_size(p, BRW_EXECUTE_1);
   brw_set_default_swsb(p, tgl_swsb_null());
   brw_MOV(p,
   retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE,
   mrf.nr,
   2), BRW_REGISTER_TYPE_UD),
            brw_pop_insn_state(p);
               {
      struct brw_reg dest;
   brw_inst *insn = next_insn(p, BRW_OPCODE_SEND);
   int send_commit_msg;
   struct brw_reg src_header = retype(brw_vec8_grf(0, 0),
            brw_inst_set_sfid(devinfo, insn, target_cache);
            src_header = vec16(src_header);
            assert(brw_inst_pred_control(devinfo, insn) == BRW_PREDICATE_NONE);
   if (devinfo->ver < 6)
            /* Until gfx6, writes followed by reads from the same location
   * are not guaranteed to be ordered unless write_commit is set.
   * If set, then a no-op write is issued to the destination
   * register to set a dependency, and a read from the destination
   * can be used to ensure the ordering.
   *
   * For gfx6, only writes between different threads need ordering
   * protection.  Our use of DP writes is all about register
   * spilling within a thread.
   */
   dest = retype(vec16(brw_null_reg()), BRW_REGISTER_TYPE_UW);
   send_commit_msg = 0;
         dest = src_header;
   send_commit_msg = 1;
                  brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, mrf);
         brw_set_src0(p, insn, brw_null_reg());
                  msg_type = GFX6_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE;
         msg_type = BRW_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE;
            brw_set_desc(p, insn,
               brw_message_desc(devinfo, mlen, send_commit_msg, true) |
         }
         /**
   * Read a block of owords (half a GRF each) from the scratch buffer
   * using a constant index per channel.
   *
   * Offset must be aligned to oword size (16 bytes).  Used for register
   * spilling.
   */
   void
   brw_oword_block_read_scratch(struct brw_codegen *p,
         struct brw_reg dest,
   struct brw_reg mrf,
   int num_regs,
   {
      const struct intel_device_info *devinfo = p->devinfo;
            if (devinfo->ver >= 6)
            if (p->devinfo->ver >= 7) {
      /* On gen 7 and above, we no longer have message registers and we can
   * send from any register we want.  By using the destination register
   * for the message, we guarantee that the implied message write won't
   * accidentally overwrite anything.  This has been a problem because
   * the MRF registers and source for the final FB write are both fixed
   * and may overlap.
   */
      } else {
         }
            const unsigned rlen = num_regs;
   const unsigned target_cache =
      (devinfo->ver >= 7 ? GFX7_SFID_DATAPORT_DATA_CACHE :
   devinfo->ver >= 6 ? GFX6_SFID_DATAPORT_RENDER_CACHE :
         {
      brw_push_insn_state(p);
   brw_set_default_swsb(p, tgl_swsb_src_dep(swsb));
   brw_set_default_exec_size(p, BRW_EXECUTE_8);
   brw_set_default_compression_control(p, BRW_COMPRESSION_NONE);
                     /* set message header global offset field (reg 0, element 2) */
   brw_set_default_exec_size(p, BRW_EXECUTE_1);
   brw_set_default_swsb(p, tgl_swsb_null());
            brw_pop_insn_state(p);
               {
               brw_inst_set_sfid(devinfo, insn, target_cache);
   assert(brw_inst_pred_control(devinfo, insn) == 0);
            brw_set_dest(p, insn, dest);	/* UW? */
   brw_set_src0(p, insn, mrf);
         brw_set_src0(p, insn, brw_null_reg());
                        brw_set_desc(p, insn,
               brw_message_desc(devinfo, 1, rlen, true) |
   brw_dp_read_desc(devinfo, brw_scratch_surface_idx(p),
         }
      void
   gfx7_block_read_scratch(struct brw_codegen *p,
                     {
      brw_inst *insn = next_insn(p, BRW_OPCODE_SEND);
                     /* The HW requires that the header is present; this is to get the g0.5
   * scratch offset.
   */
            /* According to the docs, offset is "A 12-bit HWord offset into the memory
   * Immediate Memory buffer as specified by binding table 0xFF."  An HWORD
   * is 32 bytes, which happens to be the size of a register.
   */
   offset /= REG_SIZE;
            gfx7_set_dp_scratch_message(p, insn,
                              false, /* scratch read */
   false, /* OWords */
   false, /* invalidate after read */
   }
      /**
   * Read float[4] vectors from the data port constant cache.
   * Location (in buffer) should be a multiple of 16.
   * Used for fetching shader constants.
   */
   void brw_oword_block_read(struct brw_codegen *p,
      struct brw_reg dest,
   struct brw_reg mrf,
   uint32_t offset,
      {
      const struct intel_device_info *devinfo = p->devinfo;
   const unsigned target_cache =
      (devinfo->ver >= 6 ? GFX6_SFID_DATAPORT_CONSTANT_CACHE :
      const unsigned exec_size = 1 << brw_get_default_exec_size(p);
            /* On newer hardware, offset is in units of owords. */
   if (devinfo->ver >= 6)
                     brw_push_insn_state(p);
   brw_set_default_predicate_control(p, BRW_PREDICATE_NONE);
   brw_set_default_compression_control(p, BRW_COMPRESSION_NONE);
            brw_push_insn_state(p);
   brw_set_default_exec_size(p, BRW_EXECUTE_8);
   brw_set_default_swsb(p, tgl_swsb_src_dep(swsb));
            /* set message header global offset field (reg 0, element 2) */
   brw_set_default_exec_size(p, BRW_EXECUTE_1);
   brw_set_default_swsb(p, tgl_swsb_null());
   brw_MOV(p,
   retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE,
         mrf.nr,
   brw_imm_ud(offset));
                                       /* cast dest to a uword[8] vector */
            brw_set_dest(p, insn, dest);
   if (devinfo->ver >= 6) {
         } else {
      brw_set_src0(p, insn, brw_null_reg());
               brw_set_desc(p, insn,
               brw_message_desc(devinfo, 1, DIV_ROUND_UP(exec_size, 8), true) |
   brw_dp_read_desc(devinfo, bind_table_index,
               }
      brw_inst *
   brw_fb_WRITE(struct brw_codegen *p,
               struct brw_reg payload,
   struct brw_reg implied_header,
   unsigned msg_control,
   unsigned binding_table_index,
   unsigned msg_length,
   unsigned response_length,
   bool eot,
   {
      const struct intel_device_info *devinfo = p->devinfo;
   const unsigned target_cache =
      (devinfo->ver >= 6 ? GFX6_SFID_DATAPORT_RENDER_CACHE :
      brw_inst *insn;
            if (brw_get_default_exec_size(p) >= BRW_EXECUTE_16)
         else
            if (devinfo->ver >= 6) {
         } else {
         }
   brw_inst_set_sfid(devinfo, insn, target_cache);
            if (devinfo->ver >= 6) {
      /* headerless version, just submit color payload */
      } else {
      assert(payload.file == BRW_MESSAGE_REGISTER_FILE);
   brw_inst_set_base_mrf(devinfo, insn, payload.nr);
               brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_desc(p, insn,
               brw_message_desc(devinfo, msg_length, response_length,
         brw_fb_write_desc(devinfo, binding_table_index, msg_control,
               }
      brw_inst *
   gfx9_fb_READ(struct brw_codegen *p,
               struct brw_reg dst,
   struct brw_reg payload,
   unsigned binding_table_index,
   unsigned msg_length,
   {
      const struct intel_device_info *devinfo = p->devinfo;
   assert(devinfo->ver >= 9);
            brw_inst_set_sfid(devinfo, insn, GFX6_SFID_DATAPORT_RENDER_CACHE);
   brw_set_dest(p, insn, dst);
   brw_set_src0(p, insn, payload);
   brw_set_desc(
      p, insn,
   brw_message_desc(devinfo, msg_length, response_length, true) |
   brw_fb_read_desc(devinfo, binding_table_index, 0 /* msg_control */,
                  }
      /**
   * Texture sample instruction.
   * Note: the msg_type plus msg_length values determine exactly what kind
   * of sampling operation is performed.  See volume 4, page 161 of docs.
   */
   void brw_SAMPLE(struct brw_codegen *p,
   struct brw_reg dest,
   unsigned msg_reg_nr,
   struct brw_reg src0,
   unsigned binding_table_index,
   unsigned sampler,
   unsigned msg_type,
   unsigned response_length,
   unsigned msg_length,
   unsigned header_present,
   unsigned simd_mode,
   unsigned return_format)
   {
      const struct intel_device_info *devinfo = p->devinfo;
            if (msg_reg_nr != -1)
            insn = next_insn(p, BRW_OPCODE_SEND);
   brw_inst_set_sfid(devinfo, insn, BRW_SFID_SAMPLER);
            /* From the 965 PRM (volume 4, part 1, section 14.2.41):
   *
   *    "Instruction compression is not allowed for this instruction (that
   *     is, send). The hardware behavior is undefined if this instruction is
   *     set as compressed. However, compress control can be set to "SecHalf"
   *     to affect the EMask generation."
   *
   * No similar wording is found in later PRMs, but there are examples
   * utilizing send with SecHalf.  More importantly, SIMD8 sampler messages
   * are allowed in SIMD16 mode and they could not work without SecHalf.  For
   * these reasons, we allow BRW_COMPRESSION_2NDHALF here.
   */
            if (devinfo->ver < 6)
            brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_desc(p, insn,
               brw_message_desc(devinfo, msg_length, response_length,
      }
      /* Adjust the message header's sampler state pointer to
   * select the correct group of 16 samplers.
   */
   void brw_adjust_sampler_state_pointer(struct brw_codegen *p,
               {
      /* The "Sampler Index" field can only store values between 0 and 15.
   * However, we can add an offset to the "Sampler State Pointer"
   * field, effectively selecting a different set of 16 samplers.
   *
   * The "Sampler State Pointer" needs to be aligned to a 32-byte
   * offset, and each sampler state is only 16-bytes, so we can't
   * exclusively use the offset - we have to use both.
                     if (sampler_index.file == BRW_IMMEDIATE_VALUE) {
      const int sampler_state_size = 16; /* 16 bytes */
            if (sampler >= 16) {
      assert(devinfo->verx10 >= 75);
   brw_ADD(p,
         get_element_ud(header, 3),
         } else {
      /* Non-const sampler array indexing case */
   if (devinfo->verx10 <= 70) {
                           brw_push_insn_state(p);
   brw_AND(p, temp, get_element_ud(sampler_index, 0), brw_imm_ud(0x0f0));
   brw_set_default_swsb(p, tgl_swsb_regdist(1));
   brw_SHL(p, temp, temp, brw_imm_ud(4));
   brw_ADD(p,
         get_element_ud(header, 3),
   get_element_ud(brw_vec8_grf(0, 0), 3),
         }
      /* All these variables are pretty confusing - we might be better off
   * using bitmasks and macros for this, in the old style.  Or perhaps
   * just having the caller instantiate the fields in dword3 itself.
   */
   void brw_urb_WRITE(struct brw_codegen *p,
      struct brw_reg dest,
   unsigned msg_reg_nr,
   struct brw_reg src0,
         unsigned msg_length,
   unsigned response_length,
   unsigned offset,
      {
      const struct intel_device_info *devinfo = p->devinfo;
                     if (devinfo->ver >= 7 && !(flags & BRW_URB_WRITE_USE_CHANNEL_MASKS)) {
      /* Enable Channel Masks in the URB_WRITE_HWORD message header */
   brw_push_insn_state(p);
   brw_set_default_access_mode(p, BRW_ALIGN_1);
   brw_set_default_mask_control(p, BRW_MASK_DISABLE);
   brw_set_default_exec_size(p, BRW_EXECUTE_1);
   brw_OR(p, retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE, msg_reg_nr, 5),
         brw_imm_ud(0xff00));
                                       brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
            if (devinfo->ver < 6)
            brw_set_urb_message(p,
         insn,
   flags,
   msg_length,
   response_length,
      }
      void
   brw_send_indirect_message(struct brw_codegen *p,
                           unsigned sfid,
   struct brw_reg dst,
   {
      const struct intel_device_info *devinfo = p->devinfo;
                              if (desc.file == BRW_IMMEDIATE_VALUE) {
      send = next_insn(p, BRW_OPCODE_SEND);
   brw_set_src0(p, send, retype(payload, BRW_REGISTER_TYPE_UD));
      } else {
      const struct tgl_swsb swsb = brw_get_default_swsb(p);
            brw_push_insn_state(p);
   brw_set_default_access_mode(p, BRW_ALIGN_1);
   brw_set_default_mask_control(p, BRW_MASK_DISABLE);
   brw_set_default_exec_size(p, BRW_EXECUTE_1);
   brw_set_default_predicate_control(p, BRW_PREDICATE_NONE);
            /* Load the indirect descriptor to an address register using OR so the
   * caller can specify additional descriptor bits with the desc_imm
   * immediate.
   */
                     brw_set_default_swsb(p, tgl_swsb_dst_dep(swsb, 1));
   send = next_insn(p, BRW_OPCODE_SEND);
            if (devinfo->ver >= 12)
         else
               brw_set_dest(p, send, dst);
   brw_inst_set_sfid(devinfo, send, sfid);
      }
      void
   brw_send_indirect_split_message(struct brw_codegen *p,
                                 unsigned sfid,
   struct brw_reg dst,
   struct brw_reg payload0,
   struct brw_reg payload1,
   struct brw_reg desc,
   unsigned desc_imm,
   {
      const struct intel_device_info *devinfo = p->devinfo;
                              if (desc.file == BRW_IMMEDIATE_VALUE) {
         } else {
      const struct tgl_swsb swsb = brw_get_default_swsb(p);
            brw_push_insn_state(p);
   brw_set_default_access_mode(p, BRW_ALIGN_1);
   brw_set_default_mask_control(p, BRW_MASK_DISABLE);
   brw_set_default_exec_size(p, BRW_EXECUTE_1);
   brw_set_default_predicate_control(p, BRW_PREDICATE_NONE);
            /* Load the indirect descriptor to an address register using OR so the
   * caller can specify additional descriptor bits with the desc_imm
   * immediate.
   */
            brw_pop_insn_state(p);
                        if (ex_desc.file == BRW_IMMEDIATE_VALUE &&
      !ex_desc_scratch &&
   (devinfo->ver >= 12 ||
   ((ex_desc.ud | ex_desc_imm) & INTEL_MASK(15, 12)) == 0)) {
   /* ATS-M PRMs, Volume 2d: Command Reference: Structures,
   * EU_INSTRUCTION_SEND instruction
   *
   *    "ExBSO: Exists If: ([ExDesc.IsReg]==true)"
   */
   assert(!ex_bso);
      } else {
      const struct tgl_swsb swsb = brw_get_default_swsb(p);
            brw_push_insn_state(p);
   brw_set_default_access_mode(p, BRW_ALIGN_1);
   brw_set_default_mask_control(p, BRW_MASK_DISABLE);
   brw_set_default_exec_size(p, BRW_EXECUTE_1);
   brw_set_default_predicate_control(p, BRW_PREDICATE_NONE);
            /* Load the indirect extended descriptor to an address register using OR
   * so the caller can specify additional descriptor bits with the
   * desc_imm immediate.
   *
   * Even though the instruction dispatcher always pulls the SFID and EOT
   * fields from the instruction itself, actual external unit which
   * processes the message gets the SFID and EOT from the extended
   * descriptor which comes from the address register.  If we don't OR
   * those two bits in, the external unit may get confused and hang.
   */
            if (ex_desc_scratch) {
      /* Or the scratch surface offset together with the immediate part of
   * the extended descriptor.
   */
   assert(devinfo->verx10 >= 125);
   brw_AND(p, addr,
         retype(brw_vec1_grf(0, 5), BRW_REGISTER_TYPE_UD),
      } else if (ex_desc.file == BRW_IMMEDIATE_VALUE) {
      /* ex_desc bits 15:12 don't exist in the instruction encoding prior
   * to Gfx12, so we may have fallen back to an indirect extended
   * descriptor.
   */
      } else {
                  brw_pop_insn_state(p);
                        send = next_insn(p, devinfo->ver >= 12 ? BRW_OPCODE_SEND : BRW_OPCODE_SENDS);
   brw_set_dest(p, send, dst);
   brw_set_src0(p, send, retype(payload0, BRW_REGISTER_TYPE_UD));
            if (desc.file == BRW_IMMEDIATE_VALUE) {
      brw_inst_set_send_sel_reg32_desc(devinfo, send, 0);
      } else {
      assert(desc.file == BRW_ARCHITECTURE_REGISTER_FILE);
   assert(desc.nr == BRW_ARF_ADDRESS);
   assert(desc.subnr == 0);
               if (ex_desc.file == BRW_IMMEDIATE_VALUE) {
      brw_inst_set_send_sel_reg32_ex_desc(devinfo, send, 0);
      } else {
      assert(ex_desc.file == BRW_ARCHITECTURE_REGISTER_FILE);
   assert(ex_desc.nr == BRW_ARF_ADDRESS);
   assert((ex_desc.subnr & 0x3) == 0);
   brw_inst_set_send_sel_reg32_ex_desc(devinfo, send, 1);
               if (ex_bso) {
      brw_inst_set_send_ex_bso(devinfo, send, true);
      }
   brw_inst_set_sfid(devinfo, send, sfid);
      }
      static void
   brw_send_indirect_surface_message(struct brw_codegen *p,
                                 {
      if (surface.file != BRW_IMMEDIATE_VALUE) {
      const struct tgl_swsb swsb = brw_get_default_swsb(p);
            brw_push_insn_state(p);
   brw_set_default_access_mode(p, BRW_ALIGN_1);
   brw_set_default_mask_control(p, BRW_MASK_DISABLE);
   brw_set_default_exec_size(p, BRW_EXECUTE_1);
   brw_set_default_predicate_control(p, BRW_PREDICATE_NONE);
            /* Mask out invalid bits from the surface index to avoid hangs e.g. when
   * some surface array is accessed out of bounds.
   */
   brw_AND(p, addr,
         suboffset(vec1(retype(surface, BRW_REGISTER_TYPE_UD)),
                     surface = addr;
                  }
      static bool
   while_jumps_before_offset(const struct intel_device_info *devinfo,
         {
      int scale = 16 / brw_jump_scale(devinfo);
   int jip = devinfo->ver == 6 ? brw_inst_gfx6_jump_count(devinfo, insn)
         assert(jip < 0);
      }
         static int
   brw_find_next_block_end(struct brw_codegen *p, int start_offset)
   {
      int offset;
   void *store = p->store;
                     for (offset = next_offset(devinfo, store, start_offset);
      offset < p->next_insn_offset;
   offset = next_offset(devinfo, store, offset)) {
            switch (brw_inst_opcode(p->isa, insn)) {
   case BRW_OPCODE_IF:
      depth++;
      case BRW_OPCODE_ENDIF:
      if (depth == 0)
         depth--;
      case BRW_OPCODE_WHILE:
      /* If the while doesn't jump before our instruction, it's the end
   * of a sibling do...while loop.  Ignore it.
   */
   if (!while_jumps_before_offset(devinfo, insn, offset, start_offset))
            case BRW_OPCODE_ELSE:
   case BRW_OPCODE_HALT:
      if (depth == 0)
            default:
                        }
      /* There is no DO instruction on gfx6, so to find the end of the loop
   * we have to see if the loop is jumping back before our start
   * instruction.
   */
   static int
   brw_find_loop_end(struct brw_codegen *p, int start_offset)
   {
      const struct intel_device_info *devinfo = p->devinfo;
   int offset;
                     /* Always start after the instruction (such as a WHILE) we're trying to fix
   * up.
   */
   for (offset = next_offset(devinfo, store, start_offset);
      offset < p->next_insn_offset;
   offset = next_offset(devinfo, store, offset)) {
            if (while_jumps_before_offset(devinfo, insn, offset, start_offset))
      return offset;
         }
   assert(!"not reached");
      }
      /* After program generation, go back and update the UIP and JIP of
   * BREAK, CONT, and HALT instructions to their correct locations.
   */
   void
   brw_set_uip_jip(struct brw_codegen *p, int start_offset)
   {
      const struct intel_device_info *devinfo = p->devinfo;
   int offset;
   int br = brw_jump_scale(devinfo);
   int scale = 16 / br;
            if (devinfo->ver < 6)
            for (offset = start_offset; offset < p->next_insn_offset; offset += 16) {
      brw_inst *insn = store + offset;
            switch (brw_inst_opcode(p->isa, insn)) {
   case BRW_OPCODE_BREAK: {
      int block_end_offset = brw_find_next_block_end(p, offset);
      /* Gfx7 UIP points to WHILE; Gfx6 points just after it */
            (brw_find_loop_end(p, offset) - offset +
      break;
                  case BRW_OPCODE_CONTINUE: {
      int block_end_offset = brw_find_next_block_end(p, offset);
   assert(block_end_offset != 0);
   brw_inst_set_jip(devinfo, insn, (block_end_offset - offset) / scale);
                     break;
                  case BRW_OPCODE_ENDIF: {
      int block_end_offset = brw_find_next_block_end(p, offset);
   int32_t jump = (block_end_offset == 0) ?
         if (devinfo->ver >= 7)
            break;
                  /* From the Sandy Bridge PRM (volume 4, part 2, section 8.3.19):
      *
   *    "In case of the halt instruction not inside any conditional
   *     code block, the value of <JIP> and <UIP> should be the
   *     same. In case of the halt instruction inside conditional code
   *     block, the <UIP> should be the end of the program, and the
   *     <JIP> should be end of the most inner conditional code block."
   *
   * The uip will have already been set by whoever set up the
   * instruction.
   */
      if (block_end_offset == 0) {
         } else {
         }
               break;
                  default:
               }
      void brw_ff_sync(struct brw_codegen *p,
      struct brw_reg dest,
   unsigned msg_reg_nr,
   struct brw_reg src0,
   bool allocate,
   unsigned response_length,
      {
      const struct intel_device_info *devinfo = p->devinfo;
                     insn = next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
            if (devinfo->ver < 6)
            brw_set_ff_sync_message(p,
      insn,
   allocate,
   response_length,
   }
      /**
   * Emit the SEND instruction necessary to generate stream output data on Gfx6
   * (for transform feedback).
   *
   * If send_commit_msg is true, this is the last piece of stream output data
   * from this thread, so send the data as a committed write.  According to the
   * Sandy Bridge PRM (volume 2 part 1, section 4.5.1):
   *
   *   "Prior to End of Thread with a URB_WRITE, the kernel must ensure all
   *   writes are complete by sending the final write as a committed write."
   */
   void
   brw_svb_write(struct brw_codegen *p,
               struct brw_reg dest,
   unsigned msg_reg_nr,
   struct brw_reg src0,
   {
      const struct intel_device_info *devinfo = p->devinfo;
   assert(devinfo->ver == 6);
   const unsigned target_cache = GFX6_SFID_DATAPORT_RENDER_CACHE;
                     insn = next_insn(p, BRW_OPCODE_SEND);
   brw_inst_set_sfid(devinfo, insn, target_cache);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_desc(p, insn,
               brw_message_desc(devinfo, 1, send_commit_msg, true) |
   brw_dp_write_desc(devinfo, binding_table_index,
      }
      static unsigned
   brw_surface_payload_size(unsigned num_channels,
         {
      if (exec_size == 0)
         else if (exec_size <= 8)
         else
      }
      void
   brw_untyped_atomic(struct brw_codegen *p,
                     struct brw_reg dst,
   struct brw_reg payload,
   struct brw_reg surface,
   unsigned atomic_op,
   {
      const struct intel_device_info *devinfo = p->devinfo;
   const unsigned sfid = (devinfo->verx10 >= 75 ?
               const bool align1 = brw_get_default_access_mode(p) == BRW_ALIGN_1;
   /* SIMD4x2 untyped atomic instructions only exist on HSW+ */
   const bool has_simd4x2 = devinfo->verx10 >= 75;
   const unsigned exec_size = align1 ? 1 << brw_get_default_exec_size(p) :
         const unsigned response_length =
         const unsigned desc =
      brw_message_desc(devinfo, msg_length, response_length, header_present) |
   brw_dp_untyped_atomic_desc(devinfo, exec_size, atomic_op,
      /* Mask out unused components -- This is especially important in Align16
   * mode on generations that don't have native support for SIMD4x2 atomics,
   * because unused but enabled components will cause the dataport to perform
   * additional atomic operations on the addresses that happen to be in the
   * uninitialized Y, Z and W coordinates of the payload.
   */
            brw_send_indirect_surface_message(p, sfid, brw_writemask(dst, mask),
      }
      void
   brw_untyped_surface_read(struct brw_codegen *p,
                           struct brw_reg dst,
   {
      const struct intel_device_info *devinfo = p->devinfo;
   const unsigned sfid = (devinfo->verx10 >= 75 ?
               const bool align1 = brw_get_default_access_mode(p) == BRW_ALIGN_1;
   const unsigned exec_size = align1 ? 1 << brw_get_default_exec_size(p) : 0;
   const unsigned response_length =
         const unsigned desc =
      brw_message_desc(devinfo, msg_length, response_length, false) |
            }
      void
   brw_untyped_surface_write(struct brw_codegen *p,
                           struct brw_reg payload,
   {
      const struct intel_device_info *devinfo = p->devinfo;
   const unsigned sfid = (devinfo->verx10 >= 75 ?
               const bool align1 = brw_get_default_access_mode(p) == BRW_ALIGN_1;
   /* SIMD4x2 untyped surface write instructions only exist on HSW+ */
   const bool has_simd4x2 = devinfo->verx10 >= 75;
   const unsigned exec_size = align1 ? 1 << brw_get_default_exec_size(p) :
         const unsigned desc =
      brw_message_desc(devinfo, msg_length, 0, header_present) |
      /* Mask out unused components -- See comment in brw_untyped_atomic(). */
            brw_send_indirect_surface_message(p, sfid, brw_writemask(brw_null_reg(), mask),
      }
      static void
   brw_set_memory_fence_message(struct brw_codegen *p,
                           {
               brw_set_desc(p, insn, brw_message_desc(
                     switch (sfid) {
   case GFX6_SFID_DATAPORT_RENDER_CACHE:
      brw_inst_set_dp_msg_type(devinfo, insn, GFX7_DATAPORT_RC_MEMORY_FENCE);
      case GFX7_SFID_DATAPORT_DATA_CACHE:
      brw_inst_set_dp_msg_type(devinfo, insn, GFX7_DATAPORT_DC_MEMORY_FENCE);
      default:
                  if (commit_enable)
            assert(devinfo->ver >= 11 || bti == 0);
      }
      static void
   gfx12_set_memory_fence_message(struct brw_codegen *p,
                     {
      const unsigned mlen = 1 * reg_unit(p->devinfo); /* g0 header */
   /* Completion signaled by write to register. No data returned. */
                     if (sfid == BRW_SFID_URB && p->devinfo->ver < 20) {
      brw_set_desc(p, insn, brw_urb_fence_desc(p->devinfo) |
      } else {
      enum lsc_fence_scope scope = lsc_fence_msg_desc_scope(p->devinfo, desc);
            if (sfid == GFX12_SFID_TGM) {
      scope = LSC_FENCE_TILE;
               /* Wa_14012437816:
   *
   *   "For any fence greater than local scope, always set flush type to
   *    at least invalidate so that fence goes on properly."
   *
   *   "The bug is if flush_type is 'None', the scope is always downgraded
   *    to 'local'."
   *
   * Here set scope to NONE_6 instead of NONE, which has the same effect
   * as NONE but avoids the downgrade to scope LOCAL.
   */
   if (intel_needs_workaround(p->devinfo, 14012437816) &&
      scope > LSC_FENCE_LOCAL &&
   flush_type == LSC_FLUSH_TYPE_NONE) {
               brw_set_desc(p, insn, lsc_fence_msg_desc(p->devinfo, scope,
               }
      void
   brw_memory_fence(struct brw_codegen *p,
                  struct brw_reg dst,
   struct brw_reg src,
   enum opcode send_op,
   enum brw_message_target sfid,
      {
               dst = retype(vec1(dst), BRW_REGISTER_TYPE_UW);
            /* Set dst as destination for dependency tracking, the MEMORY_FENCE
   * message doesn't write anything back.
   */
   struct brw_inst *insn = next_insn(p, send_op);
   brw_inst_set_mask_control(devinfo, insn, BRW_MASK_DISABLE);
   brw_inst_set_exec_size(devinfo, insn, BRW_EXECUTE_1);
   brw_set_dest(p, insn, dst);
            /* All DG2 hardware requires LSC for fence messages, even A-step */
   if (devinfo->has_lsc)
         else
      }
      void
   brw_find_live_channel(struct brw_codegen *p, struct brw_reg dst, bool last)
   {
      const struct intel_device_info *devinfo = p->devinfo;
   const unsigned exec_size = 1 << brw_get_default_exec_size(p);
   const unsigned qtr_control = brw_get_default_group(p) / 8;
                              /* The flag register is only used on Gfx7 in align1 mode, so avoid setting
   * unnecessary bits in the instruction words, get the information we need
   * and reset the default flag register. This allows more instructions to be
   * compacted.
   */
   const unsigned flag_subreg = p->current->flag_subreg;
            if (brw_get_default_access_mode(p) == BRW_ALIGN_1) {
                        brw_set_default_exec_size(p, BRW_EXECUTE_1);
            /* Run enough instructions returning zero with execution masking and
   * a conditional modifier enabled in order to get the full execution
   * mask in f1.0.  We could use a single 32-wide move here if it
   * weren't because of the hardware bug that causes channel enables to
   * be applied incorrectly to the second half of 32-wide instructions
   * on Gfx7.
   */
   const unsigned lower_size = MIN2(16, exec_size);
   for (unsigned i = 0; i < exec_size / lower_size; i++) {
      inst = brw_MOV(p, retype(brw_null_reg(), BRW_REGISTER_TYPE_UW),
         brw_inst_set_mask_control(devinfo, inst, BRW_MASK_ENABLE);
   brw_inst_set_group(devinfo, inst, lower_size * i + 8 * qtr_control);
   brw_inst_set_cond_modifier(devinfo, inst, BRW_CONDITIONAL_Z);
   brw_inst_set_exec_size(devinfo, inst, cvt(lower_size) - 1);
   brw_inst_set_flag_reg_nr(devinfo, inst, flag_subreg / 2);
               /* Find the first bit set in the exec_size-wide portion of the flag
   * register that was updated by the last sequence of MOV
   * instructions.
   */
   const enum brw_reg_type type = brw_int_type(exec_size / 8, false);
   brw_set_default_exec_size(p, BRW_EXECUTE_1);
   if (!last) {
         } else {
      inst = brw_LZD(p, vec1(dst), byte_offset(retype(flag, type), qtr_control));
   struct brw_reg neg = vec1(dst);
   neg.negate = true;
         } else {
               /* Overwrite the destination without and with execution masking to
   * find out which of the channels is active.
   */
   brw_push_insn_state(p);
   brw_set_default_exec_size(p, BRW_EXECUTE_4);
   brw_MOV(p, brw_writemask(vec4(dst), WRITEMASK_X),
            inst = brw_MOV(p, brw_writemask(vec4(dst), WRITEMASK_X),
         brw_pop_insn_state(p);
                  }
      void
   brw_broadcast(struct brw_codegen *p,
               struct brw_reg dst,
   {
      const struct intel_device_info *devinfo = p->devinfo;
   const bool align1 = brw_get_default_access_mode(p) == BRW_ALIGN_1;
            brw_push_insn_state(p);
   brw_set_default_mask_control(p, BRW_MASK_DISABLE);
            assert(src.file == BRW_GENERAL_REGISTER_FILE &&
                  /* Gen12.5 adds the following region restriction:
   *
   *    "Vx1 and VxH indirect addressing for Float, Half-Float, Double-Float
   *    and Quad-Word data must not be used."
   *
   * We require the source and destination types to match so stomp to an
   * unsigned integer type.
   */
   assert(src.type == dst.type);
   src.type = dst.type = brw_reg_type_from_bit_size(type_sz(src.type) * 8,
            if ((src.vstride == 0 && (src.hstride == 0 || !align1)) ||
      idx.file == BRW_IMMEDIATE_VALUE) {
   /* Trivial, the source is already uniform or the index is a constant.
   * We will typically not get here if the optimizer is doing its job, but
   * asserting would be mean.
   */
   const unsigned i = idx.file == BRW_IMMEDIATE_VALUE ? idx.ud : 0;
   src = align1 ? stride(suboffset(src, i), 0, 1, 0) :
            if (type_sz(src.type) > 4 && !devinfo->has_64bit_int) {
      brw_MOV(p, subscript(dst, BRW_REGISTER_TYPE_D, 0),
         brw_set_default_swsb(p, tgl_swsb_null());
   brw_MOV(p, subscript(dst, BRW_REGISTER_TYPE_D, 1),
      } else {
            } else {
      /* From the Haswell PRM section "Register Region Restrictions":
   *
   *    "The lower bits of the AddressImmediate must not overflow to
   *    change the register address.  The lower 5 bits of Address
   *    Immediate when added to lower 5 bits of address register gives
   *    the sub-register offset. The upper bits of Address Immediate
   *    when added to upper bits of address register gives the register
   *    address. Any overflow from sub-register offset is dropped."
   *
   * Fortunately, for broadcast, we never have a sub-register offset so
   * this isn't an issue.
   */
            if (align1) {
      const struct brw_reg addr =
         unsigned offset = src.nr * REG_SIZE + src.subnr;
                  brw_push_insn_state(p);
                  /* Take into account the component size and horizontal stride. */
   assert(src.vstride == src.hstride + src.width);
   brw_SHL(p, addr, vec1(idx),
                  /* We can only address up to limit bytes using the indirect
   * addressing immediate, account for the difference if the source
   * register is above this limit.
   */
   if (offset >= limit) {
      brw_set_default_swsb(p, tgl_swsb_regdist(1));
   brw_ADD(p, addr, addr, brw_imm_ud(offset - offset % limit));
                                 /* Use indirect addressing to fetch the specified component. */
   if (type_sz(src.type) > 4 &&
      (devinfo->platform == INTEL_PLATFORM_CHV || intel_device_info_is_9lp(devinfo) ||
   !devinfo->has_64bit_int)) {
   /* From the Cherryview PRM Vol 7. "Register Region Restrictions":
   *
   *    "When source or destination datatype is 64b or operation is
   *    integer DWord multiply, indirect addressing must not be
   *    used."
   *
   * To work around both of this issue, we do two integer MOVs
   * insead of one 64-bit MOV.  Because no double value should ever
   * cross a register boundary, it's safe to use the immediate
   * offset in the indirect here to handle adding 4 bytes to the
   * offset and avoid the extra ADD to the register file.
   */
   brw_MOV(p, subscript(dst, BRW_REGISTER_TYPE_D, 0),
               brw_set_default_swsb(p, tgl_swsb_null());
   brw_MOV(p, subscript(dst, BRW_REGISTER_TYPE_D, 1),
            } else {
      brw_MOV(p, dst,
         } else {
      /* In SIMD4x2 mode the index can be either zero or one, replicate it
   * to all bits of a flag register,
   */
   inst = brw_MOV(p,
               brw_inst_set_pred_control(devinfo, inst, BRW_PREDICATE_NONE);
                  /* and use predicated SEL to pick the right channel. */
   inst = brw_SEL(p, dst,
               brw_inst_set_pred_control(devinfo, inst, BRW_PREDICATE_NORMAL);
                     }
         /**
   * Emit the SEND message for a barrier
   */
   void
   brw_barrier(struct brw_codegen *p, struct brw_reg src)
   {
      const struct intel_device_info *devinfo = p->devinfo;
                     brw_push_insn_state(p);
   brw_set_default_access_mode(p, BRW_ALIGN_1);
   inst = next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, inst, retype(brw_null_reg(), BRW_REGISTER_TYPE_UW));
   brw_set_src0(p, inst, src);
   brw_set_src1(p, inst, brw_null_reg());
   brw_set_desc(p, inst, brw_message_desc(devinfo,
            brw_inst_set_sfid(devinfo, inst, BRW_SFID_MESSAGE_GATEWAY);
   brw_inst_set_gateway_subfuncid(devinfo, inst,
            brw_inst_set_mask_control(devinfo, inst, BRW_MASK_DISABLE);
      }
         /**
   * Emit the wait instruction for a barrier
   */
   void
   brw_WAIT(struct brw_codegen *p)
   {
      const struct intel_device_info *devinfo = p->devinfo;
                     insn = next_insn(p, BRW_OPCODE_WAIT);
   brw_set_dest(p, insn, src);
   brw_set_src0(p, insn, src);
            brw_inst_set_exec_size(devinfo, insn, BRW_EXECUTE_1);
      }
      void
   brw_float_controls_mode(struct brw_codegen *p,
         {
               /* From the Skylake PRM, Volume 7, page 760:
   *  "Implementation Restriction on Register Access: When the control
   *   register is used as an explicit source and/or destination, hardware
   *   does not ensure execution pipeline coherency. Software must set the
   *   thread control field to ‘switch’ for an instruction that uses
   *   control register as an explicit operand."
   *
   * On Gfx12+ this is implemented in terms of SWSB annotations instead.
   */
            brw_inst *inst = brw_AND(p, brw_cr0_reg(0), brw_cr0_reg(0),
         brw_inst_set_exec_size(p->devinfo, inst, BRW_EXECUTE_1);
   if (p->devinfo->ver < 12)
            if (mode) {
      brw_inst *inst_or = brw_OR(p, brw_cr0_reg(0), brw_cr0_reg(0),
         brw_inst_set_exec_size(p->devinfo, inst_or, BRW_EXECUTE_1);
   if (p->devinfo->ver < 12)
               if (p->devinfo->ver >= 12)
      }
      void
   brw_update_reloc_imm(const struct brw_isa_info *isa,
               {
               /* Sanity check that the instruction is a MOV of an immediate */
   assert(brw_inst_opcode(isa, inst) == BRW_OPCODE_MOV);
            /* If it was compacted, we can't safely rewrite */
               }
      /* A default value for constants that will be patched at run-time.
   * We pick an arbitrary value that prevents instruction compaction.
   */
   #define DEFAULT_PATCH_IMM 0x4a7cc037
      void
   brw_MOV_reloc_imm(struct brw_codegen *p,
                     {
      assert(type_sz(src_type) == 4);
            brw_add_reloc(p, id, BRW_SHADER_RELOC_TYPE_MOV_IMM,
               }
