   /*
   * Copyright © 2017 Intel Corporation
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
      #include "util/u_math.h"
   #include "nir.h"
   #include "nir_builder.h"
      /**
   * \file nir_opt_intrinsics.c
   */
      static nir_intrinsic_instr *
   lower_subgroups_64bit_split_intrinsic(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      nir_def *comp;
   if (component == 0)
         else
            nir_intrinsic_instr *intr = nir_intrinsic_instr_create(b->shader, intrin->intrinsic);
   nir_def_init(&intr->instr, &intr->def, 1, 32);
   intr->const_index[0] = intrin->const_index[0];
   intr->const_index[1] = intrin->const_index[1];
   intr->src[0] = nir_src_for_ssa(comp);
   if (nir_intrinsic_infos[intrin->intrinsic].num_srcs == 2)
            intr->num_components = 1;
   nir_builder_instr_insert(b, &intr->instr);
      }
      static nir_def *
   lower_subgroup_op_to_32bit(nir_builder *b, nir_intrinsic_instr *intrin)
   {
      assert(intrin->src[0].ssa->bit_size == 64);
   nir_intrinsic_instr *intr_x = lower_subgroups_64bit_split_intrinsic(b, intrin, 0);
   nir_intrinsic_instr *intr_y = lower_subgroups_64bit_split_intrinsic(b, intrin, 1);
      }
      static nir_def *
   ballot_type_to_uint(nir_builder *b, nir_def *value,
         {
      /* Only the new-style SPIR-V subgroup instructions take a ballot result as
   * an argument, so we only use this on uvec4 types.
   */
            return nir_extract_bits(b, &value, 1, 0, options->ballot_components,
      }
      static nir_def *
   uint_to_ballot_type(nir_builder *b, nir_def *value,
         {
      assert(util_is_power_of_two_nonzero(num_components));
                     /* If the source doesn't have enough bits, zero-pad */
   if (total_bits > value->bit_size * value->num_components)
                     /* If the source has too many components, truncate.  This can happen if,
   * for instance, we're implementing GL_ARB_shader_ballot or
   * VK_EXT_shader_subgroup_ballot which have 64-bit ballot values on an
   * architecture with a native 128-bit uvec4 ballot.  This comes up in Zink
   * for OpenGL on Vulkan.  It's the job of the driver calling this lowering
   * pass to ensure that it's restricted subgroup sizes sufficiently that we
   * have enough ballot bits.
   */
   if (value->num_components > num_components)
               }
      static nir_def *
   lower_subgroup_op_to_scalar(nir_builder *b, nir_intrinsic_instr *intrin)
   {
      /* This is safe to call on scalar things but it would be silly */
            nir_def *value = intrin->src[0].ssa;
            for (unsigned i = 0; i < intrin->num_components; i++) {
      nir_intrinsic_instr *chan_intrin =
         nir_def_init(&chan_intrin->instr, &chan_intrin->def, 1,
                  /* value */
   chan_intrin->src[0] = nir_src_for_ssa(nir_channel(b, value, i));
   /* invocation */
   if (nir_intrinsic_infos[intrin->intrinsic].num_srcs > 1) {
      assert(nir_intrinsic_infos[intrin->intrinsic].num_srcs == 2);
               chan_intrin->const_index[0] = intrin->const_index[0];
            nir_builder_instr_insert(b, &chan_intrin->instr);
                  }
      static nir_def *
   lower_vote_eq_to_scalar(nir_builder *b, nir_intrinsic_instr *intrin)
   {
               nir_def *result = NULL;
   for (unsigned i = 0; i < intrin->num_components; i++) {
               if (intrin->intrinsic == nir_intrinsic_vote_feq) {
         } else {
                  if (result) {
         } else {
                        }
      static nir_def *
   lower_vote_eq(nir_builder *b, nir_intrinsic_instr *intrin)
   {
               /* We have to implicitly lower to scalar */
   nir_def *all_eq = NULL;
   for (unsigned i = 0; i < intrin->num_components; i++) {
               nir_def *is_eq;
   if (intrin->intrinsic == nir_intrinsic_vote_feq) {
         } else {
                  if (all_eq == NULL) {
         } else {
                        }
      static nir_def *
   lower_shuffle_to_swizzle(nir_builder *b, nir_intrinsic_instr *intrin)
   {
               if (mask >= 32)
            return nir_masked_swizzle_amd(b, intrin->src[0].ssa,
            }
      /* Lowers "specialized" shuffles to a generic nir_intrinsic_shuffle. */
      static nir_def *
   lower_to_shuffle(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (intrin->intrinsic == nir_intrinsic_shuffle_xor &&
      options->lower_shuffle_to_swizzle_amd &&
            nir_def *result = lower_shuffle_to_swizzle(b, intrin);
   if (result)
               nir_def *index = nir_load_subgroup_invocation(b);
   switch (intrin->intrinsic) {
   case nir_intrinsic_shuffle_xor:
      index = nir_ixor(b, index, intrin->src[1].ssa);
      case nir_intrinsic_shuffle_up:
      index = nir_isub(b, index, intrin->src[1].ssa);
      case nir_intrinsic_shuffle_down:
      index = nir_iadd(b, index, intrin->src[1].ssa);
      case nir_intrinsic_quad_broadcast:
      index = nir_ior(b, nir_iand_imm(b, index, ~0x3),
            case nir_intrinsic_quad_swap_horizontal:
      /* For Quad operations, subgroups are divided into quads where
   * (invocation % 4) is the index to a square arranged as follows:
   *
   *    +---+---+
   *    | 0 | 1 |
   *    +---+---+
   *    | 2 | 3 |
   *    +---+---+
   */
   index = nir_ixor(b, index, nir_imm_int(b, 0x1));
      case nir_intrinsic_quad_swap_vertical:
      index = nir_ixor(b, index, nir_imm_int(b, 0x2));
      case nir_intrinsic_quad_swap_diagonal:
      index = nir_ixor(b, index, nir_imm_int(b, 0x3));
      case nir_intrinsic_rotate: {
      nir_def *delta = intrin->src[1].ssa;
   nir_def *local_id = nir_load_subgroup_invocation(b);
            nir_def *rotation_group_mask =
            index = nir_iand(b, nir_iadd(b, local_id, delta),
         if (cluster_size > 0) {
      index = nir_iadd(b, index,
      }
      }
   default:
                     }
      static const struct glsl_type *
   glsl_type_for_ssa(nir_def *def)
   {
      const struct glsl_type *comp_type = def->bit_size == 1 ? glsl_bool_type() : glsl_uintN_t_type(def->bit_size);
      }
      /* Lower nir_intrinsic_shuffle to a waterfall loop + nir_read_invocation.
   */
   static nir_def *
   lower_shuffle(nir_builder *b, nir_intrinsic_instr *intrin)
   {
      nir_def *val = intrin->src[0].ssa;
            /* The loop is something like:
   *
   * while (true) {
   *    first_id = readFirstInvocation(gl_SubgroupInvocationID);
   *    first_val = readFirstInvocation(val);
   *    first_result = readInvocation(val, readFirstInvocation(id));
   *    if (id == first_id)
   *       result = first_val;
   *    if (elect()) {
   *       if (id > gl_SubgroupInvocationID) {
   *          result = first_result;
   *       }
   *       break;
   *    }
   * }
   *
   * The idea is to guarantee, on each iteration of the loop, that anything
   * reading from first_id gets the correct value, so that we can then kill
   * it off by breaking out of the loop. Before doing that we also have to
   * ensure that first_id invocation gets the correct value. It only won't be
   * assigned the correct value already if the invocation it's reading from
   * isn't already killed off, that is, if it's later than its own ID.
   * Invocations where id <= gl_SubgroupInvocationID will be assigned their
   * result in the first if, and invocations where id >
   * gl_SubgroupInvocationID will be assigned their result in the second if.
   *
   * We do this more complicated loop rather than looping over all id's
   * explicitly because at this point we don't know the "actual" subgroup
   * size and at the moment there's no way to get at it, which means we may
   * loop over always-inactive invocations.
                     nir_variable *result =
            nir_loop *loop = nir_push_loop(b);
   {
      nir_def *first_id = nir_read_first_invocation(b, subgroup_id);
   nir_def *first_val = nir_read_first_invocation(b, val);
   nir_def *first_result =
            nir_if *nif = nir_push_if(b, nir_ieq(b, id, first_id));
   {
         }
            nir_if *nif2 = nir_push_if(b, nir_elect(b, 1));
   {
      nir_if *nif3 = nir_push_if(b, nir_ult(b, subgroup_id, id));
   {
                           }
      }
               }
      static bool
   lower_subgroups_filter(const nir_instr *instr, const void *_options)
   {
         }
      /* Return a ballot-mask-sized value which represents "val" sign-extended and
   * then shifted left by "shift". Only particular values for "val" are
   * supported, see below.
   */
   static nir_def *
   build_ballot_imm_ishl(nir_builder *b, int64_t val, nir_def *shift,
         {
      /* This only works if all the high bits are the same as bit 1. */
            /* First compute the result assuming one ballot component. */
   nir_def *result =
            if (options->ballot_components == 1)
            /* Fix up the result when there is > 1 component. The idea is that nir_ishl
   * masks out the high bits of the shift value already, so in case there's
   * more than one component the component which 1 would be shifted into
   * already has the right value and all we have to do is fixup the other
   * components. Components below it should always be 0, and components above
   * it must be either 0 or ~0 because of the assert above. For example, if
   * the target ballot size is 2 x uint32, and we're shifting 1 by 33, then
   * we'll feed 33 into ishl, which will mask it off to get 1, so we'll
   * compute a single-component result of 2, which is correct for the second
   * component, but the first component needs to be 0, which we get by
   * comparing the high bits of the shift with 0 and selecting the original
   * answer or 0 for the first component (and something similar with the
   * second component). This idea is generalized here for any component count
   */
   nir_const_value min_shift[4];
   for (unsigned i = 0; i < options->ballot_components; i++)
                  nir_const_value max_shift[4];
   for (unsigned i = 0; i < options->ballot_components; i++)
                  return nir_bcsel(b, nir_ult(b, shift, max_shift_val),
                  nir_bcsel(b, nir_ult(b, shift, min_shift_val),
   }
      static nir_def *
   build_subgroup_eq_mask(nir_builder *b,
         {
                  }
      static nir_def *
   build_subgroup_ge_mask(nir_builder *b,
         {
                  }
      static nir_def *
   build_subgroup_gt_mask(nir_builder *b,
         {
                  }
      /* Return a mask which is 1 for threads up to the run-time subgroup size, i.e.
   * 1 for the entire subgroup. SPIR-V requires us to return 0 for indices at or
   * above the subgroup size for the masks, but gt_mask and ge_mask make them 1
   * so we have to "and" with this mask.
   */
   static nir_def *
   build_subgroup_mask(nir_builder *b,
         {
               /* First compute the result assuming one ballot component. */
   nir_def *result =
      nir_ushr(b, nir_imm_intN_t(b, ~0ull, options->ballot_bit_size),
               /* Since the subgroup size and ballot bitsize are both powers of two, there
   * are two possible cases to consider:
   *
   * (1) The subgroup size is less than the ballot bitsize. We need to return
   * "result" in the first component and 0 in every other component.
   * (2) The subgroup size is a multiple of the ballot bitsize. We need to
   * return ~0 if the subgroup size divided by the ballot bitsize is less
   * than or equal to the index in the vector and 0 otherwise. For example,
   * with a target ballot type of 4 x uint32 and subgroup_size = 64 we'd need
   * to return { ~0, ~0, 0, 0 }.
   *
   * In case (2) it turns out that "result" will be ~0, because
   * "ballot_bit_size - subgroup_size" is also a multiple of
   * "ballot_bit_size" and since nir_ushr masks the shift value it will
   * shifted by 0. This means that the first component can just be "result"
   * in all cases.  The other components will also get the correct value in
   * case (1) if we just use the rule in case (2), so we'll get the correct
   * result if we just follow (2) and then replace the first component with
   * "result".
   */
   nir_const_value min_idx[4];
   for (unsigned i = 0; i < options->ballot_components; i++)
                  nir_def *result_extended =
            return nir_bcsel(b, nir_ult(b, min_idx_val, subgroup_size),
      }
      static nir_def *
   vec_bit_count(nir_builder *b, nir_def *value)
   {
      nir_def *vec_result = nir_bit_count(b, value);
   nir_def *result = nir_channel(b, vec_result, 0);
   for (unsigned i = 1; i < value->num_components; i++)
            }
      static nir_def *
   vec_find_lsb(nir_builder *b, nir_def *value)
   {
      nir_def *vec_result = nir_find_lsb(b, value);
   nir_def *result = nir_imm_int(b, -1);
   for (int i = value->num_components - 1; i >= 0; i--) {
      nir_def *channel = nir_channel(b, vec_result, i);
   /* result = channel >= 0 ? (i * bitsize + channel) : result */
   result = nir_bcsel(b, nir_ige_imm(b, channel, 0),
            }
      }
      static nir_def *
   vec_find_msb(nir_builder *b, nir_def *value)
   {
      nir_def *vec_result = nir_ufind_msb(b, value);
   nir_def *result = nir_imm_int(b, -1);
   for (unsigned i = 0; i < value->num_components; i++) {
      nir_def *channel = nir_channel(b, vec_result, i);
   /* result = channel >= 0 ? (i * bitsize + channel) : result */
   result = nir_bcsel(b, nir_ige_imm(b, channel, 0),
            }
      }
      static nir_def *
   lower_dynamic_quad_broadcast(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (!options->lower_quad_broadcast_dynamic_to_const)
                     for (unsigned i = 0; i < 4; ++i) {
      nir_def *qbcst = nir_quad_broadcast(b, intrin->src[0].ssa,
            if (i)
      dst = nir_bcsel(b, nir_ieq_imm(b, intrin->src[1].ssa, i),
      else
                  }
      static nir_def *
   lower_read_invocation_to_cond(nir_builder *b, nir_intrinsic_instr *intrin)
   {
      return nir_read_invocation_cond_ir3(b, intrin->def.bit_size,
                  }
      static nir_def *
   lower_subgroups_instr(nir_builder *b, nir_instr *instr, void *_options)
   {
               nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_vote_any:
   case nir_intrinsic_vote_all:
      if (options->lower_vote_trivial)
               case nir_intrinsic_vote_feq:
   case nir_intrinsic_vote_ieq:
      if (options->lower_vote_trivial)
            if (options->lower_vote_eq)
            if (options->lower_to_scalar && intrin->num_components > 1)
               case nir_intrinsic_load_subgroup_size:
      if (options->subgroup_size)
               case nir_intrinsic_read_invocation:
      if (options->lower_to_scalar && intrin->num_components > 1)
            if (options->lower_read_invocation_to_cond)
                  case nir_intrinsic_read_first_invocation:
      if (options->lower_to_scalar && intrin->num_components > 1)
               case nir_intrinsic_load_subgroup_eq_mask:
   case nir_intrinsic_load_subgroup_ge_mask:
   case nir_intrinsic_load_subgroup_gt_mask:
   case nir_intrinsic_load_subgroup_le_mask:
   case nir_intrinsic_load_subgroup_lt_mask: {
      if (!options->lower_subgroup_masks)
            nir_def *val;
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_subgroup_eq_mask:
      val = build_subgroup_eq_mask(b, options);
      case nir_intrinsic_load_subgroup_ge_mask:
      val = nir_iand(b, build_subgroup_ge_mask(b, options),
            case nir_intrinsic_load_subgroup_gt_mask:
      val = nir_iand(b, build_subgroup_gt_mask(b, options),
            case nir_intrinsic_load_subgroup_le_mask:
      val = nir_inot(b, build_subgroup_gt_mask(b, options));
      case nir_intrinsic_load_subgroup_lt_mask:
      val = nir_inot(b, build_subgroup_ge_mask(b, options));
      default:
                  return uint_to_ballot_type(b, val,
                     case nir_intrinsic_ballot: {
      if (intrin->def.num_components == options->ballot_components &&
                  nir_def *ballot =
                  return uint_to_ballot_type(b, ballot,
                     case nir_intrinsic_inverse_ballot:
      if (options->lower_inverse_ballot) {
      return nir_ballot_bitfield_extract(b, 1, intrin->src[0].ssa,
      } else if (intrin->src[0].ssa->num_components != options->ballot_components ||
               }
         case nir_intrinsic_ballot_bitfield_extract:
   case nir_intrinsic_ballot_bit_count_reduce:
   case nir_intrinsic_ballot_find_lsb:
   case nir_intrinsic_ballot_find_msb: {
      nir_def *int_val = ballot_type_to_uint(b, intrin->src[0].ssa,
            if (intrin->intrinsic != nir_intrinsic_ballot_bitfield_extract &&
      intrin->intrinsic != nir_intrinsic_ballot_find_lsb) {
   /* For OpGroupNonUniformBallotFindMSB, the SPIR-V Spec says:
   *
   *    "Find the most significant bit set to 1 in Value, considering
   *    only the bits in Value required to represent all bits of the
   *    group’s invocations.  If none of the considered bits is set to
   *    1, the result is undefined."
   *
   * It has similar text for the other three.  This means that, in case
   * the subgroup size is less than 32, we have to mask off the unused
   * bits.  If the subgroup size is fixed and greater than or equal to
   * 32, the mask will be 0xffffffff and nir_opt_algebraic will delete
   * the iand.
   *
   * We only have to worry about this for BitCount and FindMSB because
   * FindLSB counts from the bottom and BitfieldExtract selects
   * individual bits.  In either case, if run outside the range of
   * valid bits, we hit the undefined results case and we can return
   * anything we want.
   */
               switch (intrin->intrinsic) {
   case nir_intrinsic_ballot_bitfield_extract: {
      nir_def *idx = intrin->src[1].ssa;
   if (int_val->num_components > 1) {
      /* idx will be truncated by nir_ushr, so we just need to select
   * the right component using the bits of idx that are truncated in
   * the shift.
   */
   int_val =
                        }
   case nir_intrinsic_ballot_bit_count_reduce:
         case nir_intrinsic_ballot_find_lsb:
         case nir_intrinsic_ballot_find_msb:
         default:
                     case nir_intrinsic_ballot_bit_count_exclusive:
   case nir_intrinsic_ballot_bit_count_inclusive: {
      nir_def *int_val = ballot_type_to_uint(b, intrin->src[0].ssa,
         if (options->lower_ballot_bit_count_to_mbcnt_amd) {
      nir_def *acc;
   if (intrin->intrinsic == nir_intrinsic_ballot_bit_count_exclusive) {
         } else {
      acc = nir_iand_imm(b, nir_u2u32(b, int_val), 0x1);
      }
               nir_def *mask;
   if (intrin->intrinsic == nir_intrinsic_ballot_bit_count_inclusive) {
         } else {
                              case nir_intrinsic_elect: {
      if (!options->lower_elect)
                        case nir_intrinsic_shuffle:
      if (options->lower_shuffle)
         else if (options->lower_to_scalar && intrin->num_components > 1)
         else if (options->lower_shuffle_to_32bit && intrin->src[0].ssa->bit_size == 64)
            case nir_intrinsic_shuffle_xor:
   case nir_intrinsic_shuffle_up:
   case nir_intrinsic_shuffle_down:
      if (options->lower_relative_shuffle)
         else if (options->lower_to_scalar && intrin->num_components > 1)
         else if (options->lower_shuffle_to_32bit && intrin->src[0].ssa->bit_size == 64)
               case nir_intrinsic_quad_broadcast:
   case nir_intrinsic_quad_swap_horizontal:
   case nir_intrinsic_quad_swap_vertical:
   case nir_intrinsic_quad_swap_diagonal:
      if (options->lower_quad ||
      (options->lower_quad_broadcast_dynamic &&
   intrin->intrinsic == nir_intrinsic_quad_broadcast &&
   !nir_src_is_const(intrin->src[1])))
      else if (options->lower_to_scalar && intrin->num_components > 1)
               case nir_intrinsic_reduce: {
      nir_def *ret = NULL;
   /* A cluster size greater than the subgroup size is implemention defined */
   if (options->subgroup_size &&
      nir_intrinsic_cluster_size(intrin) >= options->subgroup_size) {
   nir_intrinsic_set_cluster_size(intrin, 0);
      }
   if (options->lower_to_scalar && intrin->num_components > 1)
            }
   case nir_intrinsic_inclusive_scan:
   case nir_intrinsic_exclusive_scan:
      if (options->lower_to_scalar && intrin->num_components > 1)
               case nir_intrinsic_rotate:
      if (nir_intrinsic_execution_scope(intrin) == SCOPE_SUBGROUP) {
      if (options->lower_rotate_to_shuffle)
         else if (options->lower_to_scalar && intrin->num_components > 1)
         else if (options->lower_shuffle_to_32bit && intrin->src[0].ssa->bit_size == 64)
      }
      case nir_intrinsic_masked_swizzle_amd:
      if (options->lower_to_scalar && intrin->num_components > 1) {
         } else if (options->lower_shuffle_to_32bit && intrin->src[0].ssa->bit_size == 64) {
         }
         default:
                     }
      bool
   nir_lower_subgroups(nir_shader *shader,
         {
      return nir_shader_lower_instructions(shader,
                  }
