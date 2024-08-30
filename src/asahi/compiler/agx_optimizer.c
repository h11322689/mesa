   /*
   * Copyright 2021 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "agx_compiler.h"
   #include "agx_minifloat.h"
      /* AGX peephole optimizer responsible for instruction combining. It operates in
   * a forward direction and a backward direction, in each case traversing in
   * source order. SSA means the forward pass satisfies the invariant:
   *
   *    Every def is visited before any of its uses.
   *
   * Dually, the backend pass satisfies the invariant:
   *
   *    Every use of a def is visited before the def.
   *
   * This means the forward pass can propagate modifiers forward, whereas the
   * backwards pass propagates modifiers backward. Consider an example:
   *
   *    1 = fabs 0
   *    2 = fround 1
   *    3 = fsat 1
   *
   * The forwards pass would propagate the fabs to the fround (since we can
   * lookup the fabs from the fround source and do the replacement). By contrast
   * the backwards pass would propagate the fsat back to the fround (since when
   * we see the fround we know it has only a single user, fsat).  Propagatable
   * instruction have natural directions (like pushforwards and pullbacks).
   *
   * We are careful to update the tracked state whenever we modify an instruction
   * to ensure the passes are linear-time and converge in a single iteration.
   *
   * Size conversions are worth special discussion. Consider the snippet:
   *
   *    2 = fadd 0, 1
   *    3 = f2f16 2
   *    4 = fround 3
   *
   * A priori, we can move the f2f16 in either direction. But it's not equal --
   * if we move it up to the fadd, we get FP16 for two instructions, whereas if
   * we push it into the fround, we effectively get FP32 for two instructions. So
   * f2f16 is backwards. Likewise, consider
   *
   *    2 = fadd 0, 1
   *    3 = f2f32 1
   *    4 = fround 3
   *
   * This time if we move f2f32 up to the fadd, we get FP32 for two, but if we
   * move it down to the fround, we get FP16 to too. So f2f32 is backwards.
   */
      static bool
   agx_is_fmov(agx_instr *def)
   {
      return (def->op == AGX_OPCODE_FADD) &&
      }
      /* Compose floating-point modifiers with floating-point sources */
      static agx_index
   agx_compose_float_src(agx_index to, agx_index from)
   {
      if (to.abs) {
      from.neg = false;
                           }
      static void
   agx_optimizer_fmov(agx_instr **defs, agx_instr *ins)
   {
      agx_foreach_ssa_src(ins, s) {
      agx_index src = ins->src[s];
            if (def == NULL)
         if (!agx_is_fmov(def))
         if (def->saturate)
         if (ins->op == AGX_OPCODE_FCMPSEL && s >= 2)
            /* We can fold f2f32 into 32-bit instructions, but we can't fold f2f16
   * into 16-bit instructions, since the latter would implicitly promote to
   * a 32-bit instruction which is not exact.
   */
   assert(def->src[0].size == AGX_SIZE_32 ||
                  if (src.size == AGX_SIZE_16 && def->src[0].size == AGX_SIZE_32)
                  }
      static bool
   image_write_source_can_be_immediate(agx_instr *I, unsigned s)
   {
               /* LOD can always be immediate. Actually, it's just zero so far, we don't
   * support nonzero LOD for images yet.
   */
   if (s == 2)
            /* If the "bindless" source (source 3) is an immediate, it means we don't
   * have a bindless image, instead we have a texture state index. We're
   * allowed to have immediate texture state registers (source 4). However,
   * we're not allowed to have immediate bindless offsets (also source 4).
   */
   bool is_texture_state = (I->src[3].type == AGX_INDEX_IMMEDIATE);
   if (s == 4 && is_texture_state)
            /* Otherwise, must be from a register */
      }
      static void
   agx_optimizer_inline_imm(agx_instr **defs, agx_instr *I, unsigned srcs,
         {
      for (unsigned s = 0; s < srcs; ++s) {
      agx_index src = I->src[s];
   if (src.type != AGX_INDEX_NORMAL)
         if (src.neg)
            agx_instr *def = defs[src.value];
   if (def->op != AGX_OPCODE_MOV_IMM)
            uint8_t value = def->imm;
                     /* fcmpsel takes first 2 as floats specially */
   if (s < 2 && I->op == AGX_OPCODE_FCMPSEL)
         if (I->op == AGX_OPCODE_ST_TILE && s == 0)
         if (I->op == AGX_OPCODE_ZS_EMIT && s != 0)
         if ((I->op == AGX_OPCODE_DEVICE_STORE ||
      I->op == AGX_OPCODE_LOCAL_STORE || I->op == AGX_OPCODE_ATOMIC ||
   I->op == AGX_OPCODE_LOCAL_ATOMIC) &&
   s != 2)
      if ((I->op == AGX_OPCODE_LOCAL_LOAD || I->op == AGX_OPCODE_DEVICE_LOAD) &&
                  if (I->op == AGX_OPCODE_IMAGE_WRITE &&
                  if (float_src) {
                     float f = fp16 ? _mesa_half_to_float(def->imm) : uif(def->imm);
                     } else if (value == def->imm) {
         } else if (value_u16 == def->imm && agx_allows_16bit_immediate(I)) {
               }
      static bool
   agx_optimizer_fmov_rev(agx_instr *I, agx_instr *use)
   {
      if (!agx_is_fmov(use))
         if (use->src[0].neg || use->src[0].abs)
            /* We can fold f2f16 into 32-bit instructions, but we can't fold f2f32 into
   * 16-bit instructions, since the latter would implicitly promote to a 32-bit
   * instruction which is not exact.
   */
   assert(use->dest[0].size == AGX_SIZE_32 || use->dest[0].size == AGX_SIZE_16);
            if (I->dest[0].size == AGX_SIZE_16 && use->dest[0].size == AGX_SIZE_32)
            /* saturate(saturate(x)) = saturate(x) */
   I->saturate |= use->saturate;
   I->dest[0] = use->dest[0];
      }
      static void
   agx_optimizer_copyprop(agx_instr **defs, agx_instr *I)
   {
      agx_foreach_ssa_src(I, s) {
      agx_index src = I->src[s];
            if (def == NULL)
         if (def->op != AGX_OPCODE_MOV)
            /* At the moment, not all instructions support size conversions. Notably
   * RA pseudo instructions don't handle size conversions. This should be
   * refined in the future.
   */
   if (def->src[0].size != src.size)
            /* Immediate inlining happens elsewhere */
   if (def->src[0].type == AGX_INDEX_IMMEDIATE)
            /* ALU instructions cannot take 64-bit */
   if (def->src[0].size == AGX_SIZE_64 &&
      !(I->op == AGX_OPCODE_DEVICE_LOAD && s == 0) &&
   !(I->op == AGX_OPCODE_DEVICE_STORE && s == 1) &&
                     }
      /*
   * Fuse conditions into if. Specifically, acts on if_icmp and fuses:
   *
   *    if_icmp(cmp(x, y, *), 0, ne) -> if_cmp(x, y, *)
   */
   static void
   agx_optimizer_if_cmp(agx_instr **defs, agx_instr *I)
   {
      /* Check for unfused if */
   if (!agx_is_equiv(I->src[1], agx_zero()) || I->icond != AGX_ICOND_UEQ ||
      !I->invert_cond || I->src[0].type != AGX_INDEX_NORMAL)
         /* Check for condition */
   agx_instr *def = defs[I->src[0].value];
   if (def->op != AGX_OPCODE_ICMP && def->op != AGX_OPCODE_FCMP)
            /* Fuse */
   I->src[0] = def->src[0];
   I->src[1] = def->src[1];
            if (def->op == AGX_OPCODE_ICMP) {
      I->op = AGX_OPCODE_IF_ICMP;
      } else {
      I->op = AGX_OPCODE_IF_FCMP;
         }
      /*
   * Fuse conditions into select. Specifically, acts on icmpsel and fuses:
   *
   *    icmpsel(cmp(x, y, *), 0, z, w, eq) -> cmpsel(x, y, w, z, *)
   *
   * Care must be taken to invert the condition by swapping cmpsel arguments.
   */
   static void
   agx_optimizer_cmpsel(agx_instr **defs, agx_instr *I)
   {
      /* Check for unfused select */
   if (!agx_is_equiv(I->src[1], agx_zero()) || I->icond != AGX_ICOND_UEQ ||
      I->src[0].type != AGX_INDEX_NORMAL)
         /* Check for condition */
   agx_instr *def = defs[I->src[0].value];
   if (def->op != AGX_OPCODE_ICMP && def->op != AGX_OPCODE_FCMP)
            /* Fuse */
   I->src[0] = def->src[0];
            /* In the unfused select, the condition is inverted due to the form:
   *
   *    (cond == 0) ? x : y
   *
   * So we need to swap the arguments when fusing to become cond ? y : x. If
   * the condition was supposed to be inverted, we don't swap since it's
   * already inverted. cmpsel does not have an invert_cond bit to use.
   */
   if (!def->invert_cond) {
      agx_index temp = I->src[2];
   I->src[2] = I->src[3];
               if (def->op == AGX_OPCODE_ICMP) {
      I->op = AGX_OPCODE_ICMPSEL;
      } else {
      I->op = AGX_OPCODE_FCMPSEL;
         }
      static void
   agx_optimizer_forward(agx_context *ctx)
   {
               agx_foreach_instr_global(ctx, I) {
               agx_foreach_ssa_dest(I, d) {
                  /* Optimize moves */
            /* Propagate fmov down */
   if (info.is_float || I->op == AGX_OPCODE_FCMPSEL)
            /* Inline immediates if we can. TODO: systematic */
   if (I->op != AGX_OPCODE_ST_VARY && I->op != AGX_OPCODE_COLLECT &&
      I->op != AGX_OPCODE_TEXTURE_SAMPLE &&
   I->op != AGX_OPCODE_IMAGE_LOAD && I->op != AGX_OPCODE_TEXTURE_LOAD &&
   I->op != AGX_OPCODE_UNIFORM_STORE &&
               if (I->op == AGX_OPCODE_IF_ICMP)
         else if (I->op == AGX_OPCODE_ICMPSEL)
                  }
      static void
   agx_optimizer_backward(agx_context *ctx)
   {
      agx_instr **uses = calloc(ctx->alloc, sizeof(*uses));
            agx_foreach_instr_global_rev(ctx, I) {
               agx_foreach_ssa_src(I, s) {
                        if (uses[v])
         else
                  if (info.nr_dests != 1)
            if (I->dest[0].type != AGX_INDEX_NORMAL)
                     if (!use || BITSET_TEST(multiple, I->dest[0].value))
            /* Destination has a single use, try to propagate */
   if (info.is_float && agx_optimizer_fmov_rev(I, use)) {
      agx_remove_instruction(use);
                  free(uses);
      }
      void
   agx_optimizer(agx_context *ctx)
   {
      agx_optimizer_backward(ctx);
      }
