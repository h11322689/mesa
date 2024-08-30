   /*
   * Copyright © 2015-2019 Intel Corporation
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
      /** @file brw_eu_validate.c
   *
   * This file implements a pass that validates shader assembly.
   *
   * The restrictions implemented herein are intended to verify that instructions
   * in shader assembly do not violate restrictions documented in the graphics
   * programming reference manuals.
   *
   * The restrictions are difficult for humans to quickly verify due to their
   * complexity and abundance.
   *
   * It is critical that this code is thoroughly unit tested because false
   * results will lead developers astray, which is worse than having no validator
   * at all. Functional changes to this file without corresponding unit tests (in
   * test_eu_validate.cpp) will be rejected.
   */
      #include <stdlib.h>
   #include "brw_eu.h"
      /* We're going to do lots of string concatenation, so this should help. */
   struct string {
      char *str;
      };
      static void
   cat(struct string *dest, const struct string src)
   {
      dest->str = realloc(dest->str, dest->len + src.len + 1);
   memcpy(dest->str + dest->len, src.str, src.len);
   dest->str[dest->len + src.len] = '\0';
      }
   #define CAT(dest, src) cat(&dest, (struct string){src, strlen(src)})
      static bool
   contains(const struct string haystack, const struct string needle)
   {
      return haystack.str && memmem(haystack.str, haystack.len,
      }
   #define CONTAINS(haystack, needle) \
            #define error(str)   "\tERROR: " str "\n"
   #define ERROR_INDENT "\t       "
      #define ERROR(msg) ERROR_IF(true, msg)
   #define ERROR_IF(cond, msg)                             \
      do {                                                 \
      if ((cond) && !CONTAINS(error_msg, error(msg))) { \
                  #define CHECK(func, args...)                             \
      do {                                                  \
      struct string __msg = func(isa, inst, ##args); \
   if (__msg.str) {                                   \
      cat(&error_msg, __msg);                         \
               #define STRIDE(stride) (stride != 0 ? 1 << ((stride) - 1) : 0)
   #define WIDTH(width)   (1 << (width))
      static bool
   inst_is_send(const struct brw_isa_info *isa, const brw_inst *inst)
   {
      switch (brw_inst_opcode(isa, inst)) {
   case BRW_OPCODE_SEND:
   case BRW_OPCODE_SENDC:
   case BRW_OPCODE_SENDS:
   case BRW_OPCODE_SENDSC:
         default:
            }
      static bool
   inst_is_split_send(const struct brw_isa_info *isa, const brw_inst *inst)
   {
               if (devinfo->ver >= 12) {
         } else {
      switch (brw_inst_opcode(isa, inst)) {
   case BRW_OPCODE_SENDS:
   case BRW_OPCODE_SENDSC:
         default:
               }
      static unsigned
   signed_type(unsigned type)
   {
      switch (type) {
   case BRW_REGISTER_TYPE_UD: return BRW_REGISTER_TYPE_D;
   case BRW_REGISTER_TYPE_UW: return BRW_REGISTER_TYPE_W;
   case BRW_REGISTER_TYPE_UB: return BRW_REGISTER_TYPE_B;
   case BRW_REGISTER_TYPE_UQ: return BRW_REGISTER_TYPE_Q;
   default:                   return type;
      }
      static enum brw_reg_type
   inst_dst_type(const struct brw_isa_info *isa, const brw_inst *inst)
   {
               return (devinfo->ver < 12 || !inst_is_send(isa, inst)) ?
      }
      static bool
   inst_is_raw_move(const struct brw_isa_info *isa, const brw_inst *inst)
   {
               unsigned dst_type = signed_type(inst_dst_type(isa, inst));
            if (brw_inst_src0_reg_file(devinfo, inst) == BRW_IMMEDIATE_VALUE) {
      /* FIXME: not strictly true */
   if (brw_inst_src0_type(devinfo, inst) == BRW_REGISTER_TYPE_VF ||
      brw_inst_src0_type(devinfo, inst) == BRW_REGISTER_TYPE_UV ||
   brw_inst_src0_type(devinfo, inst) == BRW_REGISTER_TYPE_V) {
         } else if (brw_inst_src0_negate(devinfo, inst) ||
                        return brw_inst_opcode(isa, inst) == BRW_OPCODE_MOV &&
            }
      static bool
   dst_is_null(const struct intel_device_info *devinfo, const brw_inst *inst)
   {
      return brw_inst_dst_reg_file(devinfo, inst) == BRW_ARCHITECTURE_REGISTER_FILE &&
      }
      static bool
   src0_is_null(const struct intel_device_info *devinfo, const brw_inst *inst)
   {
      return brw_inst_src0_address_mode(devinfo, inst) == BRW_ADDRESS_DIRECT &&
            }
      static bool
   src1_is_null(const struct intel_device_info *devinfo, const brw_inst *inst)
   {
      return brw_inst_src1_reg_file(devinfo, inst) == BRW_ARCHITECTURE_REGISTER_FILE &&
      }
      static bool
   src0_is_acc(const struct intel_device_info *devinfo, const brw_inst *inst)
   {
      return brw_inst_src0_reg_file(devinfo, inst) == BRW_ARCHITECTURE_REGISTER_FILE &&
      }
      static bool
   src1_is_acc(const struct intel_device_info *devinfo, const brw_inst *inst)
   {
      return brw_inst_src1_reg_file(devinfo, inst) == BRW_ARCHITECTURE_REGISTER_FILE &&
      }
      static bool
   src0_has_scalar_region(const struct intel_device_info *devinfo,
         {
      return brw_inst_src0_vstride(devinfo, inst) == BRW_VERTICAL_STRIDE_0 &&
            }
      static bool
   src1_has_scalar_region(const struct intel_device_info *devinfo,
         {
      return brw_inst_src1_vstride(devinfo, inst) == BRW_VERTICAL_STRIDE_0 &&
            }
      static struct string
   invalid_values(const struct brw_isa_info *isa, const brw_inst *inst)
   {
               unsigned num_sources = brw_num_sources_from_inst(isa, inst);
            switch ((enum brw_execution_size) brw_inst_exec_size(devinfo, inst)) {
   case BRW_EXECUTE_1:
   case BRW_EXECUTE_2:
   case BRW_EXECUTE_4:
   case BRW_EXECUTE_8:
   case BRW_EXECUTE_16:
   case BRW_EXECUTE_32:
         default:
      ERROR("invalid execution size");
               if (inst_is_send(isa, inst))
            if (num_sources == 3) {
      /* Nothing to test:
   *    No 3-src instructions on Gfx4-5
   *    No reg file bits on Gfx6-10 (align16)
   *    No invalid encodings on Gfx10-12 (align1)
      } else {
      if (devinfo->ver > 6) {
      ERROR_IF(brw_inst_dst_reg_file(devinfo, inst) == MRF ||
            (num_sources > 0 &&
   brw_inst_src0_reg_file(devinfo, inst) == MRF) ||
   (num_sources > 1 &&
               if (error_msg.str)
            if (num_sources == 3) {
      if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      if (devinfo->ver >= 10) {
      ERROR_IF(brw_inst_3src_a1_dst_type (devinfo, inst) == INVALID_REG_TYPE ||
            brw_inst_3src_a1_src0_type(devinfo, inst) == INVALID_REG_TYPE ||
   brw_inst_3src_a1_src1_type(devinfo, inst) == INVALID_REG_TYPE ||
   } else {
            } else {
      ERROR_IF(brw_inst_3src_a16_dst_type(devinfo, inst) == INVALID_REG_TYPE ||
               } else {
      ERROR_IF(brw_inst_dst_type (devinfo, inst) == INVALID_REG_TYPE ||
            (num_sources > 0 &&
   brw_inst_src0_type(devinfo, inst) == INVALID_REG_TYPE) ||
   (num_sources > 1 &&
               }
      static struct string
   sources_not_null(const struct brw_isa_info *isa,
         {
      const struct intel_device_info *devinfo = isa->devinfo;
   unsigned num_sources = brw_num_sources_from_inst(isa, inst);
            /* Nothing to test. 3-src instructions can only have GRF sources, and
   * there's no bit to control the file.
   */
   if (num_sources == 3)
            /* Nothing to test.  Split sends can only encode a file in sources that are
   * allowed to be NULL.
   */
   if (inst_is_split_send(isa, inst))
            if (num_sources >= 1 && brw_inst_opcode(isa, inst) != BRW_OPCODE_SYNC)
            if (num_sources == 2)
               }
      static struct string
   alignment_supported(const struct brw_isa_info *isa,
         {
      const struct intel_device_info *devinfo = isa->devinfo;
            ERROR_IF(devinfo->ver >= 11 && brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_16,
               }
      static bool
   inst_uses_src_acc(const struct brw_isa_info *isa,
         {
               /* Check instructions that use implicit accumulator sources */
   switch (brw_inst_opcode(isa, inst)) {
   case BRW_OPCODE_MAC:
   case BRW_OPCODE_MACH:
   case BRW_OPCODE_SADA2:
         default:
                  /* FIXME: support 3-src instructions */
   unsigned num_sources = brw_num_sources_from_inst(isa, inst);
               }
      static struct string
   send_restrictions(const struct brw_isa_info *isa,
         {
                        if (inst_is_split_send(isa, inst)) {
      ERROR_IF(brw_inst_send_src1_reg_file(devinfo, inst) == BRW_ARCHITECTURE_REGISTER_FILE &&
                  ERROR_IF(brw_inst_eot(devinfo, inst) &&
               ERROR_IF(brw_inst_eot(devinfo, inst) &&
                        if (brw_inst_send_src0_reg_file(devinfo, inst) == BRW_GENERAL_REGISTER_FILE &&
      brw_inst_send_src1_reg_file(devinfo, inst) == BRW_GENERAL_REGISTER_FILE) {
   /* Assume minimums if we don't know */
   unsigned mlen = 1;
   if (!brw_inst_send_sel_reg32_desc(devinfo, inst)) {
      const uint32_t desc = brw_inst_send_desc(devinfo, inst);
               unsigned ex_mlen = 1;
   if (!brw_inst_send_sel_reg32_ex_desc(devinfo, inst)) {
      const uint32_t ex_desc = brw_inst_sends_ex_desc(devinfo, inst);
   ex_mlen = brw_message_ex_desc_ex_mlen(devinfo, ex_desc) /
      }
   const unsigned src0_reg_nr = brw_inst_src0_da_reg_nr(devinfo, inst);
   const unsigned src1_reg_nr = brw_inst_send_src1_reg_nr(devinfo, inst);
   ERROR_IF((src0_reg_nr <= src1_reg_nr &&
            src1_reg_nr < src0_reg_nr + mlen) ||
   (src1_reg_nr <= src0_reg_nr &&
      } else if (inst_is_send(isa, inst)) {
      ERROR_IF(brw_inst_src0_address_mode(devinfo, inst) != BRW_ADDRESS_DIRECT,
            if (devinfo->ver >= 7) {
      ERROR_IF(brw_inst_send_src0_reg_file(devinfo, inst) != BRW_GENERAL_REGISTER_FILE,
         ERROR_IF(brw_inst_eot(devinfo, inst) &&
                     if (devinfo->ver >= 8) {
      ERROR_IF(!dst_is_null(devinfo, inst) &&
            (brw_inst_dst_da_reg_nr(devinfo, inst) +
   brw_inst_rlen(devinfo, inst) > 127) &&
   (brw_inst_src0_da_reg_nr(devinfo, inst) +
   brw_inst_mlen(devinfo, inst) >
   brw_inst_dst_da_reg_nr(devinfo, inst)),
                  }
      static bool
   is_unsupported_inst(const struct brw_isa_info *isa,
         {
         }
      /**
   * Returns whether a combination of two types would qualify as mixed float
   * operation mode
   */
   static inline bool
   types_are_mixed_float(enum brw_reg_type t0, enum brw_reg_type t1)
   {
      return (t0 == BRW_REGISTER_TYPE_F && t1 == BRW_REGISTER_TYPE_HF) ||
      }
      static enum brw_reg_type
   execution_type_for_type(enum brw_reg_type type)
   {
      switch (type) {
   case BRW_REGISTER_TYPE_NF:
   case BRW_REGISTER_TYPE_DF:
   case BRW_REGISTER_TYPE_F:
   case BRW_REGISTER_TYPE_HF:
            case BRW_REGISTER_TYPE_VF:
            case BRW_REGISTER_TYPE_Q:
   case BRW_REGISTER_TYPE_UQ:
            case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_UD:
            case BRW_REGISTER_TYPE_W:
   case BRW_REGISTER_TYPE_UW:
   case BRW_REGISTER_TYPE_B:
   case BRW_REGISTER_TYPE_UB:
   case BRW_REGISTER_TYPE_V:
   case BRW_REGISTER_TYPE_UV:
         }
      }
      /**
   * Returns the execution type of an instruction \p inst
   */
   static enum brw_reg_type
   execution_type(const struct brw_isa_info *isa, const brw_inst *inst)
   {
               unsigned num_sources = brw_num_sources_from_inst(isa, inst);
            /* Execution data type is independent of destination data type, except in
   * mixed F/HF instructions.
   */
            src0_exec_type = execution_type_for_type(brw_inst_src0_type(devinfo, inst));
   if (num_sources == 1) {
      if (src0_exec_type == BRW_REGISTER_TYPE_HF)
                     src1_exec_type = execution_type_for_type(brw_inst_src1_type(devinfo, inst));
   if (types_are_mixed_float(src0_exec_type, src1_exec_type) ||
      types_are_mixed_float(src0_exec_type, dst_exec_type) ||
   types_are_mixed_float(src1_exec_type, dst_exec_type)) {
               if (src0_exec_type == src1_exec_type)
            if (src0_exec_type == BRW_REGISTER_TYPE_NF ||
      src1_exec_type == BRW_REGISTER_TYPE_NF)
         /* Mixed operand types where one is float is float on Gen < 6
   * (and not allowed on later platforms)
   */
   if (devinfo->ver < 6 &&
      (src0_exec_type == BRW_REGISTER_TYPE_F ||
   src1_exec_type == BRW_REGISTER_TYPE_F))
         if (src0_exec_type == BRW_REGISTER_TYPE_Q ||
      src1_exec_type == BRW_REGISTER_TYPE_Q)
         if (src0_exec_type == BRW_REGISTER_TYPE_D ||
      src1_exec_type == BRW_REGISTER_TYPE_D)
         if (src0_exec_type == BRW_REGISTER_TYPE_W ||
      src1_exec_type == BRW_REGISTER_TYPE_W)
         if (src0_exec_type == BRW_REGISTER_TYPE_DF ||
      src1_exec_type == BRW_REGISTER_TYPE_DF)
            }
      /**
   * Returns whether a region is packed
   *
   * A region is packed if its elements are adjacent in memory, with no
   * intervening space, no overlap, and no replicated values.
   */
   static bool
   is_packed(unsigned vstride, unsigned width, unsigned hstride)
   {
      if (vstride == width) {
      if (vstride == 1) {
         } else {
                        }
      /**
   * Returns whether a region is linear
   *
   * A region is linear if its elements do not overlap and are not replicated.
   * Unlike a packed region, intervening space (i.e. strided values) is allowed.
   */
   static bool
   is_linear(unsigned vstride, unsigned width, unsigned hstride)
   {
      return vstride == width * hstride ||
      }
      /**
   * Returns whether an instruction is an explicit or implicit conversion
   * to/from half-float.
   */
   static bool
   is_half_float_conversion(const struct brw_isa_info *isa,
         {
                        unsigned num_sources = brw_num_sources_from_inst(isa, inst);
            if (dst_type != src0_type &&
      (dst_type == BRW_REGISTER_TYPE_HF || src0_type == BRW_REGISTER_TYPE_HF)) {
      } else if (num_sources > 1) {
      enum brw_reg_type src1_type = brw_inst_src1_type(devinfo, inst);
   return dst_type != src1_type &&
                        }
      /*
   * Returns whether an instruction is using mixed float operation mode
   */
   static bool
   is_mixed_float(const struct brw_isa_info *isa, const brw_inst *inst)
   {
               if (devinfo->ver < 8)
            if (inst_is_send(isa, inst))
            unsigned opcode = brw_inst_opcode(isa, inst);
   const struct opcode_desc *desc = brw_opcode_desc(isa, opcode);
   if (desc->ndst == 0)
            /* FIXME: support 3-src instructions */
   unsigned num_sources = brw_num_sources_from_inst(isa, inst);
            enum brw_reg_type dst_type = brw_inst_dst_type(devinfo, inst);
            if (num_sources == 1)
                     return types_are_mixed_float(src0_type, src1_type) ||
            }
      /**
   * Returns whether an instruction is an explicit or implicit conversion
   * to/from byte.
   */
   static bool
   is_byte_conversion(const struct brw_isa_info *isa,
         {
                        unsigned num_sources = brw_num_sources_from_inst(isa, inst);
            if (dst_type != src0_type &&
      (type_sz(dst_type) == 1 || type_sz(src0_type) == 1)) {
      } else if (num_sources > 1) {
      enum brw_reg_type src1_type = brw_inst_src1_type(devinfo, inst);
   return dst_type != src1_type &&
                  }
      /**
   * Checks restrictions listed in "General Restrictions Based on Operand Types"
   * in the "Register Region Restrictions" section.
   */
   static struct string
   general_restrictions_based_on_operand_types(const struct brw_isa_info *isa,
         {
               const struct opcode_desc *desc =
         unsigned num_sources = brw_num_sources_from_inst(isa, inst);
   unsigned exec_size = 1 << brw_inst_exec_size(devinfo, inst);
            if (inst_is_send(isa, inst))
            if (devinfo->ver >= 11) {
      if (num_sources == 3) {
      ERROR_IF(brw_reg_type_to_size(brw_inst_3src_a1_src1_type(devinfo, inst)) == 1 ||
            brw_reg_type_to_size(brw_inst_3src_a1_src2_type(devinfo, inst)) == 1,
   }
   if (num_sources == 2) {
      ERROR_IF(brw_reg_type_to_size(brw_inst_src1_type(devinfo, inst)) == 1,
                                 if (num_sources == 3) {
      if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1)
         else
      } else {
                  ERROR_IF(dst_type == BRW_REGISTER_TYPE_DF &&
                  ERROR_IF((dst_type == BRW_REGISTER_TYPE_Q ||
            dst_type == BRW_REGISTER_TYPE_UQ) &&
         for (unsigned s = 0; s < num_sources; s++) {
      enum brw_reg_type src_type;
   if (num_sources == 3) {
      if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      switch (s) {
   case 0: src_type = brw_inst_3src_a1_src0_type(devinfo, inst); break;
   case 1: src_type = brw_inst_3src_a1_src1_type(devinfo, inst); break;
   case 2: src_type = brw_inst_3src_a1_src2_type(devinfo, inst); break;
   default: unreachable("invalid src");
      } else {
            } else {
      switch (s) {
   case 0: src_type = brw_inst_src0_type(devinfo, inst); break;
   case 1: src_type = brw_inst_src1_type(devinfo, inst); break;
   default: unreachable("invalid src");
               ERROR_IF(src_type == BRW_REGISTER_TYPE_DF &&
                  ERROR_IF((src_type == BRW_REGISTER_TYPE_Q ||
            src_type == BRW_REGISTER_TYPE_UQ) &&
            if (num_sources == 3)
            if (exec_size == 1)
            if (desc->ndst == 0)
            /* The PRMs say:
   *
   *    Where n is the largest element size in bytes for any source or
   *    destination operand type, ExecSize * n must be <= 64.
   *
   * But we do not attempt to enforce it, because it is implied by other
   * rules:
   *
   *    - that the destination stride must match the execution data type
   *    - sources may not span more than two adjacent GRF registers
   *    - destination may not span more than two adjacent GRF registers
   *
   * In fact, checking it would weaken testing of the other rules.
            unsigned dst_stride = STRIDE(brw_inst_dst_hstride(devinfo, inst));
   bool dst_type_is_byte =
      inst_dst_type(isa, inst) == BRW_REGISTER_TYPE_B ||
         if (dst_type_is_byte) {
      if (is_packed(exec_size * dst_stride, exec_size, dst_stride)) {
      if (!inst_is_raw_move(isa, inst))
                        unsigned exec_type = execution_type(isa, inst);
   unsigned exec_type_size = brw_reg_type_to_size(exec_type);
            /* On IVB/BYT, region parameters and execution size for DF are in terms of
   * 32-bit elements, so they are doubled. For evaluating the validity of an
   * instruction, we halve them.
   */
   if (devinfo->verx10 == 70 &&
      exec_type_size == 8 && dst_type_size == 4)
         if (is_byte_conversion(isa, inst)) {
      /* From the BDW+ PRM, Volume 2a, Command Reference, Instructions - MOV:
   *
   *    "There is no direct conversion from B/UB to DF or DF to B/UB.
   *     There is no direct conversion from B/UB to Q/UQ or Q/UQ to B/UB."
   *
   * Even if these restrictions are listed for the MOV instruction, we
   * validate this more generally, since there is the possibility
   * of implicit conversions from other instructions.
   */
   enum brw_reg_type src0_type = brw_inst_src0_type(devinfo, inst);
   enum brw_reg_type src1_type = num_sources > 1 ?
            ERROR_IF(type_sz(dst_type) == 1 &&
                        ERROR_IF(type_sz(dst_type) == 8 &&
            (type_sz(src0_type) == 1 ||
            if (is_half_float_conversion(isa, inst)) {
      /**
   * A helper to validate used in the validation of the following restriction
   * from the BDW+ PRM, Volume 2a, Command Reference, Instructions - MOV:
   *
   *    "There is no direct conversion from HF to DF or DF to HF.
   *     There is no direct conversion from HF to Q/UQ or Q/UQ to HF."
   *
   * Even if these restrictions are listed for the MOV instruction, we
   * validate this more generally, since there is the possibility
   * of implicit conversions from other instructions, such us implicit
   * conversion from integer to HF with the ADD instruction in SKL+.
   */
   enum brw_reg_type src0_type = brw_inst_src0_type(devinfo, inst);
   enum brw_reg_type src1_type = num_sources > 1 ?
         ERROR_IF(dst_type == BRW_REGISTER_TYPE_HF &&
                        ERROR_IF(type_sz(dst_type) == 8 &&
                        /* From the BDW+ PRM:
   *
   *   "Conversion between Integer and HF (Half Float) must be
   *    DWord-aligned and strided by a DWord on the destination."
   *
   * Also, the above restrictions seems to be expanded on CHV and SKL+ by:
   *
   *   "There is a relaxed alignment rule for word destinations. When
   *    the destination type is word (UW, W, HF), destination data types
   *    can be aligned to either the lowest word or the second lowest
   *    word of the execution channel. This means the destination data
   *    words can be either all in the even word locations or all in the
   *    odd word locations."
   *
   * We do not implement the second rule as is though, since empirical
   * testing shows inconsistencies:
   *   - It suggests that packed 16-bit is not allowed, which is not true.
   *   - It suggests that conversions from Q/DF to W (which need to be
   *     64-bit aligned on the destination) are not possible, which is
   *     not true.
   *
   * So from this rule we only validate the implication that conversions
   * from F to HF need to be DWord strided (except in Align1 mixed
   * float mode where packed fp16 destination is allowed so long as the
   * destination is oword-aligned).
   *
   * Finally, we only validate this for Align1 because Align16 always
   * requires packed destinations, so these restrictions can't possibly
   * apply to Align16 mode.
   */
   if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      if ((dst_type == BRW_REGISTER_TYPE_HF &&
      (brw_reg_type_is_integer(src0_type) ||
         (brw_reg_type_is_integer(dst_type) &&
   (src0_type == BRW_REGISTER_TYPE_HF ||
         ERROR_IF(dst_stride * dst_type_size != 4,
                  unsigned subreg = brw_inst_dst_da1_subreg_nr(devinfo, inst);
   ERROR_IF(subreg % 4 != 0,
            } else if ((devinfo->platform == INTEL_PLATFORM_CHV ||
                  unsigned subreg = brw_inst_dst_da1_subreg_nr(devinfo, inst);
   ERROR_IF(dst_stride != 2 &&
            !(is_mixed_float(isa, inst) &&
   dst_stride == 1 && subreg % 16 == 0),
   "Conversions to HF must have either all words in even "
                  /* There are special regioning rules for mixed-float mode in CHV and SKL that
   * override the general rule for the ratio of sizes of the destination type
   * and the execution type. We will add validation for those in a later patch.
   */
   bool validate_dst_size_and_exec_size_ratio =
      !is_mixed_float(isa, inst) ||
         if (validate_dst_size_and_exec_size_ratio &&
      exec_type_size > dst_type_size) {
   if (!(dst_type_is_byte && inst_is_raw_move(isa, inst))) {
      ERROR_IF(dst_stride * dst_type_size != exec_type_size,
                              if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1 &&
      brw_inst_dst_address_mode(devinfo, inst) == BRW_ADDRESS_DIRECT) {
   /* The i965 PRM says:
   *
   *    Implementation Restriction: The relaxed alignment rule for byte
   *    destination (#10.5) is not supported.
   */
   if (devinfo->verx10 >= 45 && dst_type_is_byte) {
      ERROR_IF(subreg % exec_type_size != 0 &&
            subreg % exec_type_size != 1,
   "Destination subreg must be aligned to the size of the "
   } else {
      ERROR_IF(subreg % exec_type_size != 0,
                              }
      /**
   * Checks restrictions listed in "General Restrictions on Regioning Parameters"
   * in the "Register Region Restrictions" section.
   */
   static struct string
   general_restrictions_on_region_parameters(const struct brw_isa_info *isa,
         {
               const struct opcode_desc *desc =
         unsigned num_sources = brw_num_sources_from_inst(isa, inst);
   unsigned exec_size = 1 << brw_inst_exec_size(devinfo, inst);
            if (num_sources == 3)
            /* Split sends don't have the bits in the instruction to encode regions so
   * there's nothing to check.
   */
   if (inst_is_split_send(isa, inst))
            if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_16) {
      if (desc->ndst != 0 && !dst_is_null(devinfo, inst))
                  if (num_sources >= 1) {
      if (devinfo->verx10 >= 75) {
      ERROR_IF(brw_inst_src0_reg_file(devinfo, inst) != BRW_IMMEDIATE_VALUE &&
            brw_inst_src0_vstride(devinfo, inst) != BRW_VERTICAL_STRIDE_0 &&
   brw_inst_src0_vstride(devinfo, inst) != BRW_VERTICAL_STRIDE_2 &&
   } else {
      ERROR_IF(brw_inst_src0_reg_file(devinfo, inst) != BRW_IMMEDIATE_VALUE &&
            brw_inst_src0_vstride(devinfo, inst) != BRW_VERTICAL_STRIDE_0 &&
               if (num_sources == 2) {
      if (devinfo->verx10 >= 75) {
      ERROR_IF(brw_inst_src1_reg_file(devinfo, inst) != BRW_IMMEDIATE_VALUE &&
            brw_inst_src1_vstride(devinfo, inst) != BRW_VERTICAL_STRIDE_0 &&
   brw_inst_src1_vstride(devinfo, inst) != BRW_VERTICAL_STRIDE_2 &&
   } else {
      ERROR_IF(brw_inst_src1_reg_file(devinfo, inst) != BRW_IMMEDIATE_VALUE &&
            brw_inst_src1_vstride(devinfo, inst) != BRW_VERTICAL_STRIDE_0 &&
                           for (unsigned i = 0; i < num_sources; i++) {
      unsigned vstride, width, hstride, element_size, subreg;
      #define DO_SRC(n)                                                              \
         if (brw_inst_src ## n ## _reg_file(devinfo, inst) ==                     \
      BRW_IMMEDIATE_VALUE)                                                 \
   continue;                                                             \
      vstride = STRIDE(brw_inst_src ## n ## _vstride(devinfo, inst));          \
   width = WIDTH(brw_inst_src ## n ## _width(devinfo, inst));               \
   hstride = STRIDE(brw_inst_src ## n ## _hstride(devinfo, inst));          \
   type = brw_inst_src ## n ## _type(devinfo, inst);                        \
   element_size = brw_reg_type_to_size(type);                               \
            if (i == 0) {
         } else {
         #undef DO_SRC
            /* On IVB/BYT, region parameters and execution size for DF are in terms of
   * 32-bit elements, so they are doubled. For evaluating the validity of an
   * instruction, we halve them.
   */
   if (devinfo->verx10 == 70 &&
                  /* ExecSize must be greater than or equal to Width. */
   ERROR_IF(exec_size < width, "ExecSize must be greater than or equal "
            /* If ExecSize = Width and HorzStride ≠ 0,
   * VertStride must be set to Width * HorzStride.
   */
   if (exec_size == width && hstride != 0) {
      ERROR_IF(vstride != width * hstride,
                     /* If Width = 1, HorzStride must be 0 regardless of the values of
   * ExecSize and VertStride.
   */
   if (width == 1) {
      ERROR_IF(hstride != 0,
                     /* If ExecSize = Width = 1, both VertStride and HorzStride must be 0. */
   if (exec_size == 1 && width == 1) {
      ERROR_IF(vstride != 0 || hstride != 0,
                     /* If VertStride = HorzStride = 0, Width must be 1 regardless of the
   * value of ExecSize.
   */
   if (vstride == 0 && hstride == 0) {
      ERROR_IF(width != 1,
                     /* VertStride must be used to cross GRF register boundaries. This rule
   * implies that elements within a 'Width' cannot cross GRF boundaries.
   */
   const uint64_t mask = (1ULL << element_size) - 1;
            for (int y = 0; y < exec_size / width; y++) {
                     for (int x = 0; x < width; x++) {
      access_mask |= mask << (offset % 64);
                        if ((uint32_t)access_mask != 0 && (access_mask >> 32) != 0) {
      ERROR("VertStride must be used to cross GRF register boundaries");
                     /* Dst.HorzStride must not be 0. */
   if (desc->ndst != 0 && !dst_is_null(devinfo, inst)) {
      ERROR_IF(brw_inst_dst_hstride(devinfo, inst) == BRW_HORIZONTAL_STRIDE_0,
                  }
      static struct string
   special_restrictions_for_mixed_float_mode(const struct brw_isa_info *isa,
         {
                        const unsigned opcode = brw_inst_opcode(isa, inst);
   const unsigned num_sources = brw_num_sources_from_inst(isa, inst);
   if (num_sources >= 3)
            if (!is_mixed_float(isa, inst))
            unsigned exec_size = 1 << brw_inst_exec_size(devinfo, inst);
            enum brw_reg_type src0_type = brw_inst_src0_type(devinfo, inst);
   enum brw_reg_type src1_type = num_sources > 1 ?
                  unsigned dst_stride = STRIDE(brw_inst_dst_hstride(devinfo, inst));
            /* From the SKL PRM, Special Restrictions for Handling Mixed Mode
   * Float Operations:
   *
   *    "Indirect addressing on source is not supported when source and
   *     destination data types are mixed float."
   */
   ERROR_IF(brw_inst_src0_address_mode(devinfo, inst) != BRW_ADDRESS_DIRECT ||
            (num_sources > 1 &&
   brw_inst_src1_address_mode(devinfo, inst) != BRW_ADDRESS_DIRECT),
         /* From the SKL PRM, Special Restrictions for Handling Mixed Mode
   * Float Operations:
   *
   *    "No SIMD16 in mixed mode when destination is f32. Instruction
   *     execution size must be no more than 8."
   */
   ERROR_IF(exec_size > 8 && dst_type == BRW_REGISTER_TYPE_F,
                  if (is_align16) {
      /* From the SKL PRM, Special Restrictions for Handling Mixed Mode
   * Float Operations:
   *
   *   "In Align16 mode, when half float and float data types are mixed
   *    between source operands OR between source and destination operands,
   *    the register content are assumed to be packed."
   *
   * Since Align16 doesn't have a concept of horizontal stride (or width),
   * it means that vertical stride must always be 4, since 0 and 2 would
   * lead to replicated data, and any other value is disallowed in Align16.
   */
   ERROR_IF(brw_inst_src0_vstride(devinfo, inst) != BRW_VERTICAL_STRIDE_4,
            ERROR_IF(num_sources >= 2 &&
                  /* From the SKL PRM, Special Restrictions for Handling Mixed Mode
   * Float Operations:
   *
   *   "For Align16 mixed mode, both input and output packed f16 data
   *    must be oword aligned, no oword crossing in packed f16."
   *
   * The previous rule requires that Align16 operands are always packed,
   * and since there is only one bit for Align16 subnr, which represents
   * offsets 0B and 16B, this rule is always enforced and we don't need to
   * validate it.
            /* From the SKL PRM, Special Restrictions for Handling Mixed Mode
   * Float Operations:
   *
   *    "No SIMD16 in mixed mode when destination is packed f16 for both
   *     Align1 and Align16."
   *
   * And:
   *
   *   "In Align16 mode, when half float and float data types are mixed
   *    between source operands OR between source and destination operands,
   *    the register content are assumed to be packed."
   *
   * Which implies that SIMD16 is not available in Align16. This is further
   * confirmed by:
   *
   *    "For Align16 mixed mode, both input and output packed f16 data
   *     must be oword aligned, no oword crossing in packed f16"
   *
   * Since oword-aligned packed f16 data would cross oword boundaries when
   * the execution size is larger than 8.
   */
            /* From the SKL PRM, Special Restrictions for Handling Mixed Mode
   * Float Operations:
   *
   *    "No accumulator read access for Align16 mixed float."
   */
   ERROR_IF(inst_uses_src_acc(isa, inst),
      } else {
               /* From the SKL PRM, Special Restrictions for Handling Mixed Mode
   * Float Operations:
   *
   *    "No SIMD16 in mixed mode when destination is packed f16 for both
   *     Align1 and Align16."
   */
   ERROR_IF(exec_size > 8 && dst_is_packed &&
                        /* From the SKL PRM, Special Restrictions for Handling Mixed Mode
   * Float Operations:
   *
   *    "Math operations for mixed mode:
   *     - In Align1, f16 inputs need to be strided"
   */
   if (opcode == BRW_OPCODE_MATH) {
      if (src0_type == BRW_REGISTER_TYPE_HF) {
      ERROR_IF(STRIDE(brw_inst_src0_hstride(devinfo, inst)) <= 1,
               if (num_sources >= 2 && src1_type == BRW_REGISTER_TYPE_HF) {
      ERROR_IF(STRIDE(brw_inst_src1_hstride(devinfo, inst)) <= 1,
                  if (dst_type == BRW_REGISTER_TYPE_HF && dst_stride == 1) {
      /* From the SKL PRM, Special Restrictions for Handling Mixed Mode
   * Float Operations:
   *
   *    "In Align1, destination stride can be smaller than execution
   *     type. When destination is stride of 1, 16 bit packed data is
   *     updated on the destination. However, output packed f16 data
   *     must be oword aligned, no oword crossing in packed f16."
   *
   * The requirement of not crossing oword boundaries for 16-bit oword
   * aligned data means that execution size is limited to 8.
   */
   unsigned subreg;
   if (brw_inst_dst_address_mode(devinfo, inst) == BRW_ADDRESS_DIRECT)
         else
         ERROR_IF(subreg % 16 != 0,
               ERROR_IF(exec_size > 8,
                  /* From the SKL PRM, Special Restrictions for Handling Mixed Mode
   * Float Operations:
   *
   *    "When source is float or half float from accumulator register and
   *     destination is half float with a stride of 1, the source must
   *     register aligned. i.e., source must have offset zero."
   *
   * Align16 mixed float mode doesn't allow accumulator access on sources,
   * so we only need to check this for Align1.
   */
   if (src0_is_acc(devinfo, inst) &&
      (src0_type == BRW_REGISTER_TYPE_F ||
   src0_type == BRW_REGISTER_TYPE_HF)) {
   ERROR_IF(brw_inst_src0_da1_subreg_nr(devinfo, inst) != 0,
                        if (num_sources > 1 &&
      src1_is_acc(devinfo, inst) &&
   (src1_type == BRW_REGISTER_TYPE_F ||
   src1_type == BRW_REGISTER_TYPE_HF)) {
   ERROR_IF(brw_inst_src1_da1_subreg_nr(devinfo, inst) != 0,
                        /* From the SKL PRM, Special Restrictions for Handling Mixed Mode
   * Float Operations:
   *
   *    "No swizzle is allowed when an accumulator is used as an implicit
   *     source or an explicit source in an instruction. i.e. when
   *     destination is half float with an implicit accumulator source,
   *     destination stride needs to be 2."
   *
   * FIXME: it is not quite clear what the first sentence actually means
   *        or its link to the implication described after it, so we only
   *        validate the explicit implication, which is clearly described.
   */
   if (dst_type == BRW_REGISTER_TYPE_HF &&
      inst_uses_src_acc(isa, inst)) {
   ERROR_IF(dst_stride != 2,
            "Mixed float mode with implicit/explicit accumulator "
                  }
      /**
   * Creates an \p access_mask for an \p exec_size, \p element_size, and a region
   *
   * An \p access_mask is a 32-element array of uint64_t, where each uint64_t is
   * a bitmask of bytes accessed by the region.
   *
   * For instance the access mask of the source gX.1<4,2,2>F in an exec_size = 4
   * instruction would be
   *
   *    access_mask[0] = 0x00000000000000F0
   *    access_mask[1] = 0x000000000000F000
   *    access_mask[2] = 0x0000000000F00000
   *    access_mask[3] = 0x00000000F0000000
   *    access_mask[4-31] = 0
   *
   * because the first execution channel accesses bytes 7-4 and the second
   * execution channel accesses bytes 15-12, etc.
   */
   static void
   align1_access_mask(uint64_t access_mask[static 32],
               {
      const uint64_t mask = (1ULL << element_size) - 1;
   unsigned rowbase = subreg;
            for (int y = 0; y < exec_size / width; y++) {
               for (int x = 0; x < width; x++) {
      access_mask[element++] = mask << (offset % 64);
                              }
      /**
   * Returns the number of registers accessed according to the \p access_mask
   */
   static int
   registers_read(const uint64_t access_mask[static 32])
   {
               for (unsigned i = 0; i < 32; i++) {
      if (access_mask[i] > 0xFFFFFFFF) {
         } else if (access_mask[i]) {
                        }
      /**
   * Checks restrictions listed in "Region Alignment Rules" in the "Register
   * Region Restrictions" section.
   */
   static struct string
   region_alignment_rules(const struct brw_isa_info *isa,
         {
      const struct intel_device_info *devinfo = isa->devinfo;
   const struct opcode_desc *desc =
         unsigned num_sources = brw_num_sources_from_inst(isa, inst);
   unsigned exec_size = 1 << brw_inst_exec_size(devinfo, inst);
   uint64_t dst_access_mask[32], src0_access_mask[32], src1_access_mask[32];
            if (num_sources == 3)
            if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_16)
            if (inst_is_send(isa, inst))
            memset(dst_access_mask, 0, sizeof(dst_access_mask));
   memset(src0_access_mask, 0, sizeof(src0_access_mask));
            for (unsigned i = 0; i < num_sources; i++) {
      unsigned vstride, width, hstride, element_size, subreg;
            /* In Direct Addressing mode, a source cannot span more than 2 adjacent
   * GRF registers.
      #define DO_SRC(n)                                                              \
         if (brw_inst_src ## n ## _address_mode(devinfo, inst) !=                 \
      BRW_ADDRESS_DIRECT)                                                  \
   continue;                                                             \
      if (brw_inst_src ## n ## _reg_file(devinfo, inst) ==                     \
      BRW_IMMEDIATE_VALUE)                                                 \
   continue;                                                             \
      vstride = STRIDE(brw_inst_src ## n ## _vstride(devinfo, inst));          \
   width = WIDTH(brw_inst_src ## n ## _width(devinfo, inst));               \
   hstride = STRIDE(brw_inst_src ## n ## _hstride(devinfo, inst));          \
   type = brw_inst_src ## n ## _type(devinfo, inst);                        \
   element_size = brw_reg_type_to_size(type);                               \
   subreg = brw_inst_src ## n ## _da1_subreg_nr(devinfo, inst);             \
   align1_access_mask(src ## n ## _access_mask,                             \
                  if (i == 0) {
         } else {
         #undef DO_SRC
            unsigned num_vstride = exec_size / width;
   unsigned num_hstride = width;
   unsigned vstride_elements = (num_vstride - 1) * vstride;
   unsigned hstride_elements = (num_hstride - 1) * hstride;
   unsigned offset = (vstride_elements + hstride_elements) * element_size +
         ERROR_IF(offset >= 64 * reg_unit(devinfo),
               if (desc->ndst == 0 || dst_is_null(devinfo, inst))
            unsigned stride = STRIDE(brw_inst_dst_hstride(devinfo, inst));
   enum brw_reg_type dst_type = inst_dst_type(isa, inst);
   unsigned element_size = brw_reg_type_to_size(dst_type);
   unsigned subreg = brw_inst_dst_da1_subreg_nr(devinfo, inst);
   unsigned offset = ((exec_size - 1) * stride * element_size) + subreg;
   ERROR_IF(offset >= 64 * reg_unit(devinfo),
            if (error_msg.str)
            /* On IVB/BYT, region parameters and execution size for DF are in terms of
   * 32-bit elements, so they are doubled. For evaluating the validity of an
   * instruction, we halve them.
   */
   if (devinfo->verx10 == 70 &&
      element_size == 8)
         align1_access_mask(dst_access_mask, exec_size, element_size, subreg,
                        unsigned dst_regs = registers_read(dst_access_mask);
   unsigned src0_regs = registers_read(src0_access_mask);
            /* The SNB, IVB, HSW, BDW, and CHV PRMs say:
   *
   *    When an instruction has a source region spanning two registers and a
   *    destination region contained in one register, the number of elements
   *    must be the same between two sources and one of the following must be
   *    true:
   *
   *       1. The destination region is entirely contained in the lower OWord
   *          of a register.
   *       2. The destination region is entirely contained in the upper OWord
   *          of a register.
   *       3. The destination elements are evenly split between the two OWords
   *          of a register.
   */
   if (devinfo->ver <= 8) {
      if (dst_regs == 1 && (src0_regs == 2 || src1_regs == 2)) {
               for (unsigned i = 0; i < exec_size; i++) {
      if (dst_access_mask[i] > 0x0000FFFF) {
         } else {
      assert(dst_access_mask[i] != 0);
                  ERROR_IF(lower_oword_writes != 0 &&
            upper_oword_writes != 0 &&
   upper_oword_writes != lower_oword_writes,
               /* The IVB and HSW PRMs say:
   *
   *    When an instruction has a source region that spans two registers and
   *    the destination spans two registers, the destination elements must be
   *    evenly split between the two registers [...]
   *
   * The SNB PRM contains similar wording (but written in a much more
   * confusing manner).
   *
   * The BDW PRM says:
   *
   *    When destination spans two registers, the source may be one or two
   *    registers. The destination elements must be evenly split between the
   *    two registers.
   *
   * The SKL PRM says:
   *
   *    When destination of MATH instruction spans two registers, the
   *    destination elements must be evenly split between the two registers.
   *
   * It is not known whether this restriction applies to KBL other Gens after
   * SKL.
   */
   if (devinfo->ver <= 8 ||
               /* Nothing explicitly states that on Gen < 8 elements must be evenly
   * split between two destination registers in the two exceptional
   * source-region-spans-one-register cases, but since Broadwell requires
   * evenly split writes regardless of source region, we assume that it was
   * an oversight and require it.
   */
   if (dst_regs == 2) {
               for (unsigned i = 0; i < exec_size; i++) {
      if (dst_access_mask[i] > 0xFFFFFFFF) {
         } else {
      assert(dst_access_mask[i] != 0);
                  ERROR_IF(upper_reg_writes != lower_reg_writes,
                        /* The IVB and HSW PRMs say:
   *
   *    When an instruction has a source region that spans two registers and
   *    the destination spans two registers, the destination elements must be
   *    evenly split between the two registers and each destination register
   *    must be entirely derived from one source register.
   *
   *    Note: In such cases, the regioning parameters must ensure that the
   *    offset from the two source registers is the same.
   *
   * The SNB PRM contains similar wording (but written in a much more
   * confusing manner).
   *
   * There are effectively three rules stated here:
   *
   *    For an instruction with a source and a destination spanning two
   *    registers,
   *
   *       (1) destination elements must be evenly split between the two
   *           registers
   *       (2) all destination elements in a register must be derived
   *           from one source register
   *       (3) the offset (i.e. the starting location in each of the two
   *           registers spanned by a region) must be the same in the two
   *           registers spanned by a region
   *
   * It is impossible to violate rule (1) without violating (2) or (3), so we
   * do not attempt to validate it.
   */
   if (devinfo->ver <= 7 && dst_regs == 2) {
      #define DO_SRC(n)                                                             \
            if (src ## n ## _regs <= 1)                                          \
      continue;                                                         \
      for (unsigned i = 0; i < exec_size; i++) {                           \
      if ((dst_access_mask[i] > 0xFFFFFFFF) !=                          \
      (src ## n ## _access_mask[i] > 0xFFFFFFFF)) {                 \
   ERROR("Each destination register must be entirely derived "    \
               }                                                                    \
         unsigned offset_0 =                                                  \
         unsigned offset_1 = offset_0;                                        \
         for (unsigned i = 0; i < exec_size; i++) {                           \
      if (src ## n ## _access_mask[i] > 0xFFFFFFFF) {                   \
      offset_1 = __builtin_ctzll(src ## n ## _access_mask[i]) - 32;  \
         }                                                                    \
         ERROR_IF(num_sources == 2 && offset_0 != offset_1,                   \
                  if (i == 0) {
         } else {
      #undef DO_SRC
                     /* The IVB and HSW PRMs say:
   *
   *    When destination spans two registers, the source MUST span two
   *    registers. The exception to the above rule:
   *        1. When source is scalar, the source registers are not
   *           incremented.
   *        2. When source is packed integer Word and destination is packed
   *           integer DWord, the source register is not incremented by the
   *           source sub register is incremented.
   *
   * The SNB PRM does not contain this rule, but the internal documentation
   * indicates that it applies to SNB as well. We assume that the rule applies
   * to Gen <= 5 although their PRMs do not state it.
   *
   * While the documentation explicitly says in exception (2) that the
   * destination must be an integer DWord, the hardware allows at least a
   * float destination type as well. We emit such instructions from
   *
   *    fs_visitor::emit_interpolation_setup_gfx6
   *    fs_visitor::emit_fragcoord_interpolation
   *
   * and have for years with no ill effects.
   *
   * Additionally the simulator source code indicates that the real condition
   * is that the size of the destination type is 4 bytes.
   *
   * HSW PRMs also add a note to the second exception:
   *  "When lower 8 channels are disabled, the sub register of source1
   *   operand is not incremented. If the lower 8 channels are expected
   *   to be disabled, say by predication, the instruction must be split
   *   into pair of simd8 operations."
   *
   * We can't reliably know if the channels won't be disabled due to,
   * for example, IMASK. So, play it safe and disallow packed-word exception
   * for src1.
   */
   if (devinfo->ver <= 7 && dst_regs == 2) {
      enum brw_reg_type dst_type = inst_dst_type(isa, inst);
   bool dst_is_packed_dword =
                  #define DO_SRC(n)                                                                  \
            unsigned vstride, width, hstride;                                         \
   vstride = STRIDE(brw_inst_src ## n ## _vstride(devinfo, inst));           \
   width = WIDTH(brw_inst_src ## n ## _width(devinfo, inst));                \
   hstride = STRIDE(brw_inst_src ## n ## _hstride(devinfo, inst));           \
   bool src ## n ## _is_packed_word =                                        \
      n != 1 && is_packed(vstride, width, hstride) &&                        \
   (brw_inst_src ## n ## _type(devinfo, inst) == BRW_REGISTER_TYPE_W ||   \
   brw_inst_src ## n ## _type(devinfo, inst) == BRW_REGISTER_TYPE_UW);   \
      ERROR_IF(src ## n ## _regs == 1 &&                                        \
            !src ## n ## _has_scalar_region(devinfo, inst) &&                \
   !(dst_is_packed_dword && src ## n ## _is_packed_word),           \
               if (i == 0) {
         } else {
      #undef DO_SRC
                        }
      static struct string
   vector_immediate_restrictions(const struct brw_isa_info *isa,
         {
               unsigned num_sources = brw_num_sources_from_inst(isa, inst);
            if (num_sources == 3 || num_sources == 0)
            unsigned file = num_sources == 1 ?
               if (file != BRW_IMMEDIATE_VALUE)
            enum brw_reg_type dst_type = inst_dst_type(isa, inst);
   unsigned dst_type_size = brw_reg_type_to_size(dst_type);
   unsigned dst_subreg = brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1 ?
         unsigned dst_stride = STRIDE(brw_inst_dst_hstride(devinfo, inst));
   enum brw_reg_type type = num_sources == 1 ?
                  /* The PRMs say:
   *
   *    When an immediate vector is used in an instruction, the destination
   *    must be 128-bit aligned with destination horizontal stride equivalent
   *    to a word for an immediate integer vector (v) and equivalent to a
   *    DWord for an immediate float vector (vf).
   *
   * The text has not been updated for the addition of the immediate unsigned
   * integer vector type (uv) on SNB, but presumably the same restriction
   * applies.
   */
   switch (type) {
   case BRW_REGISTER_TYPE_V:
   case BRW_REGISTER_TYPE_UV:
   case BRW_REGISTER_TYPE_VF:
      ERROR_IF(dst_subreg % (128 / 8) != 0,
                  if (type == BRW_REGISTER_TYPE_VF) {
      ERROR_IF(dst_type_size * dst_stride != 4,
            } else {
      ERROR_IF(dst_type_size * dst_stride != 2,
            }
      default:
                     }
      static struct string
   special_requirements_for_handling_double_precision_data_types(
               {
               unsigned num_sources = brw_num_sources_from_inst(isa, inst);
            if (num_sources == 3 || num_sources == 0)
            /* Split sends don't have types so there's no doubles there. */
   if (inst_is_split_send(isa, inst))
            enum brw_reg_type exec_type = execution_type(isa, inst);
            enum brw_reg_file dst_file = brw_inst_dst_reg_file(devinfo, inst);
   enum brw_reg_type dst_type = inst_dst_type(isa, inst);
   unsigned dst_type_size = brw_reg_type_to_size(dst_type);
   unsigned dst_hstride = STRIDE(brw_inst_dst_hstride(devinfo, inst));
   unsigned dst_reg = brw_inst_dst_da_reg_nr(devinfo, inst);
   unsigned dst_subreg = brw_inst_dst_da1_subreg_nr(devinfo, inst);
            bool is_integer_dword_multiply =
      devinfo->ver >= 8 &&
   brw_inst_opcode(isa, inst) == BRW_OPCODE_MUL &&
   (brw_inst_src0_type(devinfo, inst) == BRW_REGISTER_TYPE_D ||
   brw_inst_src0_type(devinfo, inst) == BRW_REGISTER_TYPE_UD) &&
   (brw_inst_src1_type(devinfo, inst) == BRW_REGISTER_TYPE_D ||
         const bool is_double_precision =
            for (unsigned i = 0; i < num_sources; i++) {
      unsigned vstride, width, hstride, type_size, reg, subreg, address_mode;
   bool is_scalar_region;
   enum brw_reg_file file;
      #define DO_SRC(n)                                                              \
         if (brw_inst_src ## n ## _reg_file(devinfo, inst) ==                     \
      BRW_IMMEDIATE_VALUE)                                                 \
   continue;                                                             \
      is_scalar_region = src ## n ## _has_scalar_region(devinfo, inst);        \
   vstride = STRIDE(brw_inst_src ## n ## _vstride(devinfo, inst));          \
   width = WIDTH(brw_inst_src ## n ## _width(devinfo, inst));               \
   hstride = STRIDE(brw_inst_src ## n ## _hstride(devinfo, inst));          \
   file = brw_inst_src ## n ## _reg_file(devinfo, inst);                    \
   type = brw_inst_src ## n ## _type(devinfo, inst);                        \
   type_size = brw_reg_type_to_size(type);                                  \
   reg = brw_inst_src ## n ## _da_reg_nr(devinfo, inst);                    \
   subreg = brw_inst_src ## n ## _da1_subreg_nr(devinfo, inst);             \
            if (i == 0) {
         } else {
         #undef DO_SRC
            const unsigned src_stride = (hstride ? hstride : vstride) * type_size;
            /* The PRMs say that for CHV, BXT:
   *
   *    When source or destination datatype is 64b or operation is integer
   *    DWord multiply, regioning in Align1 must follow these rules:
   *
   *    1. Source and Destination horizontal stride must be aligned to the
   *       same qword.
   *    2. Regioning must ensure Src.Vstride = Src.Width * Src.Hstride.
   *    3. Source and Destination offset must be the same, except the case
   *       of scalar source.
   *
   * We assume that the restriction applies to GLK as well.
   */
   if (is_double_precision &&
      brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1 &&
   (devinfo->platform == INTEL_PLATFORM_CHV || intel_device_info_is_9lp(devinfo))) {
   ERROR_IF(!is_scalar_region &&
            (src_stride % 8 != 0 ||
   dst_stride % 8 != 0 ||
               ERROR_IF(vstride != width * hstride,
                  ERROR_IF(!is_scalar_region && dst_subreg != subreg,
                     /* The PRMs say that for CHV, BXT:
   *
   *    When source or destination datatype is 64b or operation is integer
   *    DWord multiply, indirect addressing must not be used.
   *
   * We assume that the restriction applies to GLK as well.
   */
   if (is_double_precision &&
      (devinfo->platform == INTEL_PLATFORM_CHV || intel_device_info_is_9lp(devinfo))) {
   ERROR_IF(BRW_ADDRESS_REGISTER_INDIRECT_REGISTER == address_mode ||
            BRW_ADDRESS_REGISTER_INDIRECT_REGISTER == dst_address_mode,
            /* The PRMs say that for CHV, BXT:
   *
   *    ARF registers must never be used with 64b datatype or when
   *    operation is integer DWord multiply.
   *
   * We assume that the restriction applies to GLK as well.
   *
   * We assume that the restriction does not apply to the null register.
   */
   if (is_double_precision &&
      (devinfo->platform == INTEL_PLATFORM_CHV ||
   intel_device_info_is_9lp(devinfo))) {
   ERROR_IF(brw_inst_opcode(isa, inst) == BRW_OPCODE_MAC ||
            brw_inst_acc_wr_control(devinfo, inst) ||
   (BRW_ARCHITECTURE_REGISTER_FILE == file &&
   reg != BRW_ARF_NULL) ||
   (BRW_ARCHITECTURE_REGISTER_FILE == dst_file &&
   dst_reg != BRW_ARF_NULL),
            /* From the hardware spec section "Register Region Restrictions":
   *
   * There are two rules:
   *
   * "In case of all floating point data types used in destination:" and
   *
   * "In case where source or destination datatype is 64b or operation is
   *  integer DWord multiply:"
   *
   * both of which list the same restrictions:
   *
   *  "1. Register Regioning patterns where register data bit location
   *      of the LSB of the channels are changed between source and
   *      destination are not supported on Src0 and Src1 except for
   *      broadcast of a scalar.
   *
   *   2. Explicit ARF registers except null and accumulator must not be
   *      used."
   */
   if (devinfo->verx10 >= 125 &&
      (brw_reg_type_is_floating_point(dst_type) ||
   is_double_precision)) {
   ERROR_IF(!is_scalar_region &&
            BRW_ADDRESS_REGISTER_INDIRECT_REGISTER != address_mode &&
   (!is_linear(vstride, width, hstride) ||
   src_stride != dst_stride ||
   subreg != dst_subreg),
   "Register Regioning patterns where register data bit "
               ERROR_IF((file == BRW_ARCHITECTURE_REGISTER_FILE &&
            reg != BRW_ARF_NULL && !(reg >= BRW_ARF_ACCUMULATOR && reg < BRW_ARF_FLAG)) ||
   (dst_file == BRW_ARCHITECTURE_REGISTER_FILE &&
   dst_reg != BRW_ARF_NULL && dst_reg != BRW_ARF_ACCUMULATOR),
            /* From the hardware spec section "Register Region Restrictions":
   *
   * "Vx1 and VxH indirect addressing for Float, Half-Float, Double-Float and
   *  Quad-Word data must not be used."
   */
   if (devinfo->verx10 >= 125 &&
      (brw_reg_type_is_floating_point(type) || type_sz(type) == 8)) {
   ERROR_IF(address_mode == BRW_ADDRESS_REGISTER_INDIRECT_REGISTER &&
            vstride == BRW_VERTICAL_STRIDE_ONE_DIMENSIONAL,
               /* The PRMs say that for BDW, SKL:
   *
   *    If Align16 is required for an operation with QW destination and non-QW
   *    source datatypes, the execution size cannot exceed 2.
   *
   * We assume that the restriction applies to all Gfx8+ parts.
   */
   if (is_double_precision && devinfo->ver >= 8) {
      enum brw_reg_type src0_type = brw_inst_src0_type(devinfo, inst);
   enum brw_reg_type src1_type =
         unsigned src0_type_size = brw_reg_type_to_size(src0_type);
            ERROR_IF(brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_16 &&
            dst_type_size == 8 &&
   (src0_type_size != 8 || src1_type_size != 8) &&
   brw_inst_exec_size(devinfo, inst) > BRW_EXECUTE_2,
            /* The PRMs say that for CHV, BXT:
   *
   *    When source or destination datatype is 64b or operation is integer
   *    DWord multiply, DepCtrl must not be used.
   *
   * We assume that the restriction applies to GLK as well.
   */
   if (is_double_precision &&
      (devinfo->platform == INTEL_PLATFORM_CHV || intel_device_info_is_9lp(devinfo))) {
   ERROR_IF(brw_inst_no_dd_check(devinfo, inst) ||
                        }
      static struct string
   instruction_restrictions(const struct brw_isa_info *isa,
         {
      const struct intel_device_info *devinfo = isa->devinfo;
            /* From Wa_1604601757:
   *
   * "When multiplying a DW and any lower precision integer, source modifier
   *  is not supported."
   */
   if (devinfo->ver >= 12 &&
      brw_inst_opcode(isa, inst) == BRW_OPCODE_MUL) {
   enum brw_reg_type exec_type = execution_type(isa, inst);
   const bool src0_valid = type_sz(brw_inst_src0_type(devinfo, inst)) == 4 ||
      brw_inst_src0_reg_file(devinfo, inst) == BRW_IMMEDIATE_VALUE ||
   !(brw_inst_src0_negate(devinfo, inst) ||
      const bool src1_valid = type_sz(brw_inst_src1_type(devinfo, inst)) == 4 ||
      brw_inst_src1_reg_file(devinfo, inst) == BRW_IMMEDIATE_VALUE ||
               ERROR_IF(!brw_reg_type_is_floating_point(exec_type) &&
            type_sz(exec_type) == 4 && !(src0_valid && src1_valid),
            if (brw_inst_opcode(isa, inst) == BRW_OPCODE_CMP ||
      brw_inst_opcode(isa, inst) == BRW_OPCODE_CMPN) {
   if (devinfo->ver <= 7) {
      /* Page 166 of the Ivy Bridge PRM Volume 4 part 3 (Execution Unit
   * ISA) says:
   *
   *    Accumulator cannot be destination, implicit or explicit. The
   *    destination must be a general register or the null register.
   *
   * Page 77 of the Haswell PRM Volume 2b contains the same text.  The
   * 965G PRMs contain similar text.
   *
   * Page 864 (page 880 of the PDF) of the Broadwell PRM Volume 7 says:
   *
   *    For the cmp and cmpn instructions, remove the accumulator
   *    restrictions.
   */
   ERROR_IF(brw_inst_dst_reg_file(devinfo, inst) == BRW_ARCHITECTURE_REGISTER_FILE &&
                     /* Page 166 of the Ivy Bridge PRM Volume 4 part 3 (Execution Unit ISA)
   * says:
   *
   *    If the destination is the null register, the {Switch} instruction
   *    option must be used.
   *
   * Page 77 of the Haswell PRM Volume 2b contains the same text.
   */
   if (devinfo->ver == 7) {
      ERROR_IF(dst_is_null(devinfo, inst) &&
            brw_inst_thread_control(devinfo, inst) != BRW_THREAD_SWITCH,
            ERROR_IF(brw_inst_cond_modifier(devinfo, inst) == BRW_CONDITIONAL_NONE,
               if (brw_inst_opcode(isa, inst) == BRW_OPCODE_SEL) {
      if (devinfo->ver < 6) {
      ERROR_IF(brw_inst_cond_modifier(devinfo, inst) != BRW_CONDITIONAL_NONE,
         ERROR_IF(brw_inst_pred_control(devinfo, inst) == BRW_PREDICATE_NONE,
      } else {
      ERROR_IF((brw_inst_cond_modifier(devinfo, inst) != BRW_CONDITIONAL_NONE) ==
                        if (brw_inst_opcode(isa, inst) == BRW_OPCODE_MUL) {
      const enum brw_reg_type src0_type = brw_inst_src0_type(devinfo, inst);
   const enum brw_reg_type src1_type = brw_inst_src1_type(devinfo, inst);
            if (devinfo->ver == 6) {
      /* Page 223 of the Sandybridge PRM volume 4 part 2 says:
   *
   *    [DevSNB]: When multiple (sic) a DW and a W, the W has to be on
   *    src0, and the DW has to be on src1.
   *
   * This text appears only in the Sandybridge PRMw.
   */
   ERROR_IF(brw_reg_type_is_integer(src0_type) &&
            type_sz(src0_type) == 4 && type_sz(src1_type) < 4,
   } else if (devinfo->ver >= 7) {
      /* Page 966 (page 982 of the PDF) of Broadwell PRM volume 2a says:
   *
   *    When multiplying a DW and any lower precision integer, the DW
   *    operand must on src0.
   *
   * Ivy Bridge, Haswell, Skylake, and Ice Lake PRMs contain the same
   * text.
   */
   ERROR_IF(brw_reg_type_is_integer(src1_type) &&
            type_sz(src0_type) < 4 && type_sz(src1_type) == 4,
            if (devinfo->ver <= 7) {
      /* Section 14.2.28 of Intel 965 Express Chipset PRM volume 4 says:
   *
   *    Source operands cannot be an accumulator register.
   *
   * Iron Lake, Sandybridge, and Ivy Bridge PRMs have the same text.
   * Haswell does not.  Given that later PRMs have different
   * restrictions on accumulator sources (see below), it seems most
   * likely that Haswell shares the Ivy Bridge restriction.
   */
   ERROR_IF(src0_is_acc(devinfo, inst) || src1_is_acc(devinfo, inst),
      } else {
      /* Page 971 (page 987 of the PDF), section "Accumulator
   * Restrictions," of the Broadwell PRM volume 7 says:
   *
   *    Integer source operands cannot be accumulators.
   *
   * The Skylake and Ice Lake PRMs contain the same text.
   */
   ERROR_IF((src0_is_acc(devinfo, inst) &&
            brw_reg_type_is_integer(src0_type)) ||
   (src1_is_acc(devinfo, inst) &&
            if (devinfo->ver <= 6) {
      /* Page 223 of the Sandybridge PRM volume 4 part 2 says:
   *
   *    Dword integer source is not allowed for this instruction in
   *    float execution mode.  In other words, if one source is of type
   *    float (:f, :vf), the other source cannot be of type dword
   *    integer (:ud or :d).
   *
   * G965 and Iron Lake PRMs have similar text.  Later GPUs do not
   * allow mixed source types at all, but that restriction should be
   * handled elsewhere.
   */
   ERROR_IF(execution_type(isa, inst) == BRW_REGISTER_TYPE_F &&
            (src0_type == BRW_REGISTER_TYPE_UD ||
   src0_type == BRW_REGISTER_TYPE_D ||
   src1_type == BRW_REGISTER_TYPE_UD ||
   src1_type == BRW_REGISTER_TYPE_D),
            if (devinfo->ver <= 7) {
      /* Page 118 of the Haswell PRM volume 2b says:
   *
   *    When operating on integers with at least one of the source
   *    being a DWord type (signed or unsigned), the destination cannot
   *    be floating-point (implementation note: the data converter only
   *    looks at the low 34 bits of the result).
   *
   * G965, Iron Lake, Sandybridge, and Ivy Bridge have similar text.
   * Later GPUs do not allow mixed source and destination types at all,
   * but that restriction should be handled elsewhere.
   */
   ERROR_IF(dst_type == BRW_REGISTER_TYPE_F &&
            (src0_type == BRW_REGISTER_TYPE_UD ||
   src0_type == BRW_REGISTER_TYPE_D ||
   src1_type == BRW_REGISTER_TYPE_UD ||
            if (devinfo->ver == 8) {
      /* Page 966 (page 982 of the PDF) of the Broadwell PRM volume 2a
   * says:
   *
   *    When multiplying DW x DW, the dst cannot be accumulator.
   *
   * This text also appears in the Cherry Trail / Braswell PRM, but it
   * does not appear in any other PRM.
   */
   ERROR_IF((src0_type == BRW_REGISTER_TYPE_UD ||
            src0_type == BRW_REGISTER_TYPE_D) &&
   (src1_type == BRW_REGISTER_TYPE_UD ||
   src1_type == BRW_REGISTER_TYPE_D) &&
   brw_inst_dst_reg_file(devinfo, inst) == BRW_ARCHITECTURE_REGISTER_FILE &&
            /* Page 935 (page 951 of the PDF) of the Ice Lake PRM volume 2a says:
   *
   *    When multiplying integer data types, if one of the sources is a
   *    DW, the resulting full precision data is stored in the
   *    accumulator. However, if the destination data type is either W or
   *    DW, the low bits of the result are written to the destination
   *    register and the remaining high bits are discarded. This results
   *    in undefined Overflow and Sign flags. Therefore, conditional
   *    modifiers and saturation (.sat) cannot be used in this case.
   *
   * Similar text appears in every version of the PRM.
   *
   * The wording of the last sentence is not very clear.  It could either
   * be interpreted as "conditional modifiers combined with saturation
   * cannot be used" or "neither conditional modifiers nor saturation can
   * be used."  I have interpreted it as the latter primarily because that
   * is the more restrictive interpretation.
   */
   ERROR_IF((src0_type == BRW_REGISTER_TYPE_UD ||
            src0_type == BRW_REGISTER_TYPE_D ||
   src1_type == BRW_REGISTER_TYPE_UD ||
   src1_type == BRW_REGISTER_TYPE_D) &&
   (dst_type == BRW_REGISTER_TYPE_UD ||
   dst_type == BRW_REGISTER_TYPE_D ||
   dst_type == BRW_REGISTER_TYPE_UW ||
   dst_type == BRW_REGISTER_TYPE_W) &&
   (brw_inst_saturate(devinfo, inst) != 0 ||
   brw_inst_cond_modifier(devinfo, inst) != BRW_CONDITIONAL_NONE),
            if (brw_inst_opcode(isa, inst) == BRW_OPCODE_MATH) {
      unsigned math_function = brw_inst_math_function(devinfo, inst);
   switch (math_function) {
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT:
   case BRW_MATH_FUNCTION_INT_DIV_REMAINDER: {
      /* Page 442 of the Broadwell PRM Volume 2a "Extended Math Function" says:
   *    INT DIV function does not support source modifiers.
   * Bspec 6647 extends it back to Ivy Bridge.
   */
   bool src0_valid = !brw_inst_src0_negate(devinfo, inst) &&
         bool src1_valid = !brw_inst_src1_negate(devinfo, inst) &&
         ERROR_IF(!src0_valid || !src1_valid,
            }
   default:
                     if (brw_inst_opcode(isa, inst) == BRW_OPCODE_DP4A) {
      /* Page 396 (page 412 of the PDF) of the DG1 PRM volume 2a says:
   *
   *    Only one of src0 or src1 operand may be an the (sic) accumulator
   *    register (acc#).
   */
   ERROR_IF(src0_is_acc(devinfo, inst) && src1_is_acc(devinfo, inst),
                        if (brw_inst_opcode(isa, inst) == BRW_OPCODE_ADD3) {
               ERROR_IF(dst_type != BRW_REGISTER_TYPE_D &&
            dst_type != BRW_REGISTER_TYPE_UD &&
               for (unsigned i = 0; i < 3; i++) {
               switch (i) {
   case 0: src_type = brw_inst_3src_a1_src0_type(devinfo, inst); break;
   case 1: src_type = brw_inst_3src_a1_src1_type(devinfo, inst); break;
   case 2: src_type = brw_inst_3src_a1_src2_type(devinfo, inst); break;
                  ERROR_IF(src_type != BRW_REGISTER_TYPE_D &&
            src_type != BRW_REGISTER_TYPE_UD &&
               if (i == 0) {
      if (brw_inst_3src_a1_src0_is_imm(devinfo, inst)) {
      ERROR_IF(src_type != BRW_REGISTER_TYPE_W &&
               } else if (i == 2) {
      if (brw_inst_3src_a1_src2_is_imm(devinfo, inst)) {
      ERROR_IF(src_type != BRW_REGISTER_TYPE_W &&
                              if (brw_inst_opcode(isa, inst) == BRW_OPCODE_OR ||
      brw_inst_opcode(isa, inst) == BRW_OPCODE_AND ||
   brw_inst_opcode(isa, inst) == BRW_OPCODE_XOR ||
   brw_inst_opcode(isa, inst) == BRW_OPCODE_NOT) {
   if (devinfo->ver >= 8) {
      /* While the behavior of the negate source modifier is defined as
   * logical not, the behavior of abs source modifier is not
   * defined. Disallow it to be safe.
   */
   ERROR_IF(brw_inst_src0_abs(devinfo, inst),
         ERROR_IF(brw_inst_opcode(isa, inst) != BRW_OPCODE_NOT &&
                        /* Page 479 (page 495 of the PDF) of the Broadwell PRM volume 2a says:
   *
   *    Source modifier is not allowed if source is an accumulator.
   *
   * The same text also appears for OR, NOT, and XOR instructions.
   */
   ERROR_IF((brw_inst_src0_abs(devinfo, inst) ||
            brw_inst_src0_negate(devinfo, inst)) &&
      ERROR_IF(brw_num_sources_from_inst(isa, inst) > 1 &&
            (brw_inst_src1_abs(devinfo, inst) ||
   brw_inst_src1_negate(devinfo, inst)) &&
            /* Page 479 (page 495 of the PDF) of the Broadwell PRM volume 2a says:
   *
   *    This operation does not produce sign or overflow conditions. Only
   *    the .e/.z or .ne/.nz conditional modifiers should be used.
   *
   * The same text also appears for OR, NOT, and XOR instructions.
   *
   * Per the comment around nir_op_imod in brw_fs_nir.cpp, we have
   * determined this to not be true. The only conditions that seem
   * absolutely sketchy are O, R, and U.  Some OpenGL shaders from Doom
   * 2016 have been observed to generate and.g and operate correctly.
   */
   const enum brw_conditional_mod cmod =
         ERROR_IF(cmod == BRW_CONDITIONAL_O ||
            cmod == BRW_CONDITIONAL_R ||
            if (brw_inst_opcode(isa, inst) == BRW_OPCODE_BFI2) {
      ERROR_IF(brw_inst_cond_modifier(devinfo, inst) != BRW_CONDITIONAL_NONE,
            ERROR_IF(brw_inst_saturate(devinfo, inst),
                     if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1)
         else
            ERROR_IF(dst_type != BRW_REGISTER_TYPE_D &&
                  for (unsigned s = 0; s < 3; s++) {
               if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      switch (s) {
   case 0: src_type = brw_inst_3src_a1_src0_type(devinfo, inst); break;
   case 1: src_type = brw_inst_3src_a1_src1_type(devinfo, inst); break;
   case 2: src_type = brw_inst_3src_a1_src2_type(devinfo, inst); break;
   default: unreachable("invalid src");
      } else {
                  ERROR_IF(src_type != dst_type,
                  if (brw_inst_opcode(isa, inst) == BRW_OPCODE_CSEL) {
      ERROR_IF(brw_inst_pred_control(devinfo, inst) != BRW_PREDICATE_NONE,
            /* CSEL is CMP and SEL fused into one. The condition modifier, which
   * does not actually modify the flags, controls the built-in comparison.
   */
   ERROR_IF(brw_inst_cond_modifier(devinfo, inst) == BRW_CONDITIONAL_NONE,
                     if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1)
         else
            if (devinfo->ver < 8) {
         } else if (devinfo->ver <= 9) {
      ERROR_IF(dst_type != BRW_REGISTER_TYPE_F,
      } else {
      ERROR_IF(dst_type != BRW_REGISTER_TYPE_F &&
            dst_type != BRW_REGISTER_TYPE_HF &&
   dst_type != BRW_REGISTER_TYPE_D &&
            for (unsigned s = 0; s < 3; s++) {
               if (brw_inst_access_mode(devinfo, inst) == BRW_ALIGN_1) {
      switch (s) {
   case 0: src_type = brw_inst_3src_a1_src0_type(devinfo, inst); break;
   case 1: src_type = brw_inst_3src_a1_src1_type(devinfo, inst); break;
   case 2: src_type = brw_inst_3src_a1_src2_type(devinfo, inst); break;
   default: unreachable("invalid src");
      } else {
                  ERROR_IF(src_type != dst_type,
                     }
      static struct string
   send_descriptor_restrictions(const struct brw_isa_info *isa,
         {
      const struct intel_device_info *devinfo = isa->devinfo;
            if (inst_is_split_send(isa, inst)) {
      /* We can only validate immediate descriptors */
   if (brw_inst_send_sel_reg32_desc(devinfo, inst))
      } else if (inst_is_send(isa, inst)) {
      /* We can only validate immediate descriptors */
   if (brw_inst_src1_reg_file(devinfo, inst) != BRW_IMMEDIATE_VALUE)
      } else {
                           switch (brw_inst_sfid(devinfo, inst)) {
   case BRW_SFID_URB:
      if (devinfo->ver < 20)
            case GFX12_SFID_TGM:
   case GFX12_SFID_SLM:
   case GFX12_SFID_UGM:
               ERROR_IF(lsc_opcode_has_transpose(lsc_msg_desc_opcode(devinfo, desc)) &&
            lsc_msg_desc_transpose(devinfo, desc) &&
            default:
                  if (brw_inst_sfid(devinfo, inst) == BRW_SFID_URB && devinfo->ver < 20) {
      /* Gfx4 doesn't have a "header present" bit in the SEND message. */
   ERROR_IF(devinfo->ver > 4 && !brw_inst_header_present(devinfo, inst),
            switch (brw_inst_urb_opcode(devinfo, inst)) {
   case BRW_URB_OPCODE_WRITE_HWORD:
            /* case FF_SYNC: */
   case BRW_URB_OPCODE_WRITE_OWORD:
      /* Gfx5 / Gfx6 FF_SYNC message and Gfx7+ URB_WRITE_OWORD have the
   * same opcode value.
   */
   if (devinfo->ver == 5 || devinfo->ver == 6) {
      ERROR_IF(brw_inst_urb_global_offset(devinfo, inst) != 0,
         ERROR_IF(brw_inst_urb_swizzle_control(devinfo, inst) != 0,
         ERROR_IF(brw_inst_urb_used(devinfo, inst) != 0,
                        /* Volume 4 part 2 of the Sandybridge PRM (page 28) says:
   *
   *    A message response (writeback) length of 1 GRF will be
   *    indicated on the ‘send’ instruction if the thread requires
   *    response data and/or synchronization.
   */
   ERROR_IF((unsigned)brw_inst_rlen(devinfo, inst) > 1,
      } else {
      ERROR_IF(devinfo->ver < 7,
                  case BRW_URB_OPCODE_READ_HWORD:
   case BRW_URB_OPCODE_READ_OWORD:
      ERROR_IF(devinfo->ver < 7,
               case GFX7_URB_OPCODE_ATOMIC_MOV:
   case GFX7_URB_OPCODE_ATOMIC_INC:
      ERROR_IF(devinfo->ver < 7,
               case GFX8_URB_OPCODE_ATOMIC_ADD:
      /* The Haswell PRM lists this opcode as valid on page 317. */
   ERROR_IF(devinfo->verx10 < 75,
               case GFX8_URB_OPCODE_SIMD8_READ:
      ERROR_IF(brw_inst_rlen(devinfo, inst) == 0,
               case GFX8_URB_OPCODE_SIMD8_WRITE:
      ERROR_IF(devinfo->ver < 8,
               case GFX125_URB_OPCODE_FENCE:
      ERROR_IF(devinfo->verx10 < 125,
               default:
      ERROR_IF(true, "Invalid URB message");
                     }
      bool
   brw_validate_instruction(const struct brw_isa_info *isa,
                     {
               if (is_unsupported_inst(isa, inst)) {
         } else {
               if (error_msg.str == NULL) {
      CHECK(sources_not_null);
   CHECK(send_restrictions);
   CHECK(alignment_supported);
   CHECK(general_restrictions_based_on_operand_types);
   CHECK(general_restrictions_on_region_parameters);
   CHECK(special_restrictions_for_mixed_float_mode);
   CHECK(region_alignment_rules);
   CHECK(vector_immediate_restrictions);
   CHECK(special_requirements_for_handling_double_precision_data_types);
   CHECK(instruction_restrictions);
                  if (error_msg.str && disasm) {
         }
               }
      bool
   brw_validate_instructions(const struct brw_isa_info *isa,
               {
      const struct intel_device_info *devinfo = isa->devinfo;
            for (int src_offset = start_offset; src_offset < end_offset;) {
      const brw_inst *inst = assembly + src_offset;
   bool is_compact = brw_inst_cmpt_control(devinfo, inst);
   unsigned inst_size = is_compact ? sizeof(brw_compact_inst)
                  if (is_compact) {
      brw_compact_inst *compacted = (void *)inst;
   brw_uncompact_instruction(isa, &uncompacted, compacted);
               bool v = brw_validate_instruction(isa, inst, src_offset,
                                 }
