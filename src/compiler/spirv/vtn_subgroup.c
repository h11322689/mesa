   /*
   * Copyright © 2016 Intel Corporation
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
      #include "vtn_private.h"
      static struct vtn_ssa_value *
   vtn_build_subgroup_instr(struct vtn_builder *b,
                           nir_intrinsic_op nir_op,
   {
      /* Some of the subgroup operations take an index.  SPIR-V allows this to be
   * any integer type.  To make things simpler for drivers, we only support
   * 32-bit indices.
   */
   if (index && index->bit_size != 32)
                     vtn_assert(dst->type == src0->type);
   if (!glsl_type_is_vector_or_scalar(dst->type)) {
      for (unsigned i = 0; i < glsl_get_length(dst->type); i++) {
      dst->elems[0] =
      vtn_build_subgroup_instr(b, nir_op, src0->elems[i], index,
   }
               nir_intrinsic_instr *intrin =
         nir_def_init_for_type(&intrin->instr, &intrin->def, dst->type);
            intrin->src[0] = nir_src_for_ssa(src0->def);
   if (index)
            intrin->const_index[0] = const_idx0;
                                 }
      void
   vtn_handle_subgroup(struct vtn_builder *b, SpvOp opcode,
         {
               switch (opcode) {
   case SpvOpGroupNonUniformElect: {
      vtn_fail_if(dest_type->type != glsl_bool_type(),
         nir_intrinsic_instr *elect =
         nir_def_init_for_type(&elect->instr, &elect->def, dest_type->type);
   nir_builder_instr_insert(&b->nb, &elect->instr);
   vtn_push_nir_ssa(b, w[2], &elect->def);
               case SpvOpGroupNonUniformBallot:
   case SpvOpSubgroupBallotKHR: {
      bool has_scope = (opcode != SpvOpSubgroupBallotKHR);
   vtn_fail_if(dest_type->type != glsl_vector_type(GLSL_TYPE_UINT, 4),
         nir_intrinsic_instr *ballot =
         ballot->src[0] = nir_src_for_ssa(vtn_get_nir_ssa(b, w[3 + has_scope]));
   nir_def_init(&ballot->instr, &ballot->def, 4, 32);
   ballot->num_components = 4;
   nir_builder_instr_insert(&b->nb, &ballot->instr);
   vtn_push_nir_ssa(b, w[2], &ballot->def);
               case SpvOpGroupNonUniformInverseBallot: {
      nir_def *dest = nir_inverse_ballot(&b->nb, 1, vtn_get_nir_ssa(b, w[4]));
   vtn_push_nir_ssa(b, w[2], dest);
               case SpvOpGroupNonUniformBallotBitExtract:
   case SpvOpGroupNonUniformBallotBitCount:
   case SpvOpGroupNonUniformBallotFindLSB:
   case SpvOpGroupNonUniformBallotFindMSB: {
      nir_def *src0, *src1 = NULL;
   nir_intrinsic_op op;
   switch (opcode) {
   case SpvOpGroupNonUniformBallotBitExtract:
      op = nir_intrinsic_ballot_bitfield_extract;
   src0 = vtn_get_nir_ssa(b, w[4]);
   src1 = vtn_get_nir_ssa(b, w[5]);
      case SpvOpGroupNonUniformBallotBitCount:
      switch ((SpvGroupOperation)w[4]) {
   case SpvGroupOperationReduce:
      op = nir_intrinsic_ballot_bit_count_reduce;
      case SpvGroupOperationInclusiveScan:
      op = nir_intrinsic_ballot_bit_count_inclusive;
      case SpvGroupOperationExclusiveScan:
      op = nir_intrinsic_ballot_bit_count_exclusive;
      default:
         }
   src0 = vtn_get_nir_ssa(b, w[5]);
      case SpvOpGroupNonUniformBallotFindLSB:
      op = nir_intrinsic_ballot_find_lsb;
   src0 = vtn_get_nir_ssa(b, w[4]);
      case SpvOpGroupNonUniformBallotFindMSB:
      op = nir_intrinsic_ballot_find_msb;
   src0 = vtn_get_nir_ssa(b, w[4]);
      default:
                  nir_intrinsic_instr *intrin =
            intrin->src[0] = nir_src_for_ssa(src0);
   if (src1)
            nir_def_init_for_type(&intrin->instr, &intrin->def,
                  vtn_push_nir_ssa(b, w[2], &intrin->def);
               case SpvOpGroupNonUniformBroadcastFirst:
   case SpvOpSubgroupFirstInvocationKHR: {
      bool has_scope = (opcode != SpvOpSubgroupFirstInvocationKHR);
   vtn_push_ssa_value(b, w[2],
      vtn_build_subgroup_instr(b, nir_intrinsic_read_first_invocation,
                        case SpvOpGroupNonUniformBroadcast:
   case SpvOpGroupBroadcast:
   case SpvOpSubgroupReadInvocationKHR: {
      bool has_scope = (opcode != SpvOpSubgroupReadInvocationKHR);
   vtn_push_ssa_value(b, w[2],
      vtn_build_subgroup_instr(b, nir_intrinsic_read_invocation,
                        case SpvOpGroupNonUniformAll:
   case SpvOpGroupNonUniformAny:
   case SpvOpGroupNonUniformAllEqual:
   case SpvOpGroupAll:
   case SpvOpGroupAny:
   case SpvOpSubgroupAllKHR:
   case SpvOpSubgroupAnyKHR:
   case SpvOpSubgroupAllEqualKHR: {
      vtn_fail_if(dest_type->type != glsl_bool_type(),
         nir_intrinsic_op op;
   switch (opcode) {
   case SpvOpGroupNonUniformAll:
   case SpvOpGroupAll:
   case SpvOpSubgroupAllKHR:
      op = nir_intrinsic_vote_all;
      case SpvOpGroupNonUniformAny:
   case SpvOpGroupAny:
   case SpvOpSubgroupAnyKHR:
      op = nir_intrinsic_vote_any;
      case SpvOpSubgroupAllEqualKHR:
      op = nir_intrinsic_vote_ieq;
      case SpvOpGroupNonUniformAllEqual:
      switch (glsl_get_base_type(vtn_ssa_value(b, w[4])->type)) {
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_DOUBLE:
      op = nir_intrinsic_vote_feq;
      case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
   case GLSL_TYPE_BOOL:
      op = nir_intrinsic_vote_ieq;
      default:
         }
      default:
                  nir_def *src0;
   if (opcode == SpvOpGroupNonUniformAll || opcode == SpvOpGroupAll ||
      opcode == SpvOpGroupNonUniformAny || opcode == SpvOpGroupAny ||
   opcode == SpvOpGroupNonUniformAllEqual) {
      } else {
         }
   nir_intrinsic_instr *intrin =
         if (nir_intrinsic_infos[op].src_components[0] == 0)
         intrin->src[0] = nir_src_for_ssa(src0);
   nir_def_init_for_type(&intrin->instr, &intrin->def,
                  vtn_push_nir_ssa(b, w[2], &intrin->def);
               case SpvOpGroupNonUniformShuffle:
   case SpvOpGroupNonUniformShuffleXor:
   case SpvOpGroupNonUniformShuffleUp:
   case SpvOpGroupNonUniformShuffleDown: {
      nir_intrinsic_op op;
   switch (opcode) {
   case SpvOpGroupNonUniformShuffle:
      op = nir_intrinsic_shuffle;
      case SpvOpGroupNonUniformShuffleXor:
      op = nir_intrinsic_shuffle_xor;
      case SpvOpGroupNonUniformShuffleUp:
      op = nir_intrinsic_shuffle_up;
      case SpvOpGroupNonUniformShuffleDown:
      op = nir_intrinsic_shuffle_down;
      default:
         }
   vtn_push_ssa_value(b, w[2],
      vtn_build_subgroup_instr(b, op, vtn_ssa_value(b, w[4]),
                  case SpvOpSubgroupShuffleINTEL:
   case SpvOpSubgroupShuffleXorINTEL: {
      nir_intrinsic_op op = opcode == SpvOpSubgroupShuffleINTEL ?
         vtn_push_ssa_value(b, w[2],
      vtn_build_subgroup_instr(b, op, vtn_ssa_value(b, w[3]),
                  case SpvOpSubgroupShuffleUpINTEL:
   case SpvOpSubgroupShuffleDownINTEL: {
      /* TODO: Move this lower on the compiler stack, where we can move the
   * current/other data to adjacent registers to avoid doing a shuffle
   * twice.
            nir_builder *nb = &b->nb;
   nir_def *size = nir_load_subgroup_size(nb);
            /* Rewrite UP in terms of DOWN.
   *
   *   UP(a, b, delta) == DOWN(a, b, size - delta)
   */
   if (opcode == SpvOpSubgroupShuffleUpINTEL)
            nir_def *index = nir_iadd(nb, nir_load_subgroup_invocation(nb), delta);
   struct vtn_ssa_value *current =
                  struct vtn_ssa_value *next =
                  nir_def *cond = nir_ilt(nb, index, size);
                        case SpvOpGroupNonUniformRotateKHR: {
      const mesa_scope scope = vtn_translate_scope(b, vtn_constant_uint(b, w[3]));
   const uint32_t cluster_size = count > 6 ? vtn_constant_uint(b, w[6]) : 0;
   vtn_fail_if(cluster_size && !IS_POT(cluster_size),
            struct vtn_ssa_value *value = vtn_ssa_value(b, w[4]);
   struct vtn_ssa_value *delta = vtn_ssa_value(b, w[5]);
   vtn_push_nir_ssa(b, w[2],
      vtn_build_subgroup_instr(b, nir_intrinsic_rotate,
                  case SpvOpGroupNonUniformQuadBroadcast:
      vtn_push_ssa_value(b, w[2],
      vtn_build_subgroup_instr(b, nir_intrinsic_quad_broadcast,
                  case SpvOpGroupNonUniformQuadSwap: {
      unsigned direction = vtn_constant_uint(b, w[5]);
   nir_intrinsic_op op;
   switch (direction) {
   case 0:
      op = nir_intrinsic_quad_swap_horizontal;
      case 1:
      op = nir_intrinsic_quad_swap_vertical;
      case 2:
      op = nir_intrinsic_quad_swap_diagonal;
      default:
         }
   vtn_push_ssa_value(b, w[2],
                     case SpvOpGroupNonUniformIAdd:
   case SpvOpGroupNonUniformFAdd:
   case SpvOpGroupNonUniformIMul:
   case SpvOpGroupNonUniformFMul:
   case SpvOpGroupNonUniformSMin:
   case SpvOpGroupNonUniformUMin:
   case SpvOpGroupNonUniformFMin:
   case SpvOpGroupNonUniformSMax:
   case SpvOpGroupNonUniformUMax:
   case SpvOpGroupNonUniformFMax:
   case SpvOpGroupNonUniformBitwiseAnd:
   case SpvOpGroupNonUniformBitwiseOr:
   case SpvOpGroupNonUniformBitwiseXor:
   case SpvOpGroupNonUniformLogicalAnd:
   case SpvOpGroupNonUniformLogicalOr:
   case SpvOpGroupNonUniformLogicalXor:
   case SpvOpGroupIAdd:
   case SpvOpGroupFAdd:
   case SpvOpGroupFMin:
   case SpvOpGroupUMin:
   case SpvOpGroupSMin:
   case SpvOpGroupFMax:
   case SpvOpGroupUMax:
   case SpvOpGroupSMax:
   case SpvOpGroupIAddNonUniformAMD:
   case SpvOpGroupFAddNonUniformAMD:
   case SpvOpGroupFMinNonUniformAMD:
   case SpvOpGroupUMinNonUniformAMD:
   case SpvOpGroupSMinNonUniformAMD:
   case SpvOpGroupFMaxNonUniformAMD:
   case SpvOpGroupUMaxNonUniformAMD:
   case SpvOpGroupSMaxNonUniformAMD: {
      nir_op reduction_op;
   switch (opcode) {
   case SpvOpGroupNonUniformIAdd:
   case SpvOpGroupIAdd:
   case SpvOpGroupIAddNonUniformAMD:
      reduction_op = nir_op_iadd;
      case SpvOpGroupNonUniformFAdd:
   case SpvOpGroupFAdd:
   case SpvOpGroupFAddNonUniformAMD:
      reduction_op = nir_op_fadd;
      case SpvOpGroupNonUniformIMul:
      reduction_op = nir_op_imul;
      case SpvOpGroupNonUniformFMul:
      reduction_op = nir_op_fmul;
      case SpvOpGroupNonUniformSMin:
   case SpvOpGroupSMin:
   case SpvOpGroupSMinNonUniformAMD:
      reduction_op = nir_op_imin;
      case SpvOpGroupNonUniformUMin:
   case SpvOpGroupUMin:
   case SpvOpGroupUMinNonUniformAMD:
      reduction_op = nir_op_umin;
      case SpvOpGroupNonUniformFMin:
   case SpvOpGroupFMin:
   case SpvOpGroupFMinNonUniformAMD:
      reduction_op = nir_op_fmin;
      case SpvOpGroupNonUniformSMax:
   case SpvOpGroupSMax:
   case SpvOpGroupSMaxNonUniformAMD:
      reduction_op = nir_op_imax;
      case SpvOpGroupNonUniformUMax:
   case SpvOpGroupUMax:
   case SpvOpGroupUMaxNonUniformAMD:
      reduction_op = nir_op_umax;
      case SpvOpGroupNonUniformFMax:
   case SpvOpGroupFMax:
   case SpvOpGroupFMaxNonUniformAMD:
      reduction_op = nir_op_fmax;
      case SpvOpGroupNonUniformBitwiseAnd:
   case SpvOpGroupNonUniformLogicalAnd:
      reduction_op = nir_op_iand;
      case SpvOpGroupNonUniformBitwiseOr:
   case SpvOpGroupNonUniformLogicalOr:
      reduction_op = nir_op_ior;
      case SpvOpGroupNonUniformBitwiseXor:
   case SpvOpGroupNonUniformLogicalXor:
      reduction_op = nir_op_ixor;
      default:
                  nir_intrinsic_op op;
   unsigned cluster_size = 0;
   switch ((SpvGroupOperation)w[4]) {
   case SpvGroupOperationReduce:
      op = nir_intrinsic_reduce;
      case SpvGroupOperationInclusiveScan:
      op = nir_intrinsic_inclusive_scan;
      case SpvGroupOperationExclusiveScan:
      op = nir_intrinsic_exclusive_scan;
      case SpvGroupOperationClusteredReduce:
      op = nir_intrinsic_reduce;
   assert(count == 7);
   cluster_size = vtn_constant_uint(b, w[6]);
      default:
                  vtn_push_ssa_value(b, w[2],
      vtn_build_subgroup_instr(b, op, vtn_ssa_value(b, w[5]), NULL,
                  default:
            }
